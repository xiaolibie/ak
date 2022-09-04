#include "pch-il2cpp.h"
#include "KillAura.h"

#include <helpers.h>
#include <algorithm>

#include <cheat/events.h>
#include <cheat/game/EntityManager.h>
#include <cheat/game/util.h>
#include <cheat/game/filters.h>

namespace cheat::feature 
{
	static void BaseMoveSyncPlugin_ConvertSyncTaskToMotionInfo_Hook(app::BaseMoveSyncPlugin* __this, MethodInfo* method);

    KillAura::KillAura() : Feature(),
		NF(f_Enabled, u8"杀戮光环", u8"杀戮领域", false),
		NF(f_DamageMode, u8"破坏模式", u8"破坏模式", false),
		NF(f_PercentDamageMode, u8"损伤模式百分比", u8"破坏模式", false),
		NF(f_InstantDeathMode, u8"立即死亡", "Instant death", false),
		NF(f_OnlyTargeted, u8"只针对", u8"杀戮领域", true),
		NF(f_Range, u8"范围", "KillAura", 15.0f),
		NF(f_AttackDelay, u8"攻击延迟时间（毫秒)", u8"杀戮领域", 100),
		NF(f_RepeatDelay, u8"重复延迟时间（毫秒）", u8"杀戮领域", 1000),
		NF(f_DamageValue, u8"碰撞损伤值", u8"破坏模式", 233.0f),
		NF(f_PercentDamageTimes, u8"杀死时间", u8"破坏模式", 3)
    { 
		events::GameUpdateEvent += MY_METHOD_HANDLER(KillAura::OnGameUpdate);
		HookManager::install(app::MoleMole_BaseMoveSyncPlugin_ConvertSyncTaskToMotionInfo, BaseMoveSyncPlugin_ConvertSyncTaskToMotionInfo_Hook);
	}

    const FeatureGUIInfo& KillAura::GetGUIInfo() const
    {
        static const FeatureGUIInfo info{ u8"杀戮领域", u8"大世界", true };
        return info;
    }

    void KillAura::DrawMain()
    {
		ConfigWidget(u8"开关", f_Enabled, u8"只是总开关，需要在下面设置模式.");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(255, 165, 0, 255), u8"选择一个或者多个理想的模式.");

		ConfigWidget(u8"破坏伤害模式", f_DamageMode, u8"杀死光环对你周围的怪物造成碰撞伤害.");
		ImGui::Indent();
		ConfigWidget(u8"百分比模式", f_PercentDamageMode, u8"碰撞损坏百分比值.");
		if (f_DamageMode)
		{
			if (!f_PercentDamageMode)
			{
				ConfigWidget(u8"伤害值", f_DamageValue, 1, 0, 10000000, u8"碰撞损伤值");
			}
			else
			{
				ConfigWidget(u8"死亡时间", f_PercentDamageTimes, 1, 1, 100, u8"杀死需要的时间.");
			}
		}
		ImGui::Unindent();
		ConfigWidget(u8"瞬间模式", f_InstantDeathMode, u8"尝试作用于周围所有目标.");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(255, 165, 0, 255), u8"可能与机械阵列和水无相等出现bug.");
		ConfigWidget(u8"范围", f_Range, 0.1f, 5.0f, 100.0f);
		ConfigWidget(u8"敌不犯我，我不犯人", f_OnlyTargeted, u8"如果启用，杀戮光环将只影响以你为目标的怪物/对你的攻击.");
		ConfigWidget(u8"伤害延迟  (ms)", f_AttackDelay, 1, 0, 1000, u8"下一次碰撞损坏前的延迟（毫秒）.");
		ConfigWidget(u8"个体延迟 (ms)", f_RepeatDelay, 1, 100, 2000, u8"碰撞前延迟毫秒，同一怪物.");
    }

    bool KillAura::NeedStatusDraw() const
	{
        return f_Enabled;
    }

    void KillAura::DrawStatus() 
    { 
        ImGui::Text(u8"杀戮空间 [%s%s]\n[%.01fm|%s|%dms|%dms]",
			f_DamageMode && f_InstantDeathMode ? "Extreme" : f_DamageMode ? "Crash" : f_InstantDeathMode ? "Instant" : "None",
			f_DamageMode ? !f_PercentDamageMode ? "|Fixed" : fmt::format("|Rate({})", f_PercentDamageTimes.value()).c_str() : "",
			f_Range.value(),
			f_OnlyTargeted ? "Aggro" : "All",
			f_AttackDelay.value(),
			f_RepeatDelay.value());
    }

    KillAura& KillAura::GetInstance()
    {
        static KillAura instance;
        return instance;
    }

	// Kill aura logic is just emulate monster fall crash, so simple but works.
	// Note. No work on mob with shield, maybe update like auto ore destroy.
	void KillAura::OnGameUpdate()
	{
		static std::default_random_engine generator;
		static std::uniform_int_distribution<int> distribution(-50, 50);

		static int64_t nextAttackTime = 0;
		static std::map<uint32_t, int64_t> monsterRepeatTimeMap;
		static std::queue<game::Entity*> attackQueue;
		static std::unordered_set<uint32_t> attackSet;

		if (!f_Enabled || !f_DamageMode)
			return;

		auto eventManager = GET_SINGLETON(MoleMole_EventManager);
		if (eventManager == nullptr || *app::MoleMole_EventHelper_Allocate_103__MethodInfo == nullptr)
			return;

		auto currentTime = util::GetCurrentTimeMillisec();
		if (currentTime < nextAttackTime)
			return;

		auto& manager = game::EntityManager::instance();

		for (const auto& monster : manager.entities(game::filters::combined::Monsters))   
		{
			auto monsterID = monster->runtimeID();

			if (attackSet.count(monsterID) > 0)
				continue;

			if (monsterRepeatTimeMap.count(monsterID) > 0 && monsterRepeatTimeMap[monsterID] > currentTime)
				continue;

			auto combat = monster->combat();
			if (combat == nullptr)
				continue;

			auto combatProp = combat->fields._combatProperty_k__BackingField;
			if (combatProp == nullptr)
				continue;

			auto maxHP = app::MoleMole_SafeFloat_get_Value(combatProp->fields.maxHP, nullptr);
			auto isLockHp = combatProp->fields.islockHP == nullptr || app::MoleMole_FixedBoolStack_get_value(combatProp->fields.islockHP, nullptr);
			auto isInvincible = combatProp->fields.isInvincible == nullptr || app::MoleMole_FixedBoolStack_get_value(combatProp->fields.isInvincible, nullptr);
			auto HP = app::MoleMole_SafeFloat_get_Value(combatProp->fields.HP, nullptr);
			if (maxHP < 10 || HP < 2 || isLockHp || isInvincible)
				continue;

			if (f_OnlyTargeted && combat->fields._attackTarget.runtimeID != manager.avatar()->runtimeID())
				continue;

			if (manager.avatar()->distance(monster) > f_Range)
				continue;

			attackQueue.push(monster);
			attackSet.insert(monsterID);
		}

		if (attackQueue.empty())
			return;

		auto monster = attackQueue.front();
		attackQueue.pop();

		if (!monster->isLoaded())
		{
			// If monster entity isn't active means that it was unloaded (it happen when player teleport or moving fast)
			// And we don't have way to get id
			// So better to clear all queue, to prevent memory leak
			// This happen rarely, so don't give any performance issues
			std::queue<game::Entity*> empty;
			std::swap(attackQueue, empty);

			attackSet.clear();
			return;
		}

		attackSet.erase(monster->runtimeID());
		
		auto combat = monster->combat();

		auto crashEvt = app::MoleMole_EventHelper_Allocate_103(*app::MoleMole_EventHelper_Allocate_103__MethodInfo);
		app::MoleMole_EvtCrash_Init(crashEvt, monster->runtimeID(), nullptr);
		
		if(!f_PercentDamageMode)
		{
			//Migita^Rin#1762: Fixed inaccurate damage caused by floating point precision(Maybe)
			float FPValue;
			if (f_DamageValue <= 10000000) FPValue = 27.0f;
			if (f_DamageValue <= 9000000) FPValue = 22.5f;
			if (f_DamageValue <= 8000000) FPValue = 20.0f;
			if (f_DamageValue <= 7000000) FPValue = 17.5f;
			if (f_DamageValue <= 6000000) FPValue = 15.0f;
			if (f_DamageValue <= 5000000) FPValue = 12.5f;
			if (f_DamageValue <= 4000000) FPValue = 10.0f;
			if (f_DamageValue <= 3000000) FPValue = 7.5f;
			if (f_DamageValue <= 2000000) FPValue = 5.0f;
			if (f_DamageValue <= 1000000) FPValue = 2.5f;

			crashEvt->fields.maxHp = f_DamageValue / 0.4f + FPValue;
			crashEvt->fields.velChange = 10000000;
		}
		else
		{
			auto maxHP = app::MoleMole_SafeFloat_get_Value(combat->fields._combatProperty_k__BackingField->fields.maxHP, nullptr);
			// should works. :p
			crashEvt->fields.maxHp = maxHP / f_PercentDamageTimes / 0.4f + 5.0f;
			crashEvt->fields.velChange = 10000000;
		}
		crashEvt->fields.hitPos = monster->absolutePosition();

		app::MoleMole_EventManager_FireEvent(eventManager, reinterpret_cast<app::BaseEvent*>(crashEvt), false, nullptr);

		monsterRepeatTimeMap[monster->runtimeID()] = currentTime + (int)f_RepeatDelay + distribution(generator);

		nextAttackTime = currentTime + (int)f_AttackDelay + distribution(generator);
	}

	static void OnSyncTask(app::BaseMoveSyncPlugin* moveSync)
	{
		KillAura& killAura = KillAura::GetInstance();
		if (!killAura.f_Enabled || !killAura.f_InstantDeathMode)
			return;

		auto& manager = game::EntityManager::instance();
		auto avatarID = manager.avatar()->runtimeID();
		auto entityID = moveSync->fields._.owner->fields.entityRuntimeID;

		if (entityID == avatarID)
			return;

		auto monster = manager.entity(entityID);
		auto combat = monster->combat();
		if (combat == nullptr)
			return;

		if (killAura.f_OnlyTargeted && combat->fields._attackTarget.runtimeID != avatarID)
			return;

		if (manager.avatar()->distance(monster) > killAura.f_Range)
			return;

		moveSync->fields._syncTask.position.x = 1000000.0f;
	}

	static void BaseMoveSyncPlugin_ConvertSyncTaskToMotionInfo_Hook(app::BaseMoveSyncPlugin* __this, MethodInfo* method)
	{
		OnSyncTask(__this);
		CALL_ORIGIN(BaseMoveSyncPlugin_ConvertSyncTaskToMotionInfo_Hook, __this, method);
	}
}


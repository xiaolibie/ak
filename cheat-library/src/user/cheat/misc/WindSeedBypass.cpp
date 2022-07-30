#include "pch-il2cpp.h"
#include "WindSeedBypass.h"

#include <cheat/native.h>
#include <helpers.h>

namespace cheat::feature
{
	static void PlayerModule_OnWindSeedClientNotify_Hook(app::MoleMole_PlayerModule* __this, app::Proto_WindSeedClientNotify* notify, MethodInfo* method);
	static void PlayerModule_OnReciveLuaShell_Hook(app::MoleMole_PlayerModule* __this, app::Proto_PlayerLuaShellNotify* playerLuaShellNotify, MethodInfo* method);

	WindSeedBypass::WindSeedBypass() : Feature(),
		NFEX(f_Enabled, "Block WindSeed", "m_BlockWindSeed", "General", true, false)
	{
		HookManager::install(app::MoleMole_PlayerModule_OnWindSeedClientNotify, PlayerModule_OnWindSeedClientNotify_Hook);
		HookManager::install(app::MoleMole_PlayerModule_OnReciveLuaShell, PlayerModule_OnReciveLuaShell_Hook);
	}

	const FeatureGUIInfo& WindSeedBypass::GetGUIInfo() const
	{
		static const FeatureGUIInfo info{ u8"阻断风种子", "Settings", true };
		return info;
	}

	void WindSeedBypass::DrawMain()
	{
		ConfigWidget(f_Enabled,
			u8"阻止可能导致来自恶意服务器的风种子数据包.");
	}

	bool WindSeedBypass::NeedStatusDraw() const
	{
		return f_Enabled;
	}

	void WindSeedBypass::DrawStatus()
	{
		ImGui::Text(u8"阻断风种子");
	}

	WindSeedBypass& WindSeedBypass::GetInstance()
	{
		static WindSeedBypass instance;
		return instance;
	}

	void PlayerModule_OnWindSeedClientNotify_Hook(app::MoleMole_PlayerModule* __this, app::Proto_WindSeedClientNotify* notify, MethodInfo* method)
	{
		WindSeedBypass& WindSeedBypass = WindSeedBypass::GetInstance();
		if (WindSeedBypass.f_Enabled)
		{
			LOG_DEBUG("Server sent a WindSeedClientNotify packet!");
			return;
		}

		CALL_ORIGIN(PlayerModule_OnWindSeedClientNotify_Hook, __this, notify, method);
	}

	void PlayerModule_OnReciveLuaShell_Hook(app::MoleMole_PlayerModule* __this, app::Proto_PlayerLuaShellNotify* playerLuaShellNotify, MethodInfo* method)
	{
		WindSeedBypass& WindSeedBypass = WindSeedBypass::GetInstance();
		if (WindSeedBypass.f_Enabled)
		{
			LOG_DEBUG("Server sent a PlayerLuaShellNotify packet!");
			return;
		}

		CALL_ORIGIN(PlayerModule_OnReciveLuaShell_Hook, __this, playerLuaShellNotify, method);
	}
}

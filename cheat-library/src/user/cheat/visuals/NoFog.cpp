#include "pch-il2cpp.h"
#include "NoFog.h"

#include <helpers.h>
#include <cheat/events.h>

namespace cheat::feature
{
    static bool _prevEnabledState = false;
    NoFog::NoFog() : Feature(),
        NFEX(f_Enabled, u8"去雾", "NoFog", "Visuals", false, false)
    {
        events::GameUpdateEvent += MY_METHOD_HANDLER(NoFog::OnGameUpdate);
    }

    const FeatureGUIInfo& NoFog::GetGUIInfo() const
    {
        static const FeatureGUIInfo info{ u8"去雾", u8"模组和视觉类", false };
        return info;
    }

    void NoFog::DrawMain()
    {
        ConfigWidget(f_Enabled, u8"去雾.");
    }

    bool NoFog::NeedStatusDraw() const
    {
        return f_Enabled;
    }

    void NoFog::DrawStatus()
    {
        ImGui::Text(u8"去雾");
    }

    NoFog& NoFog::GetInstance()
    {
        static NoFog instance;
        return instance;
    }

    void NoFog::OnGameUpdate()
    {
		if (_prevEnabledState != f_Enabled)
		{
			app::RenderSettings_set_fog(!f_Enabled, nullptr);
			_prevEnabledState = f_Enabled;
		}
    }
}


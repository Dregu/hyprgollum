#include <string>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/config/values/types/Vec2Value.hpp>

#include "gollum.hpp"

inline HANDLE  PHANDLE = nullptr;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
    const std::string CLIENT_HASH     = __hyprland_api_get_client_hash();

    if (COMPOSITOR_HASH != CLIENT_HASH) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprgollum] Mismatched headers! Can't proceed.", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprgollum] Version mismatch");
    }

    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CVec2Value>("plugin:gollum:grid", "grid size", Config::VEC2{1, 1}));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CStringValue>("plugin:gollum:fit", "fit mode", "t"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CStringValue>("plugin:gollum:order", "custom order", ""));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CStringValue>("plugin:gollum:dir", "orientation", "l"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CStringValue>("plugin:gollum:new", "new status", "b"));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CIntValue>("plugin:gollum:wrap", "wrapping", 0));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CIntValue>("plugin:gollum:fs", "fullscreen mode", 2));
    HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::CIntValue>("plugin:gollum:mono", "monocle mode", 1));

    if (!HyprlandAPI::addTiledAlgo(PHANDLE, "gollum", &typeid(Layout::Tiled::CGollumAlgorithm), [] { return makeUnique<Layout::Tiled::CGollumAlgorithm>(PHANDLE); })) {
        HyprlandAPI::addNotification(PHANDLE, "[hyprgollum] addTiledAlgo failed! Can't proceed.", CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[hyprgollum] addTiledAlgo failed");
    }

    return {"hyprgollum", "Autofit column layout for Hyprland", "Dregu", "0.55.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::removeAlgo(PHANDLE, "gollum");
}
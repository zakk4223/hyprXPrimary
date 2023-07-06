#define WLR_USE_UNSTABLE

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include "globals.hpp"

#include <unistd.h>
#include <thread>

// Methods
inline CFunctionHook* g_pSetXWaylandScaleHook = nullptr;


// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}


void setXWaylandNames() {
  wl_resource* res = nullptr;
  for (auto& m : g_pCompositor->m_vMonitors) {
    wl_list_for_each(res, &m->output->resources, link) {
      const auto PCLIENT = wl_resource_get_client(res);
      if (PCLIENT == g_pXWaylandManager->m_sWLRXWayland->server->client) {
        wl_output_send_name(res, m->szName.c_str());
      }
    }
  }

}
typedef void (*origSetXWaylandScale)(void *thisptr, std::optional<double> scale);

void hksetXWaylandScale(void *thisptr, std::optional<double> scale) { 
	(*(origSetXWaylandScale)g_pSetXWaylandScaleHook->m_pOriginal)(thisptr, scale);
  setXWaylandNames();
  g_pEventManager->postEvent({"xwaylandupdate", ""});
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;


    static const auto XWAYLANDSCALEMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "setXWaylandScale");

    g_pSetXWaylandScaleHook = HyprlandAPI::createFunctionHook(PHANDLE, XWAYLANDSCALEMETHODS[0].address, (void*)&hksetXWaylandScale);
    g_pSetXWaylandScaleHook->hook();
    setXWaylandNames();
    return {"XWayland Name fix", "Fix xwayland names because stupid", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}

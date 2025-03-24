#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#define private public
#include <hyprland/src/xwayland/XWayland.hpp>
#undef private
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include "globals.hpp"

#include <unistd.h>
#include <thread>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <cstring>

// Methods
// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}


namespace XwaylandPrimaryPlugin {


SP<HOOK_CALLBACK_FN> prerenderHook;

  void setXWaylandPrimary() {
    if (!g_pXWayland || !g_pXWayland->pWM || !g_pXWayland->pWM->connection || !g_pXWayland->pWM->screen) {
      Debug::log(LOG, "XWaylandPrimary: No XWayland client");
      return;
    }
    static auto* const PRIMARYNAME = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:xwaylandprimary:display")->getDataStaticPtr();
    static auto* const FOLLOWFOCUS = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:xwaylandprimary:followfocused")->getDataStaticPtr();
		auto dofollow = **FOLLOWFOCUS;

    auto PMONITOR = g_pCompositor->getMonitorFromName(std::string{*PRIMARYNAME});

    if (dofollow && g_pCompositor->m_pLastMonitor)
    {
      PMONITOR = g_pCompositor->m_pLastMonitor.lock();
    }

    if (!PMONITOR) {
      Debug::log(LOG, "XWaylandPrimary: Could not find monitor {}", std::string{*PRIMARYNAME});
      return;
    }
  
	  xcb_connection_t *XCBCONN = g_pXWayland->pWM->connection;
    xcb_screen_t *screen = g_pXWayland->pWM->screen; 
  
    xcb_randr_get_screen_resources_cookie_t res_cookie = xcb_randr_get_screen_resources(XCBCONN, screen->root);
    xcb_randr_get_screen_resources_reply_t *res_reply = xcb_randr_get_screen_resources_reply(XCBCONN, res_cookie, 0);
    xcb_timestamp_t timestamp = res_reply->config_timestamp;
  
		int output_cnt = 0;
		xcb_randr_output_t *x_outputs;
  
		output_cnt = xcb_randr_get_screen_resources_outputs_length(res_reply);
		x_outputs = xcb_randr_get_screen_resources_outputs(res_reply);


		for (int i = 0; i < output_cnt; i++) {
			xcb_randr_get_output_info_reply_t *output = xcb_randr_get_output_info_reply(XCBCONN, xcb_randr_get_output_info(XCBCONN, x_outputs[i], timestamp), NULL);
			if (output == NULL) {
				continue;
			}
			uint8_t *output_name = xcb_randr_get_output_info_name(output);
			int len = xcb_randr_get_output_info_name_length(output);
		  Debug::log(LOG, "XWaylandPrimary: RANDR OUTPUT {}", (char *)output_name);
			if (!strncmp((char *)output_name, PMONITOR->szName.c_str(), len))
			{
          Debug::log(LOG, "XWaylandPrimary: setting primary monitor {}", (char *)output_name);
          xcb_void_cookie_t p_cookie = xcb_randr_set_output_primary_checked(XCBCONN, screen->root, x_outputs[i]);
          xcb_request_check(XCBCONN, p_cookie);
          if (prerenderHook)
          {
            prerenderHook = nullptr;
          }
					free(output);
					break;
			}
			free(output);
		}
    if (res_reply)
      free(res_reply);
    return;
  }
  
  
  
  void XWaylandready(wl_listener *listener, void *data) {
    setXWaylandPrimary();
  }
  
  wl_listener readyListener = {.notify = XWaylandready};
  inline CFunctionHook* pXWaylandReadyHook = nullptr;

  typedef int (*origXWaylandReady)(void *thisptr, int fd, uint32_t mask);

  int hkXWaylandReady(void *thisptr, int fd, uint32_t mask) { 
	  int retval = (*(origXWaylandReady)pXWaylandReadyHook->m_pOriginal)(thisptr, fd, mask);
    setXWaylandPrimary();
	  return retval;
  }


  void monitorEvent() {
    for(auto & m: g_pCompositor->m_vMonitors) {
      if (!m->output)
        continue;
      Debug::log(LOG, "XWaylandPrimary: MONITOR {} X {} Y {} WIDTH {} HEIGHT {}", m->szName, m->vecPosition.x, m->vecPosition.y, m->vecSize.x, m->vecSize.y);
    }
    if (g_pXWayland->pWM && g_pXWayland->pWM->connection) {
      //Xwayland may not have created the new output yet, so delay via a periodic hook until it does. 
      if (prerenderHook) {
        //If there's an existing prerender hook, cancel it.
				prerenderHook = nullptr;
      }
      prerenderHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "preRender", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::setXWaylandPrimary();});
    }
  }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;


    HyprlandAPI::addConfigValue(PHANDLE, "plugin:xwaylandprimary:display",Hyprlang::STRING{STRVAL_EMPTY});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:xwaylandprimary:followfocused",Hyprlang::INT{0});
    static auto CONFIGRELOAD = HyprlandAPI::registerCallbackDynamic(PHANDLE, "configReloaded", [&](void* self, SCallbackInfo& info, std::any data) { XwaylandPrimaryPlugin::setXWaylandPrimary();});

    HyprlandAPI::reloadConfig();

    static const auto XWAYLANDREADYMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "ready");
    for(auto & match: XWAYLANDREADYMETHODS) {
		  if (match.demangled.contains("CXWaylandServer::ready")) {  
        XwaylandPrimaryPlugin::pXWaylandReadyHook = HyprlandAPI::createFunctionHook(PHANDLE, match.address, (void*)&XwaylandPrimaryPlugin::hkXWaylandReady);
        XwaylandPrimaryPlugin::pXWaylandReadyHook->hook();
			  break;
	    }
	  }

    static auto MACB = HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorAdded", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::monitorEvent();});
    static auto MRCB = HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorRemoved", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::monitorEvent();});
    static auto FMCB = HyprlandAPI::registerCallbackDynamic(PHANDLE, "focusedMon", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::monitorEvent();});
	  
	  XwaylandPrimaryPlugin::setXWaylandPrimary();

    return {"XWayland Primary Display", "Set a configurable XWayland primary display", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}

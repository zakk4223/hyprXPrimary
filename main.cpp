#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/managers/XWaylandManager.hpp>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include "globals.hpp"

#include <unistd.h>
#include <thread>
#include <xcb/randr.h>
#include <xcb/xcb.h>

// Methods
// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}


namespace XwaylandPrimaryPlugin {


std::shared_ptr<HOOK_CALLBACK_FN> prerenderHook;

  void setXWaylandPrimary() {
    if (!g_pXWaylandManager->m_sWLRXWayland->server->client) {
      Debug::log(LOG, "XWaylandPrimary: No XWayland client");
      //There's no xwayland server, and xcb_connect seems to hang if there isn't?
      return;
    }
    static auto* const PRIMARYNAME = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:xwaylandprimary:display")->getDataStaticPtr();
    const auto PMONITOR = g_pCompositor->getMonitorFromName(*PRIMARYNAME);
    if (!PMONITOR) {
      Debug::log(LOG, "XWaylandPrimary: Could not find monitor {}", *PRIMARYNAME);
      return;
    }
  
    const auto XCBCONN = xcb_connect(g_pXWaylandManager->m_sWLRXWayland->display_name, NULL);
    const auto XCBERR = xcb_connection_has_error(XCBCONN);
    if (XCBERR) {
      return;
    }
  
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(XCBCONN)).data;
  
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
			if (!strncmp((char *)output_name, PMONITOR->szName.c_str(), len))
			{
          Debug::log(LOG, "XWaylandPrimary: setting primary monitor {}", (char *)output_name);
          xcb_void_cookie_t p_cookie = xcb_randr_set_output_primary_checked(XCBCONN, screen->root, x_outputs[i]);
          xcb_request_check(XCBCONN, p_cookie);
          if (prerenderHook)
          {
            HyprlandAPI::unregisterCallback(PHANDLE, prerenderHook);
            prerenderHook = nullptr;
          }
					free(output);
					break;
			}
			free(output);
		}
    if (res_reply)
      free(res_reply);
    xcb_disconnect(XCBCONN);
    return;
  }
  
  
  
  void XWaylandready(wl_listener *listener, void *data) {
    setXWaylandPrimary();
  }
  
  wl_listener readyListener = {.notify = XWaylandready};
  inline CFunctionHook* ploadConfigLoadVarsHook = nullptr;

  typedef void (*origloadConfigLoadVars)();

  void hkloadConfigLoadVars() { 
	  (*(origloadConfigLoadVars)ploadConfigLoadVarsHook->m_pOriginal)();
    setXWaylandPrimary();
  }


  void monitorEvent() {
    Debug::log(LOG, "XWaylandPrimary: MONITOR EVENT");
    for(auto & m: g_pCompositor->m_vMonitors) {
      if (!m->output)
        continue;
      Debug::log(LOG, "XWaylandPrimary: MONITOR {} X {} Y {} WIDTH {} HEIGHT {}", m->szName, m->vecPosition.x, m->vecPosition.y, m->vecSize.x, m->vecSize.y);
    }
    if (g_pXWaylandManager->m_sWLRXWayland->server->client) {
      //Xwayland may not have created the new output yet, so delay via a periodic hook until it does. 
      if (prerenderHook) {
        //If there's an existing prerender hook, cancel it.
        HyprlandAPI::unregisterCallback(PHANDLE, prerenderHook);
      }
      prerenderHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "preRender", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::setXWaylandPrimary();});
    }
  }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;


    HyprlandAPI::addConfigValue(PHANDLE, "plugin:xwaylandprimary:display",Hyprlang::STRING{STRVAL_EMPTY});
    HyprlandAPI::reloadConfig();

    //static const auto XWAYLANDSCALEMETHODS = HyprlandAPI::findFunctionsByName(PHANDLE, "loadConfigLoadVars");

    //XwaylandPrimaryPlugin::ploadConfigLoadVarsHook = HyprlandAPI::createFunctionHook(PHANDLE, XWAYLANDSCALEMETHODS[0].address, (void*)&XwaylandPrimaryPlugin::hkloadConfigLoadVars);
    //XwaylandPrimaryPlugin::ploadConfigLoadVarsHook->hook();

    HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorAdded", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::monitorEvent();});
    HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorRemoved", [&](void *self, SCallbackInfo&, std::any data) {XwaylandPrimaryPlugin::monitorEvent();});

    addWLSignal(&g_pXWaylandManager->m_sWLRXWayland->events.ready, &XwaylandPrimaryPlugin::readyListener, NULL, "Xwayland Primary Plugin");

    return {"XWayland Primary Display", "Set a configurable XWayland primary display", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
		wl_list_remove(&XwaylandPrimaryPlugin::readyListener.link);
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}

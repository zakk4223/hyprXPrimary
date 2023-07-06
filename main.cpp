#define WLR_USE_UNSTABLE

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
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
  void setXWaylandPrimary() {
    static SConfigValue* PRIMARYNAME = HyprlandAPI::getConfigValue(PHANDLE, "plugin:xwaylandprimary:display");
    const auto PMONITOR = g_pCompositor->getMonitorFromName(PRIMARYNAME->strValue);
    if (!PMONITOR) {
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
  
    int crtc_cnt = 0;
    xcb_randr_crtc_t *x_crtcs;
  
    crtc_cnt = xcb_randr_get_screen_resources_crtcs_length(res_reply);
    x_crtcs = xcb_randr_get_screen_resources_crtcs(res_reply);
  
    for (int i = 0; i < crtc_cnt; i++) {
      xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(XCBCONN, xcb_randr_get_crtc_info(XCBCONN, x_crtcs[i], timestamp), NULL);
      if (crtc == NULL) {
        continue;
      }
  
      if (crtc->x == PMONITOR->vecPosition.x && crtc->y == PMONITOR->vecPosition.y) {
        xcb_randr_output_t *crtc_outputs = xcb_randr_get_crtc_info_outputs(crtc);
        xcb_void_cookie_t p_cookie = xcb_randr_set_output_primary_checked(XCBCONN, screen->root, crtc_outputs[0]);
        xcb_request_check(XCBCONN, p_cookie);
        break;
        free(crtc);
      }
      free(crtc);
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
}


APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;


    HyprlandAPI::addConfigValue(PHANDLE, "plugin:xwaylandprimary:display", SConfigValue{.strValue = "TESTMONITOR"});
    HyprlandAPI::reloadConfig();

    addWLSignal(&g_pXWaylandManager->m_sWLRXWayland->events.ready, &XwaylandPrimaryPlugin::readyListener, NULL, "Xwayland Primary Plugin");

    return {"XWayland Primary Display", "Set a configurable XWayland primary display", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}

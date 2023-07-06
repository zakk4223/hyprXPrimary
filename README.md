# Primary XWayland display for Hyprland


Many XWayland programs (mostly games) do weird things in multi-monitor configurations where the monitors have different sizes.
Setting a primary XWayland monitor usually fixes them. You can do it via 'xrandr' but why not automate it as much as possible?

### How to use.
1. Build the plugin 
    - `make`
2. Load the plugin however you load other Hyprland plugins.

3. Add the following to your hyprland configuration:
    - `plugin {
            xwaylandprimary {
                    display=DP-1
                }
        }`

    - If you have an existing `plugin` section, just add the `xwaylandprimary` section to it. 
    - Use `hyprctl monitors` to get the display name





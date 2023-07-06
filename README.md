# Primary XWayland display for Hyprland


Many XWayland programs (mostly games) do weird things in multi-monitor configurations where the monitors have different sizes.
Setting a primary XWayland monitor usually fixes them. You can do it via 'xrandr' but why not automate it as much as possible?

### How to use.
1. Build the plugin 
    - `make`
2. Load the plugin however you load other Hyprland plugins.

3. Run the 'xwayland_primary.sh' script in your Hyprland session, give it a single command line argument: the name of the primary display.
    - `./xwayland_primary.sh DP-1`
    - `hyprctl monitors` to figure out the monitor name.

The script will set the primary, and then listen via Hyprland IPC for an event that indicates it needs to possibly reflag the primary.



### Extra details

The plugin hooks the `setXWaylandScale` function in Hyprland and does two things:
1. It sets the XWayland monitor names to the same as the Hyprland ones. 
2. It emits an 'xwaylandupdate' event over IPC.

The script just listens for this event and executes `xrandr --output $1 --primary`

### Future work.

The plugin could likely set primary display itself via xcb_randr or whatever, but I was too lazy to do that. Maybe later.


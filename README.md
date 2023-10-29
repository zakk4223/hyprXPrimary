# Primary XWayland display for Hyprland


Many XWayland programs (mostly games) do weird things in multi-monitor configurations where the monitors have different sizes.
Setting a primary XWayland monitor usually fixes them. You can do it via 'xrandr' but why not automate it as much as possible?

## Using [hyprload](https://github.com/Duckonaut/hyprload)
Add the line `"XorvatoAmigos/hyprXPrimary",` to your `hyprload.toml` config, like this

```toml
plugins = [
    "XorvatoAmigos/hyprXPrimary",
]
```

Then update via the `hyprload,update` dispatcher.

## Manual installation

1. Build the plugin 
    - `make all`
2. Load the plugin however you load other Hyprland plugins.
    - `exec-once = hyprctl plugin load <ABSOLUTE PATH TO hyprXPrimary.so>`

## Customization

1. Get the name of the monitor you want to make primary:
   
```
hyprctl monitors
```

2. Add the following to your hyprland configuration:

    Replace `DP-1` with the name you want.

```
plugin {
    xwaylandprimary {
        display = DP-1
    }
}
```

3. Restart the hyprland configuration.
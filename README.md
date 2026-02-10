# hyprgollum

Experimental automatic column / n-stack layout plugin for Hyprland 0.54+.
Designed for ultrawide and portrait screens with simplicity in mind,
so windows are always autosized and the options are few, but powerful.

## Config

```ini
general:layout = gollum
plugin {
    gollum {
        grid = 3 2         # set ideal grid size COLS ROWS
        grow = yes         # fill holes by making earlier windows bigger instead of equal split
        manual = 11223344  # repeating pattern of column numbers to follow instead of default pattern
    }
}

# all options can also be used with layoutopt workspace rules
workspace = 2, layout:gollum, layoutopt:grid:1 1
workspace = 3, layout:gollum, layoutopt:manual:1234, layoutopt:grow:0

# all options can also be used with layoutmsg binds: set, unset, toggle, cycle, reset
bind = SUPER, z, layoutmsg, set grid 1 1
bind = SUPER, x, layoutmsg, toggle grid 2 2
bind = SUPER, c, layoutmsg, reset
bind = SUPER, v, layoutmsg, cycle grid 1 1,2 1,3 2,4 2
bind = SUPER, b, layoutmsg, cycle grow 0,1

# layoutmsg overrides layoutopt, which overrides global setting
```

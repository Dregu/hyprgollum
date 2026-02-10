# hyprgollum

Experimental automatic column / n-stack layout plugin for Hyprland 0.54+.
Designed for ultrawide and portrait screens with simplicity in mind,
so windows are always autosized and the options are few, but powerful.

## Config

```ini
general:layout = gollum

plugin {
    gollum {
        grid = 3 2       # set ideal grid size COLS ROWS (default 0 0, i.e. single column)
        fit = 2          # 0=center when missing columns, 1=fill evenly, 2=grow left window (default 0)
        order = 11223344 # repeating column numbers to follow instead of default pattern
        dir = r          # horizontal orientation l/r (default l)
    }
}

# all options can also be used with layoutopt workspace rules
# layoutopt overrides global settings
workspace = 2, layout:gollum, layoutopt:grid:1 1
workspace = 3, layout:gollum, layoutopt:order:1234, layoutopt:fit:0

# all options can also be used with layoutmsg binds: set, unset, toggle, cycle, reset
# layoutmsg overrides layoutopt
bind = SUPER, z, layoutmsg, reset
bind = SUPER, x, layoutmsg, set grid 1 1
bind = SUPER, c, layoutmsg, toggle grid 2 2
bind = SUPER, v, layoutmsg, cycle grid 1 1,2 1,3 2,4 2
bind = SUPER, b, layoutmsg, cycle fit 0,1,2

# these dispatchers mimic (promoting to) master/slave on some grid setups
bind = SUPER,       t, layoutmsg, focus, top
bind = SUPER,       g, layoutmsg, focus, bottom
bind = SUPER SHIFT, t, layoutmsg, move,  top
bind = SUPER SHIFT, g, layoutmsg, move,  bottom
bind = SUPER CTRL,  t, layoutmsg, swap,  top
bind = SUPER CTRL,  g, layoutmsg, swap,  bottom
```

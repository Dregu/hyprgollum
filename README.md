# hyprgollum

Experimental automatic column / n-stack layout plugin for Hyprland 0.54+.
Designed for ultrawide and portrait screens with simplicity in mind,
so windows are always autosized and the options are few, but powerful.

## Config

```ini
general:layout = gollum

plugin {
    gollum {
        grid  = 3 2  # set ideal grid size, COLS ROWS (default 0 0, i.e. single column)
        fit   = 2    # 0=center when missing columns, 1=fill evenly, 2=grow first window (default 0)
        dir   = r    # horizontal orientation, left/right (default l)
        new   = top  # new window position, top/bottom/next/prev/smart (default b)
        order = 1234 # repeating column numbers to follow instead of default master-ish logic
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
bind = SUPER,            t, layoutmsg, focuswindow, top
bind = SUPER,            g, layoutmsg, focus,       bottom
bind = SUPER SHIFT,      t, layoutmsg, movewindow,  top
bind = SUPER SHIFT,      g, layoutmsg, move,        bot
bind = SUPER CTRL,       t, layoutmsg, swapwindow,  t
bind = SUPER CTRL,       g, layoutmsg, swap,        b
bind = SUPER SHIFT CTRL, t, layoutmsg, nextwindow,  t      # oneshot new for next opened window
bind = SUPER SHIFT CTRL, g, layoutmsg, next,        b

# you can also use top/bottom tags to preset placement for specific apps
# if there are already any tagged windows on top, opens after those
# tag overrides new, but nextwindow overrides tag
windowrule = match:class firefox|codium, tag +top
bind = SUPER, Return, exec, [tag +bottom]foot
```

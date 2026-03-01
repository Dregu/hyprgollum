# hyprgollum

Experimental automatic column / n-stack layout plugin for Hyprland 0.54+.
Designed for ultrawide and portrait screens with simplicity in mind,
so windows are always autosized and the options are few, but powerful.

- By default it just creates a single column with all windows split evenly, great for portrait monitors.
- The **grid** logic with >1 columns can roughly mimic **master** or **hyprNStack** layouts, prioritizing space for the **top** window until the ideal grid is too full, after which it will also be split automatically **once**.
- The **order** logic can be used to for a more traditional left-to-right window order, **center master** or something even more confusing.

https://github.com/user-attachments/assets/f3e780ff-6ced-4fa9-a88b-df363df469a7

## Config

### Variables

Category `plugin:gollum:`. All global settings can also be set with `layoutopt` and `layoutmsg` to override them in more specific scenarios. See [examples](#examples).

| name | description | type | default
| --- | --- | --- | --- |
| grid | Ideal grid size, defines the max number of columns, the max number of rows in the **top** column, and the ideal number of rows in a **bottom** column before a new column is created. If `#windows > x * y`, extra windows are arranged evenly across **bottom** columns. | vec2 | 1 1 |
| fit | How to fit columns in the workarea if `#columns < grid.x`. `center` to center small columns, `fill` to fit columns evenly, `top` to grow more important windows to fill gaps, `no` to just leave gaps. | str | top |
| dir | Direction/orientation, the side where the **top** window is. One of: `left`, `right` | str | left |
| new | Position in the stack for new windows. One of: `top` of the stack, `bottom` of the stack, `next` after focused, `prev` before focused, `smart` at cursor. | str | bottom |
| order | Repeating pattern of column numbers `0..9` (0-indexed) to place new windows in the order of, overriding **grid**. Gaps between columns are always filled from the top side, even if **fit** is disabled. See [cool examples](#examples). | str |  |

### Dispatchers

Most standard Hyprland dispatchers work. Adds the following `layoutmsg` dispatchers.

| name | description | params |
| --- | --- | --- |
| reset | Resets all settings to the globals. |  |
| set | Temporarily override any global setting listed in variables with **value**. | \<name> \<value> |
| unset | Restore one setting to the workspace rule or global variable. | \<name> |
| toggle | Toggle between **value** and global setting. | \<name> \<value> |
| cycle | Cycle setting between comma separated **value**s. | \<name> \<value>,\<value>,... |
| focuswindow | Focus a specific window. | `top` or `bottom` |
| movewindow | Move active window to specific position in stack. | `top` or `bottom` |
| swapwindow | Swap active window with window in specific position in stack. | `top` or `bottom` |
| nextwindow | Preselect position once for next window that opens. Overrides **tag**. | See [new](#variables) |

### Window rules

New window position can be overridden by adding the dynamic tag `top` or `bottom`. If there are already any tagged windows on top, opens after those.

### Syntax

- Most options (not tags) can be shortened to their shortest unique prefix, e.g. `b`~~ottom~~, `r`~~ight~~
- Dispatchers can be shortened to `move`~~window~~
- Precedence for the settings is roughly `nextwindow > tag > layoutmsg > layoutopt > global variable`

### Examples

```ini
general:layout = gollum

plugin {
    gollum {
        grid  = 2 2
        fit   = c
        dir   = r
        new   = top
        order = 0123
    }
}

# all options can also be used with layoutopt workspace rules
workspace = m[HDMI-A-2], layout:gollum, layoutopt:grid:1 1
workspace = 3, layout:gollum, layoutopt:order:0123, layoutopt:fit:center

# all options can also be used with layoutmsg binds: set, unset, toggle, cycle, reset
bind = SUPER, Z, layoutmsg, reset
bind = SUPER, X, layoutmsg, set grid 1 1
bind = SUPER, C, layoutmsg, toggle grid 2 2
bind = SUPER, V, layoutmsg, cycle grid 1 1,2 1,3 2,4 2
bind = SUPER, B, layoutmsg, cycle fit f,c

# these dispatchers can mimic promoting to master on some grid setups
bind = SUPER,            T, layoutmsg, focuswindow, top
bind = SUPER,            G, layoutmsg, focus,       bottom
bind = SUPER SHIFT,      T, layoutmsg, movewindow,  top
bind = SUPER SHIFT,      G, layoutmsg, move,        bot
bind = SUPER CTRL,       T, layoutmsg, swapwindow,  t
bind = SUPER CTRL,       G, layoutmsg, swap,        b
bind = SUPER SHIFT CTRL, T, layoutmsg, nextwindow,  t
bind = SUPER SHIFT CTRL, G, layoutmsg, next,        b

# use top/bottom tags to preset placement for specific apps
windowrule = match:class firefox|codium, tag +top
bind = SUPER, Return, exec, [tag +bottom]foot

# dev special, some stuff I actually use
workspace = m[DP-1], layout:gollum, layoutopt:grid:3 2 # ideal layout for 21:9 3440x1440
workspace = m[DP-1] w[tv1], gapsout:24 596 24 595      # center single window keeping its size uniform with two columns
workspace = m[DP-1] f[1], gapsout:24 596 24 595        # (when default gaps are 24/12)
workspace = m[DP-2], layout:gollum, layoutopt:grid:1 1 # portrait ultrawide of course has single column
workspace = m[DP-2] w[tv1], gapsout:668 24 648 24      # with the same centering trick for single window
bind = SUPER, X, layoutmsg, toggle grid 3 1 # switch between 3 equal columns and top that covers 2/3 columns
bind = SUPER, C, layoutmsg, cycle fit f,t   # switch between 2 equal columns and ditto
bind = SUPER, V, layoutmsg, toggle dir r    # swap top to right

# cool examples
workspace = 4, layoutopt:order:1303030, layoutopt:fit:n # like center master with always_keep_position
workspace = 5, layoutopt:order:1330003, layoutopt:fit:f # like center master but expands top window both ways too

# silly stuff that nobody should probably use
workspace = 10, layoutopt:fit:f, layoutopt:order:01234567899876543210 # just keep making columns until there are way too many, then do a U-turn
```

## Contributing

Meh, probably won't care about your issues unless they affect me. Probably won't merge PR unless it's something I didn't realize I needed. Feel free to fork and make it your own though.

## (Not) TODO

- [ ] Manual resizing probably won't be added, maybe for height very temporarily
- [ ] Moving windows around the columns unevenly is not a goal either, it's automatic
- [ ] The mfact option from master maybe one day

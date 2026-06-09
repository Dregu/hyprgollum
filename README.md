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

Category `plugin.gollum.`. All global settings can also be set with `layout_opts` and `hl.dsp.layout` to override them in more specific scenarios. See [examples](#examples).

| name | description | type | default
| --- | --- | --- | --- |
| grid | Ideal grid size, defines the max number of columns, the max number of rows in the **top** column, and the ideal number of rows in a **bottom** column before a new column is created. If `#windows > x * y`, extra windows are arranged evenly across **bottom** columns. | vec2 | `1 1` |
| fit | How to fit columns in the workarea if `#columns < grid.x`. One of: <ul><li>`center` to center small columns</li><li>`fill` to fit columns evenly</li><li>`top` to grow more important windows to fill gaps</li><li>`no` to just leave gaps</li></ul> | str | `top` |
| dir | Direction/orientation, the side where the **top** window is. One of: `left`, `right` | str | `left` |
| new | Position in the stack for new windows. One of: <ul><li>`top` of the stack</li><li>`bottom` of the stack</li><li>`next` after focused</li><li>`prev` before focused</li><li>`smart` at cursor</li></ul> | str | `bottom` |
| order | Repeating pattern of column numbers `0..9` (0-indexed) to place new windows in the order of, overriding **grid**. Gaps between columns are always filled from the top side, even if **fit** is disabled. See [cool examples](#examples). | str |  |
| wrap | Enables wrapping around the end of the stack on certain dispatchers. | int | `0` |
| fs | Remove fullscreen node from the layout stack. One of: `0` don't, `1` always, `2` only if created fullscreen or tagged `fs` | int | `2` |
| mono | Treat this fullscreen mode as monocle, i.e. maximize all nodes, so cycling doesn't cause resizes. One of: `0` none, `1` maximize, `2` fullscreen, `3` both | int | `1` |

### Dispatchers

Most standard Hyprland dispatchers work. Adds the following `hl.dsp.layout` dispatchers.

| name | description | params |
| --- | --- | --- |
| reset | Resets all settings to the globals. |  |
| set | Temporarily override any global setting listed in variables with **value**. | `<name> <value>` |
| unset | Restore one setting to the workspace rule or global variable. | `<name>` |
| toggle | Toggle between **value** and global setting. | `<name> <value>` |
| cycle | Cycle setting between comma separated **value**s. | `<name> <value>,<value>,...` |
| focus | Focus a specific window in the stack. | `top` `bottom` `next` `prev` |
| move | Move active window to specific position in stack. | `top` `bottom` `next` `prev` |
| swap | Swap active window with window in specific position in stack. | `top` `bottom` `next` `prev` |
| roll | Rotates the stack keeping the focus in the same **position**. | `next` `prev` |
| next | Preselect position once for next window that opens. Overrides **tag**. | See [new](#variables) |

### Window rules

New window position can be overridden by adding the dynamic tag `top` or `bottom`. If there are already any tagged windows on top, opens after those.

### Syntax

- Most options (not tags) can be shortened to their shortest unique prefix, e.g. `b`~~ottom~~, `r`~~ight~~
- Precedence for the settings is roughly `nextwindow > tag > layoutmsg > layoutopt > global variable`

### Examples

```lua
hl.config{
    general = {
        layout = 'gollum'
    },
    plugin = {
        gollum = {
            grid  = { 2, 2 },
            fit   = 'c',
            dir   = 'r',
            new   = 'top',
            order = '0123',
            wrap  = 1,
            fs    = 2,
            mono  = 3,
        }
    }
}

-- all options can also be used with layoutopt workspace rules
hl.workspace_rule{ workspace = 'm[HDMI-A-2]', layout = 'gollum', layout_opts = { grid = '1 1' } }
hl.workspace_rule{ workspace = 3, layout = 'gollum', layout_opts = { order = '0123', fit = 'center' } }

-- all options can also be used with layoutmsg binds: set, unset, toggle, cycle, reset
hl.bind('SUPER+Z', hl.dsp.layout('reset'))
hl.bind('SUPER+X', hl.dsp.layout('set grid 1 1'))
hl.bind('SUPER+C', hl.dsp.layout('toggle grid 2 2'))
hl.bind('SUPER+V', hl.dsp.layout('cycle grid 1 1,2 1,3 2,4 2'))
hl.bind('SUPER+B', hl.dsp.layout('cycle fit f,c'))

-- these dispatchers can mimic promoting to master on some grid setups
hl.bind('SUPER+T', hl.dsp.layout('focus top'))
hl.bind('SUPER+G', hl.dsp.layout('focus bottom'))
hl.bind('SUPER+SHIFT+T', hl.dsp.layout('move top'))
hl.bind('SUPER+SHIFT+G', hl.dsp.layout('move bot'))
hl.bind('SUPER+CTRL+T', hl.dsp.layout('swap t'))
hl.bind('SUPER+CTRL+G', hl.dsp.layout('swap b'))
hl.bind('SUPER+SHIFT+CTRL+T', hl.dsp.layout('next t'))
hl.bind('SUPER+SHIFT+CTRL+G', hl.dsp.layout('next b'))

-- use top/bottom tags to preset placement for specific apps
hl.window_rule{ match = { class = 'firefox|codium' }, tag = '+top' }
hl.bind('SUPER+Return' hl.dsp.exec_cmd('foot', { tag = '+bottom' }))

-- hide some normally fullscreen windows from the tiled layout stack
hl.window_rule{ match = { class = 'mpv' }, fullscreen = true }
hl.window_rule{ match = { class = 'vlc' }, tag = '+fs' }

-- dev special, some stuff I actually use
hl.workspace_rule{ workspace = 'm[DP-1]', layout = 'gollum', layout_opts = { grid = '3 2' } } -- ideal layout for 21:9 3440x1440
hl.workspace{ workspace = 'm[DP-1] w[tv1]', gaps_out = { 24, 596, 24, 595 } }     -- center single window keeping its size uniform with two columns
hl.workspace{ workspace = 'm[DP-1] f[1]', gaps_out = { 24, 596, 24, 595 } }       -- (when default gaps are 24/12)
hl.bind('SUPER+X', hl.dsp.layout('toggle grid 3 1')) -- switch between 3 equal columns and top that covers 2/3 columns
hl.bind('SUPER+C', hl.dsp.layout('cycle fit f,t'))   -- switch between 2 equal columns and ditto
hl.bind('SUPER+V', hl.dsp.layout('toggle dir r'))    -- swap top to right

-- cool examples
hl.workspace_rule{ workspace = 4, layout_opts = { order = '1303030', fit = 'n' } } -- like center master with always_keep_position
hl.workspace_rule{ workspace = 5, layout_opts = { order = '1330003', fit = 'f' } } -- like center master but expands top window both ways too
```

## Contributing

Meh, probably won't care about your issues unless they affect me. Probably won't merge PR unless it's something I didn't realize I needed. Feel free to fork and make it your own though.

## (Not) TODO

- [ ] Manual resizing probably won't be added, maybe for height very temporarily
- [ ] Moving windows around the columns unevenly is not a goal either, it's automatic
- [ ] The mfact option from master maybe one day

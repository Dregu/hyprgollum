# hyprgollum

Experimental automatic column / n-stack layout plugin for Hyprland.
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

# all options can be used with workspace rules:
workspace = 2, layout:gollum, layoutopt:grid:1 1
workspace = 3, layout:gollum, layoutopt:manual:1234, layoutopt:grow:0
```

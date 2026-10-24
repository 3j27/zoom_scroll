# Control + two-finger scroll zoom

A native Hyprland plugin for whole-screen magnification. Hold
Control alone and scroll vertically with two fingers. Zoom direction follows
Hyprland's `input:touchpad:natural_scroll` setting, including live changes.
With natural scrolling enabled, positive scroll deltas zoom in; otherwise,
negative deltas zoom in. Ordinary scrolling, horizontal scrolling, mouse wheels,
and other modifier combinations pass through. Locked sessions and active input
capture are skipped.

## Install with hyprpm

Requires Hyprland 0.56.2 (the tested version), `hyprpm`, a C++23 compiler,
make, pkgconf, and libei development files. On Omarchy, install the manager
with `omarchy pkg add hyprpm` if it is missing.

```sh
hyprpm add https://github.com/3j27/zoom_scroll
hyprpm enable zoom-scroll
hyprpm reload
hyprctl plugin list
hyprctl --batch 'zoomscroll selftest'
```

For startup loading on Omarchy, add to `~/.config/hypr/autostart.lua`:

```lua
o.launch_on_start("hyprpm reload")
```

Use `hyprpm update` after Hyprland upgrades, then `hyprpm reload`.
New Hyprland versions may require source changes; compatibility beyond the
tested version is not guaranteed. To disable, run `hyprpm disable zoom-scroll`
and `hyprpm reload`. Remove the repository with `hyprpm remove zoom-scroll`.

If migrating from the manual installation below, remove its `dofile(...)`
line from `hyprland.lua` and unload the old library before enabling the
hyprpm copy. Keep only one loading method to avoid handling gestures twice.
Plugin settings can still be set with `hl.config(...)` after the plugin loads.

## Build manually

Requires a C++23 compiler, make, pkgconf, and the development files for the
running Hyprland and libei. On this system those come with `gcc`, `make`,
`pkgconf`, `hyprland`, and `libei`.

```sh
cd ~/zoom_scroll
make -B all check
```

The plugin checks Hyprland's ABI hash before initializing. Rebuild after
Hyprland upgrades; use matching headers and a compiler compatible with the
compositor. Generated libraries and test executables are ignored by Git.
The library is replaced atomically and built without GNU unique symbols so
it can be unloaded and reloaded.

## Load

For a trial from a terminal in the desktop:

```sh
hyprctl plugin load "$HOME/zoom_scroll/zoom-scroll.so"
hyprctl plugin list
hyprctl --batch 'zoomscroll selftest'
```

For automatic loading, add this line to `~/.config/hypr/hyprland.lua`:

```lua
dofile(os.getenv("HOME") .. "/zoom_scroll/zoom-scroll.lua")
```

Then run `hyprctl reload` and `hyprctl configerrors`. This integration assumes
that the checkout stays at `~/zoom_scroll`. Keep the compiled library there.
The self-test briefly changes zoom, exercises the plugin's scroll handler,
and restores the original value; it does not simulate physical trackpad input.

When using a shell outside the desktop, select the session shown by
`hyprctl instances` with `hyprctl -i INSTANCE ...`.

## Settings and diagnostics

Default sensitivity is 0.01; maximum zoom is 8 and minimum is 1.
Sensitivity accepts -0.1 to 0.1; a negative value reverses direction. Zero
consumes the gesture without changing zoom. Maximum zoom accepts 1 to 20.

```sh
hyprctl eval 'hl.config({plugin = {zoom_scroll = {sensitivity = 0.02, max_zoom = 4}}})'
hyprctl --batch 'zoomscroll'
```

To persist settings, add the corresponding `hl.config(...)` call to
`zoom-scroll.lua` after loading the plugin. Diagnostics report event and
consumed counts, the last modifiers/delta, current zoom, and last update reply.
Control is modifier mask 4; Command/SUPER is 64.

To rebuild a loaded plugin, unload it, run `make -B all check`, and reload
the plugin explicitly with `hyprctl plugin load "$HOME/zoom_scroll/zoom-scroll.so"`.
Hyprland caches its configured plugin list, so reloading an unchanged config
after a manual unload does not load the library again. Unloading preserves
current magnification. To disable it,
remove the `dofile` line first, then unload and reset:

```sh
hyprctl plugin unload "$HOME/zoom_scroll/zoom-scroll.so"
hyprctl eval 'hl.config({cursor = {zoom_factor = 1}})'
```

## System validation (2026-10-22)

Built on aarch64 against Hyprland 0.56.2-7 with GCC 16.1.1. The running
compositor is v0.56.2, commit `efb50993780079460b0cbed1363e2166a2de1d9f`.
The build and unit tests pass. The rebuilt plugin loads into the live
compositor and its self-test passes: synthetic Control + finger scroll changes
zoom from 2 to 2.210342; ordinary and wheel scroll pass through; original zoom
is restored. The Lua configuration is installed in this user's Hyprland config,
and reloading reports no configuration errors.

Physical gesture feel, modifier changes mid-gesture, and lock/input-capture
behavior still require hands-on validation. Try ordinary scrolling, Control +
scroll both ways, horizontal scrolling, limits, and releasing Control during a
scroll. Existing keyboard zoom/reset shortcuts remain available.

See [investigation.md](investigation.md) for the original diagnosis: Hyprland
mouse-wheel bindings do not match finger-source axis events, and Lua does not
expose the cancellable axis signal used by this plugin.

## See also

This plugin stays focused on Control + two-finger scrolling to control
Hyprland's existing screen magnification while consuming the gesture. These
related projects offer other approaches or broader accessibility features:

- [Omarchy Magnifier](https://github.com/JPDoesDev/omarchy-magnifier): zoom
  through keys, scroll wheel, or touchpad pinch, with a zoom popup, colour
  inversion, and multi-monitor features.
- [KDE Plasma accessibility zoom](https://docs.kde.org/stable_kf6/en/plasma-desktop/kcontrol/kcmaccess/):
  modifier + two-finger scroll desktop magnification on Wayland, built into
  a different desktop environment.
- [hypr-canvas](https://github.com/aaronsb/hypr-canvas): a Hyprland plugin
  offering Super + scroll zoom as part of an infinite desktop canvas.

These are references and alternatives, not dependencies. Their compatibility
with this system and support for this exact gesture have not been tested.

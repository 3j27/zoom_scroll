# Investigation: SUPER + two-finger scroll zoom doesn't work (cmd+scroll equivalent)

## Problem
User on Omarchy (Apple Silicon / Asahi, touchpad `apple-mtp-multi-touch`) wants 
Cmd + two-finger scroll = zoom. Keyboard shortcuts work (SUPER+CTRL+Z zoom in, SUPER+CTRL+ALT+Z
reset), but SUPER + two-finger scroll does nothing. Existing binds in
`~/.config/hypr/bindings.lua` (lines ~52–63, `command-scroll-zoom` block) use
`o.rebind("SUPER + mouse_up"/"mouse_down", ...)` modifying `cursor.zoom_factor`
(+/- 0.25, clamp [1, 8]).

## Verified state
- Hyprland 0.56.2. Binds ARE registered (checked via `hyprctl binds`): modmask 64 + mouse_up
  = "Zoom in" (`__lua` dispatcher); modmask 72 (SUPER+CTRL) + mouse_up/down = window group
  prev/next (stock). So registration is fine.
- `input:emulate_discrete_scroll` = 1 (default, on) — not the blocker.
- Zoom rendering itself works via `cursor:zoom_factor` (no separate `cursor:zoom` option in
  0.56.2). `hyprctl getoption input:emulate_discrete_scroll -j` → 1;
  `binds:scroll_event_delay` → 300.

## Root cause (confirmed from Hyprland 0.56.2 source)
`src/managers/KeybindManager.cpp` → `CKeybindManager::onAxisEvent` (line 446):

```cpp
if (e.source == WL_POINTER_AXIS_SOURCE_WHEEL && e.axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    // mouse_up / mouse_down binds handled ONLY here
}
```

Binds named `mouse_up`/`mouse_down` fire **only for wheel-source axis events**
(physical mouse wheel). Touchpad two-finger scroll emits
`WL_POINTER_AXIS_SOURCE_FINGER`, so `onAxisEvent` never matches → bind never fires,
scroll passes through to the focused window. This is a compositor limitation, not a
config error. The SUPER+CTRL window-group wheel binds are equally affected.

Also checked: `InputManager::onMouseWheel` emits `Event::bus()->m_events.input.mouse.axis`
(line ~987) BEFORE the wheel-source check — but this event is NOT reachable from Lua:
`src/config/lua/LuaEventHandler.cpp` constructor registers listeners for
window/layer/monitor/workspace/config/keybinds/screenshare/start/exit and
`input.keyboard.key` ONLY. `input.mouse.axis` is not dispatched to `hl.on()` in 0.56.2.
(Custom events from plugins ARE supported via `addCustomEvent`.)

## Dead ends confirmed
- `hl.on("input.mouse.axis", ...)` — not a known Lua event in 0.56.2.
- `emulate_discrete_scroll` — on already; affects discrete value120 synthesis, not bind source.
- Touchpad gestures: Hyprland gestures = libinput swipes, 3+ fingers only; 2-finger is scroll.
- libinput scroll_method: no mode makes a touchpad emit wheel-source events.
- `hl.bind` device-scoping (`devices`, `device_inclusive`) — scopes which device matches a
  bind, doesn't change axis-source matching.

## Possible paths forward
1. **Check newer Hyprland**: upstream may have added finger-source axis binds. (Checked
   `main` branch KeybindManager.cpp — fetch returned empty, likely path change; need to
   re-check. GitHub issue search for "touchpad mouse_up bind" didn't surface a clear fix.)
2. **Hyprland plugin** (C++): hook `onMouseWheel` / listen to `m_events.input.mouse.axis`
   (Cancellable) and adjust `cursor.zoom_factor` when SUPER held; or register a custom
   event that Lua can consume (`addCustomEvent`). Heavyweight.
3. **External evdev daemon**: read touchpad events from `/dev/input` (needs `input` group),
   detect two-finger scroll + SUPER held, call zoom. Janky, duplicates libinput.
4. **Pragmatic alternative**: keep existing SUPER+CTRL+Z zoom steps, or add 3-finger
   vertical gestures via `hl.gesture` (works on this touchpad — see `input.lua` examples)
   for zoom in/out.

## Useful facts / environment notes
- `hyprctl` on this system is a Lua shim; plain queries (`binds`, `getoption`, `version`)
  work, but `dispatch` with positional args errors. Real binary path unclear; use plain
  `hyprctl` for queries.
- Omarchy Lua layer: `/usr/share/omarchy/default/hypr/helpers.lua` — `o.bind`/`o.rebind`
  wrap `hl.bind`; rebind = `hl.unbind(keys)` + bind. `o.rebind` for re-binding existing keys.
- API stubs: `/usr/share/hypr/stubs/hl.meta.lua`. Hyprland headers with Lua/event APIs:
  `/usr/include/hyprland/src/config/lua/`, `/usr/include/hyprland/src/event/EventBus.hpp`.
- User config: `~/.config/hypr/input.lua` has Apple touchpad setup (natural scroll,
  scroll_factor 0.4, 3/4-finger gestures already configured via `hl.gesture`).
- Internet access works (GitHub fetchable via curl).
- After any config change: `hyprctl reload` + `hyprctl configerrors`; use `omarchy menu
  keybindings --print` before re-binding.

## Next steps
1. Re-check upstream Hyprland (main / releases > 0.56.2) for touchpad axis-bind support
   (find new source path; tree API worked: `https://api.github.com/repos/hyprwm/Hyprland/git/trees/main?recursive=1`).
2. If no upstream fix: present options to user (plugin vs. alternative gesture/keys).

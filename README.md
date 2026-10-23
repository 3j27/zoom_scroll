# Command + two-finger scroll accessibility zoom

## Built plugin

`zoom-scroll.so` is built locally against Hyprland 0.56.2-4 with GCC 16.1.1,
the same compiler recorded in the installed compositor. It refuses to initialize
if Hyprland's API hash differs from the headers used to build it.

Build and test:

```sh
cd ~/zoom_scroll
make
make check
```

From a terminal in the running desktop, load for a trial:

```sh
hyprctl plugin load "$HOME/zoom_scroll/zoom-scroll.so"
hyprctl plugin list
```

Hold Command (SUPER) alone and scroll vertically with two fingers. Negative
scroll deltas zoom in; positive deltas zoom out. Natural-scroll settings may
affect which physical direction this corresponds to. Normal scrolling,
horizontal scrolling, mouse-wheel scrolling, and other modifier combinations
pass through. The plugin skips locked sessions and active input capture.
Zoom defaults to a maximum of 8; the minimum is 1.

Adjust speed, reverse direction, or change the maximum during the trial:

```sh
hyprctl eval 'hl.config({plugin = {zoom_scroll = {sensitivity = 0.02}}})' 
hyprctl eval 'hl.config({plugin = {zoom_scroll = {sensitivity = -0.01}}})' 
hyprctl eval 'hl.config({plugin = {zoom_scroll = {max_zoom = 4}}})' 
```

Default sensitivity is 0.01. Zero disables zoom changes while still consuming
Command + vertical finger scroll. Sensitivity range is -0.1 to 0.1; maximum
range is 1 to 20. The existing keyboard zoom/reset shortcuts remain available.

Unload and reset:

```sh
hyprctl plugin unload "$HOME/zoom_scroll/zoom-scroll.so"
hyprctl eval 'hl.config({cursor = {zoom_factor = 1}})'
```

Unloading disconnects the listener; it preserves the current magnification.
No autoload configuration has been installed. Rebuild after a Hyprland upgrade.

Validation: `make -B all check` passed, and the library exports all three plugin
entry points. Live access was restored and the plugin loaded successfully on 2026-10-11.
Hyprland lists zoom-scroll 0.1.0, both configuration defaults are registered,
and configerrors is empty. Gesture feel, event delivery after modifier release,
lock behavior, and unload still need the interactive checks listed below.

## Goal
Provide whole-screen magnification: hold physical Command (Hyprland SUPER) and scroll with two fingers. Consume the scroll while zooming, leave ordinary scrolling alone.

## Recovered Pi session
`.pi/agent/sessions/--home-admin--/2026-10-12T02-10-53-139Z_01a12de0-8c53-75f1-a361-36a6587a2139.jsonl`
The final request was to save findings here, but Pi stopped before creating this directory.

## Confirmed findings
- User reported Command+Control+Z zoom works and Command+Control+Option+Z resets it.
- Installed package: hyprland 0.56.2-4.
- ~/.config/hypr/bindings.lua already replaces SUPER+mouse_up/down workspace switching with cursor.zoom_factor changes, in increments of 0.25, clamped to 1–8. This cannot handle touchpad scrolling in this version.
- In v0.56.2 CKeybindManager::onAxisEvent, mouse_up/down bindings require WL_POINTER_AXIS_SOURCE_WHEEL. Touchpad two-finger scrolling has SOURCE_FINGER.
- The internal Event::bus()->m_events.input.mouse.axis signal is cancellable (verified in installed EventBus.hpp).
- Lua hl.on does not expose that signal: only input.keyboard.key is available in the input namespace. Verified both installed hl.meta.lua and versioned LuaEventHandler.cpp.
- Changing scroll delay or adding another modifier does not address the source check.

## Continuation findings (Codex)
Rechecked versioned upstream sources and installed headers. A native Hyprland plugin could listen to the cancellable axis signal, check g_pInputManager->getModsFromAllKBs(), change the zoom factor, and consume SUPER+finger-scroll events. This is a proposed approach, not an implemented or tested fix. It must respect lock/input capture and use headers matching the running compositor.
Fetching the guessed upstream main KeybindManager.cpp URL returned 404; no conclusion about newer releases can be drawn from that.
Live hyprctl access currently fails with "Couldn't set socket timeout (2)" despite HYPRLAND_INSTANCE_SIGNATURE being set and the instance directory existing. Runtime validation is therefore unavailable from this agent environment. No desktop configuration was changed.

## Next work
The native prototype is now implemented and compiled. The axis callback runs
before InputManager forwards events to input capture, so it explicitly checks
capture state before consuming anything.

Once live compositor access works, test: ordinary scrolling; Command-scroll
both ways; zoom limits/reset; horizontal scrolling; modifier release mid-scroll;
screen lock/input capture; plugin unload. If a scroll begins in an application
and Command is pressed or released mid-scroll, verify the application receives
a usable stop event and subsequent ordinary scrolling still works. No runtime
success should be claimed without an interactive trackpad test.

## References
- https://raw.githubusercontent.com/hyprwm/Hyprland/v0.56.2/src/managers/KeybindManager.cpp
- https://raw.githubusercontent.com/hyprwm/Hyprland/v0.56.2/src/config/lua/LuaEventHandler.cpp
- /usr/include/hyprland/src/event/EventBus.hpp
- /usr/share/hypr/stubs/hl.meta.lua

## Runtime fix and diagnostics (0.1.1)

The initial plugin consumed scroll but used the legacy `keyword` command,
which this Lua configuration rejects. Version 0.1.1 uses `eval` with
`hl.config`, and passes the event through if the update returns an error.

Live self-test passed: synthetic Command + finger-scroll changed the actual
compositor zoom from 2 to 2.210342; ordinary and wheel events passed through;
original zoom was restored. This exercises the shared scroll handler with
synthetic modifiers; a physical trackpad gesture still needs user verification.

```sh
hyprctl --batch 'zoomscroll'
hyprctl --batch 'zoomscroll selftest'
```

Diagnostics record event count, consumed count, last modifier mask, last delta,
current zoom, and last update reply. Control is mask 4, Command/SUPER is 64.
The plugin still targets Command, matching the original request.

The original library used GNU unique symbols, which can prevent full unloading
and cause the loader to reuse old code. The fixed Makefile disables these and
builds to a temporary file before replacing the library. For this session the
fixed plugin was loaded under `zoom-scroll-fixed.so` to bypass the cached old
library. Unload that exact path when testing unload.

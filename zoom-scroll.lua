-- Load from ~/.config/hypr/hyprland.lua after building ~/zoom_scroll.
-- Hyprland checks the plugin ABI; rebuild after compositor upgrades.
hl.plugin.load(os.getenv("HOME") .. "/zoom_scroll/zoom-scroll.so")

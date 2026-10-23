#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/protocols/InputCapture.hpp>
#include <format>
#include <stdexcept>
#include "zoom.hpp"

namespace {
CHyprSignalListener axisListener;
SP<Config::Values::CFloatValue> sensitivity;
SP<Config::Values::CFloatValue> maximum;
SP<SHyprCtlCommand> diagnosticCommand;
uint64_t events = 0, consumed = 0;
uint32_t lastMods = 0;
double lastDelta = 0;
std::string lastReply;

std::string setZoom(double value) {
    return HyprlandAPI::invokeHyprctlCommand("eval", std::format("hl.config({{cursor = {{zoom_factor = {:.6f}}}}})", value));
}

void processAxis(const IPointer::SAxisEvent& event, Event::SCallbackInfo& info, uint32_t mods) {
    // Control alone; other modifier combinations pass through.
    constexpr uint32_t CONTROL = 1U << 2;
    if (info.cancelled || event.source != WL_POINTER_AXIS_SOURCE_FINGER ||
        event.axis != WL_POINTER_AXIS_VERTICAL_SCROLL || !g_pInputManager ||
        g_pInputManager->isLocked() || (PROTO::inputCapture && PROTO::inputCapture->isCaptured()) ||
        mods != CONTROL)
        return;

    // Consume even zero-delta stop events and scroll at the zoom limits.
    if (event.delta == 0) {
        info.cancelled = true;
        return;
    }
    if (!std::isfinite(event.delta))
        return;

    static const CConfigValue<Config::FLOAT> zoom("cursor:zoom_factor");
    const double current = *zoom;
    const double next = ZoomScroll::nextZoom(current, event.delta, sensitivity->value(), maximum->value());
    if (next != current) {
        lastReply = setZoom(next);
        // Do not swallow scrolling when the compositor rejects the update.
        if (lastReply != "ok")
            return;
    }
    info.cancelled = true;
}

void onAxis(const IPointer::SAxisEvent& event, Event::SCallbackInfo& info) {
    ++events;
    lastMods = g_pInputManager ? g_pInputManager->getModsFromAllKBs() : 0;
    lastDelta = event.delta;
    processAxis(event, info, lastMods);
    if (info.cancelled)
        ++consumed;
}

std::string diagnostic(eHyprCtlOutputFormat, std::string request) {
    static const CConfigValue<Config::FLOAT> zoom("cursor:zoom_factor");
    if (request.ends_with("selftest")) {
        if (g_pInputManager->isLocked() || (PROTO::inputCapture && PROTO::inputCapture->isCaptured()))
            return "selftest skipped: locked or captured";
        const double original = *zoom;
        if (setZoom(2) != "ok")
            return "FAIL: unable to set initial zoom";
        IPointer::SAxisEvent event;
        event.source = WL_POINTER_AXIS_SOURCE_FINGER;
        event.delta = -10;
        Event::SCallbackInfo info;
        processAxis(event, info, 4);
        const double after = *zoom;
        const double expected = ZoomScroll::nextZoom(2, -10, sensitivity->value(), maximum->value());
        const bool changedCorrectly = info.cancelled && std::abs(after - expected) < 0.00001;
        info.cancelled = false;
        processAxis(event, info, 0);
        const bool ordinaryPasses = !info.cancelled && *zoom == after;
        info.cancelled = false;
        processAxis(event, info, 64);
        const bool commandPasses = !info.cancelled && *zoom == after;
        info.cancelled = false;
        event.source = WL_POINTER_AXIS_SOURCE_WHEEL;
        processAxis(event, info, 4);
        const bool wheelPasses = !info.cancelled && *zoom == after;
        const auto restored = setZoom(original);
        return std::format("{}: synthetic finger scroll 2 -> {:.6f}, ordinary={}, command={}, wheel={}, restore={}",
            changedCorrectly && ordinaryPasses && commandPasses && wheelPasses && restored == "ok" ? "PASS" : "FAIL",
            after, ordinaryPasses, commandPasses, wheelPasses, restored);
    }
    return std::format("events={} consumed={} last_mods={} last_delta={} zoom={} last_update_reply={}\nZoom modifier: Control=4 (alone). SUPER/Command=64 passes through. Self-test: hyprctl --batch 'zoomscroll selftest'",
        events, consumed, lastMods, lastDelta, *zoom, lastReply);
}
}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    if (std::string{__hyprland_api_get_hash()} != __hyprland_api_get_client_hash())
        throw std::runtime_error("zoom-scroll: Hyprland ABI mismatch; rebuild against the running version");

    sensitivity = makeShared<Config::Values::CFloatValue>(
        "plugin:zoom_scroll:sensitivity", "Zoom speed per scroll unit; negative reverses direction", 0.01F,
        Config::Values::SFloatValueOptions{.min = -0.1F, .max = 0.1F});
    maximum = makeShared<Config::Values::CFloatValue>(
        "plugin:zoom_scroll:max_zoom", "Maximum whole-screen zoom factor", 8.F,
        Config::Values::SFloatValueOptions{.min = 1.F, .max = 20.F});
    if (!HyprlandAPI::addConfigValueV2(handle, sensitivity) || !HyprlandAPI::addConfigValueV2(handle, maximum))
        throw std::runtime_error("zoom-scroll: cannot register configuration");

    axisListener = Event::bus()->m_events.input.mouse.axis.listen(onAxis);
    diagnosticCommand = HyprlandAPI::registerHyprCtlCommand(handle, {"zoomscroll", false, diagnostic});
    if (!diagnosticCommand)
        throw std::runtime_error("zoom-scroll: cannot register diagnostics");
    return {"zoom-scroll", "Control + two-finger scroll accessibility zoom", "local", "0.1.2"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    axisListener.reset();
    sensitivity.reset();
    maximum.reset();
}

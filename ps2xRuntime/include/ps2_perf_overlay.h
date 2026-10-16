#pragma once

class PS2Runtime;

// Performance overlay for the presentation window.
//
// F10 toggles a small box with:
//   - game FPS: frames per second the game completes. Games differ in how a frame can be seen
//     from outside (many never flip DISPFB and copy into one display buffer instead), so the game
//     code reports it with noteGameFrame(); without reports the line shows "--".
//   - speed: emulated vertical blanks per second against the 60 of the real console. VBlanks are
//     scheduled in emulated EE time, so this is how fast the emulated machine runs.
//   - window FPS: presentation loop rate (raylib).
// PS2X_SHOW_FPS=1 shows it from startup. While visible it also prints a "[perf]" line every
// 5 seconds, so scripts can measure without looking at the window.
namespace ps2_perf_overlay
{
    // Game code (an override) calls this once per completed game frame, from any thread.
    void noteGameFrame() noexcept;

    // Call once per presentation frame, between BeginDrawing() and EndDrawing(), on the window thread,
    // before the debug UI is drawn.
    void frame(PS2Runtime &runtime);
}

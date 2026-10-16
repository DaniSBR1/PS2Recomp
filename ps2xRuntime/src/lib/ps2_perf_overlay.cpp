#include "ps2_perf_overlay.h"

#include "ps2_runtime.h"
#include "runtime/ee_scheduler.h"
#include "raylib.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <iostream>

namespace ps2_perf_overlay
{
    namespace
    {
        constexpr double kSampleWindowSeconds = 0.5;
        constexpr double kLogPeriodSeconds = 5.0;
        constexpr double kConsoleVBlanksPerSecond = 60.0;

        std::atomic<uint64_t> g_gameFrames{0};

        struct State
        {
            bool initialized = false;
            bool visible = false;
            bool gameReportsFrames = false;
            double lastToggle = -1.0;

            double windowStart = 0.0;
            uint64_t framesAtWindowStart = 0;
            uint64_t vsyncAtWindowStart = 0;

            double gameFps = 0.0;
            double vblanksPerSecond = 0.0;

            double lastLogTime = 0.0;
            uint64_t framesAtLastLog = 0;
            uint64_t vsyncAtLastLog = 0;
        };

        State &state()
        {
            static State s;
            return s;
        }

        void initialize(State &s, PS2Runtime &runtime, double now)
        {
            const char *env = std::getenv("PS2X_SHOW_FPS");
            s.visible = env != nullptr && env[0] == '1';
            s.windowStart = now;
            s.lastLogTime = now;
            s.framesAtWindowStart = s.framesAtLastLog = g_gameFrames.load(std::memory_order_relaxed);
            s.vsyncAtWindowStart = s.vsyncAtLastLog = runtime.eeScheduler().currentVSyncTick();
            s.initialized = true;
        }

        void sample(State &s, PS2Runtime &runtime, double now)
        {
            const uint64_t frames = g_gameFrames.load(std::memory_order_relaxed);
            if (frames != 0u)
                s.gameReportsFrames = true;
            const uint64_t vsync = runtime.eeScheduler().currentVSyncTick();

            const double elapsed = now - s.windowStart;
            if (elapsed >= kSampleWindowSeconds)
            {
                s.gameFps = static_cast<double>(frames - s.framesAtWindowStart) / elapsed;
                s.vblanksPerSecond = static_cast<double>(vsync - s.vsyncAtWindowStart) / elapsed;
                s.framesAtWindowStart = frames;
                s.vsyncAtWindowStart = vsync;
                s.windowStart = now;
            }

            const double sinceLog = now - s.lastLogTime;
            if (!s.visible || sinceLog >= kLogPeriodSeconds)
            {
                if (s.visible)
                {
                    const double vblanks = static_cast<double>(vsync - s.vsyncAtLastLog) / sinceLog;
                    std::printf("[perf] game FPS %.2f, speed %.1f%% (VBlank/s %.1f), window FPS %d\n",
                                static_cast<double>(frames - s.framesAtLastLog) / sinceLog,
                                100.0 * vblanks / kConsoleVBlanksPerSecond, vblanks, GetFPS());
                    std::fflush(stdout);
                }
                s.framesAtLastLog = frames;
                s.vsyncAtLastLog = vsync;
                s.lastLogTime = now;
            }
        }

        void draw(const State &s)
        {
            char lines[3][64];
            if (s.gameReportsFrames)
                std::snprintf(lines[0], sizeof(lines[0]), "FPS juego: %.1f", s.gameFps);
            else
                std::snprintf(lines[0], sizeof(lines[0]), "FPS juego: --");
            std::snprintf(lines[1], sizeof(lines[1]), "Velocidad: %.0f%% (%.1f VBlank/s)",
                          100.0 * s.vblanksPerSecond / kConsoleVBlanksPerSecond, s.vblanksPerSecond);
            std::snprintf(lines[2], sizeof(lines[2]), "Ventana:   %d FPS", GetFPS());

            constexpr int kFontSize = 20;
            constexpr int kPadding = 8;
            constexpr int kLineHeight = kFontSize + 4;
            int width = 0;
            for (const char *line : lines)
            {
                const int lineWidth = MeasureText(line, kFontSize);
                width = lineWidth > width ? lineWidth : width;
            }
            const int boxWidth = width + kPadding * 2;
            const int boxHeight = kLineHeight * 3 + kPadding * 2 - 4;
            const int x = GetScreenWidth() - boxWidth - 10;
            const int y = 70; // below the debug UI bar

            DrawRectangle(x, y, boxWidth, boxHeight, Color{0, 0, 0, 170});
            // Green from 30 FPS up, yellow from 15, red below
            const Color fpsColor = !s.gameReportsFrames ? RAYWHITE
                                   : s.gameFps >= 30.0  ? GREEN
                                   : s.gameFps >= 15.0  ? YELLOW
                                                        : RED;
            DrawText(lines[0], x + kPadding, y + kPadding, kFontSize, fpsColor);
            DrawText(lines[1], x + kPadding, y + kPadding + kLineHeight, kFontSize, RAYWHITE);
            DrawText(lines[2], x + kPadding, y + kPadding + kLineHeight * 2, kFontSize, RAYWHITE);
        }
    }

    void noteGameFrame() noexcept
    {
        g_gameFrames.fetch_add(1u, std::memory_order_relaxed);
    }

    void frame(PS2Runtime &runtime)
    {
        State &s = state();
        const double now = GetTime();
        if (!s.initialized)
            initialize(s, runtime, now);
        // Debounced: one physical press must toggle once
        constexpr double kToggleDebounceSeconds = 0.25;
        if (IsKeyPressed(KEY_F10) && now - s.lastToggle >= kToggleDebounceSeconds)
        {
            s.lastToggle = now;
            s.visible = !s.visible;
            std::cout << "[perf] overlay " << (s.visible ? "on" : "off") << " (F10)" << std::endl;
        }
        sample(s, runtime, now);
        if (s.visible)
            draw(s);
    }
}

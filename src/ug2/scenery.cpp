// Scenery settings and the scene log (scenery.h): one line per timing log interval, on the render thread.

#include "scenery.h"

#include <platform/config.h>
#include <platform/log.h>
#include <platform/perf.h>
#include <recomp.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>

#if defined(__APPLE__)
#include <unistd.h>
#endif

namespace ug2 { namespace scenery {

Settings g_settings = {};
Counters g_counters = {};

namespace
{

using x86::reg32;

const reg32 Players = 0x008900AC;
const reg32 PlayerVehicle = 0x4;
const reg32 VehiclePosition = 0x60;
const reg32 FramesRendered = 0x0086514C;

Counters s_last = {};
reg32 s_lastFrames = 0;
reg32 s_everyFrames = 0;    // scene_log_frames

struct Line
{
    char text[1024] = {};
    int length = 0;

    __attribute__((format(printf, 2, 3))) void append(const char* format, ...)
    {
        if (length >= int(sizeof(text)) - 1)
            return;
        va_list args;
        va_start(args, format);
        length += vsnprintf(text + length, sizeof(text) - length, format, args);
        va_end(args);
        length = std::min(length, int(sizeof(text)) - 1);
    }
};

void report(uint32_t)
{
    const Counters now = g_counters;
    const Counters& b = s_last;
    const reg32 frames = rc::ld32(FramesRendered);
    Line line;
    const reg32 player = rc::ld32(Players);
    const reg32 vehicle = player ? rc::ld32(player + PlayerVehicle) : 0;
    line.append("scene: %u game frames, car at (%.0f, %.0f, %.0f)", unsigned(frames - s_lastFrames),
                vehicle ? double(rc::ldf32(vehicle + VehiclePosition)) : 0.0,
                vehicle ? double(rc::ldf32(vehicle + VehiclePosition + 4)) : 0.0,
                vehicle ? double(rc::ldf32(vehicle + VehiclePosition + 8)) : 0.0);
    line.append(", scale %.0f", double(now.scale));
    static const char* const names[PhaseCount] = {"cull", "draw", "flush"};
    for (int p = 0; p < PhaseCount; ++p)
        line.append("%s %s %u calls %.1f ms %.3f Mi", p ? "," : " |", names[p], unsigned(now.calls[p] - b.calls[p]),
                    double(now.us[p] - b.us[p]) / 1000.0, double(now.instructions[p] - b.instructions[p]) / 1e6);
    line.append(", thread %.3f Mi", double(now.threadInstructions - b.threadInstructions) / 1e6);
    for (int v = 0; v < 2; ++v)
    {
        line.append(" | %s: tested %u, engine culled %u, small culled %u (%u polys), drawn/polys by width",
                    v ? "other views" : "player views", unsigned(now.tested[v] - b.tested[v]),
                    unsigned(now.engineCulled[v] - b.engineCulled[v]), unsigned(now.smallCulled[v] - b.smallCulled[v]),
                    unsigned(now.smallPolys[v] - b.smallPolys[v]));
        for (int i = 0; i < WidthBands; ++i)
            line.append(" %u/%u", unsigned(now.drawn[v][i] - b.drawn[v][i]), unsigned(now.polys[v][i] - b.polys[v][i]));
    }
    line.append(" | lowered %u (%u -> %u polys)", unsigned(now.lowered - b.lowered),
                unsigned(now.loweredFrom - b.loweredFrom), unsigned(now.loweredTo - b.loweredTo));
    pf::logInfo("%s", line.text);
    s_last = now;
    s_lastFrames = frames;
}

}

Stamp stamp()
{
    Stamp s = {pf::perf::now(), 0};
#if defined(__APPLE__)
    uint64_t counts[2] = {};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    // thread_selfcounts: instructions and cycles of the calling thread.
    if (syscall(186, 1, counts, sizeof(counts)) == 0)
        s.instructions = counts[0];
#pragma clang diagnostic pop
#endif
    return s;
}

void add(Phase phase, const Stamp& start)
{
    const Stamp end = stamp();
    ++g_counters.calls[phase];
    g_counters.us[phase] += end.us - start.us;
    g_counters.instructions[phase] += end.instructions - start.instructions;
    g_counters.threadInstructions = end.instructions;
    if (s_everyFrames && rc::ld32(FramesRendered) - s_lastFrames >= s_everyFrames)
        report(0);
}

void configure()
{
    const int world = std::min(std::max(pf::Config::getInt("world_min_pixels", 0), 0), 64);
    const int other = std::min(std::max(pf::Config::getInt("reflection_min_pixels", world), 0), 64);
    g_settings.mainMinPixels = float(world);
    g_settings.otherMinPixels = float(other);
    const int lod = std::min(std::max(pf::Config::getInt("world_lod_pixels", 0), 0), 100000);
    g_settings.lodPixels = float(lod);
    g_settings.lodMinTris = uint32_t(std::min(std::max(pf::Config::getInt("world_lod_min_tris", 24), 0), 100000));
    g_settings.lodMaxPercent = uint32_t(std::min(std::max(pf::Config::getInt("world_lod_max_percent", 75), 0), 100));
    g_settings.lodMinWidth = float(std::min(std::max(pf::Config::getInt("world_lod_min_width", 0), 0), 10000));
    g_settings.lodSigns = pf::Config::getBool("world_lod_signs", true);
    g_settings.keepLights = pf::Config::getBool("world_min_pixels_lights", true);
    g_settings.log = pf::Config::getBool("scene_log", false) && pf::perf::enabled();
    pf::logInfo("scenery: minimum width %d px in the player views (world_min_pixels), %d px in the others "
                "(reflection_min_pixels), models with lights %s; reflection models below %d px (world_lod_pixels) for types of "
                "%u triangles or more whose reflection model has at most %u%% of them, %d m wide or more, signs %s; scene log %s (scene_log, needs timing_log; a line per "
                "timing log interval, or per scene_log_frames game frames)", world, other,
                g_settings.keepLights ? "kept" : "culled too", lod, unsigned(g_settings.lodMinTris),
                unsigned(g_settings.lodMaxPercent), int(g_settings.lodMinWidth),
                g_settings.lodSigns ? "included (world_lod_signs)" : "excluded", g_settings.log ? "on" : "off");
    s_everyFrames = reg32(std::max(pf::Config::getInt("scene_log_frames", 0), 0));
    if (g_settings.log && !s_everyFrames)
        pf::perf::setReporter(report);
}

}}

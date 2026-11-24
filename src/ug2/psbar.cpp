// The PS2 bottom bar: the PS2 bar group inside UI_PC_Help_Bar.fng is filled from the PC bar's own per-screen flags.

#include "psbar.h"
#include "ownscreens.h"
#include "psbarlogic.h"
#include "psbarpackage.h"

#include <lib/memmap.h>
#include <platform/config.h>
#include <platform/log.h>
#include <recomp.h>

#include <string>

namespace ug2 { namespace psbar {

namespace
{

using x86::reg32;

const reg32 BitText = 0x0052D330;       // stdcall (bit): the language text hash of an item bit
const reg32 BitMessage = 0x0052D170;    // stdcall (bit): the message hash an item bit sends
const reg32 FindObject = 0x005379C0;    // cdecl (package name, object hash)
const reg32 SetText = 0x00535FE0;       // thiscall on a text object (string): sets the text, grows its buffer
const reg32 SetFormatted = 0x00537B80;  // cdecl (package name, object hash, format, argument)
const reg32 ShowObject = 0x0050CA50, HideObject = 0x0050CA00;       // cdecl (object), both recursive over groups
const reg32 TableEntries = 0x008383D8, TableCount = 0x008383D0;
const reg32 GameScreenTable = 0x007F7DC8;
const reg32 StringFormat = 0x00786E70, ProfileName = 0x0083A9E0;
const reg32 DataOffset = 0x2C, ObjectFlags = 0x1C, DirtyFlag = 0x400000;
const char* const Bar = "UI_PC_Help_Bar.fng", *const Donor = "UI_OptionsController.fng";
const char* const DonorArchive = "C:\\FRONTEND\\FrontB.lzc";

Prompt s_second = { 0, nullptr };
bool s_secondShown = false;
HelpVerdict s_help = { true, "general", "not decided yet" };
reg32 s_buffer = 0;
bool s_missingLogged = false;

std::string readString(reg32 address)
{
    std::string text;
    for (char c; address && (c = char(rc::ld8(address))) != 0 && text.size() < 160; ++address)
        text += c;
    return text;
}

std::string wideString(reg32 address)
{
    std::string text;
    for (unsigned c; address && (c = rc::ld16(address)) != 0 && text.size() < 64; address += 2)
        text += c < 128 ? char(c) : '?';
    return text;
}

reg32 call(win32::WinApplication* app, x86::CPU& cpu, reg32 address, std::initializer_list<reg32> args, reg32 self = 0)
{
    const x86::CPU saved = cpu;
    reg32 sp = cpu.esp;
    for (const reg32* it = args.end(); it != args.begin();)
    {
        --it;
        sp -= 4;
        rc::st32(sp, *it);
    }
    sp -= 4;
    rc::st32(sp, 0);
    cpu.esp = sp;
    cpu.ecx = self;
    app->dynamic_call(address, cpu);
    const reg32 result = cpu.eax;
    cpu = saved;
    return result;
}

std::string tableText(reg32 hash)
{
    const reg32 table = rc::ld32(TableEntries), count = rc::ld32(TableCount);
    for (reg32 i = 0; table && i < count; ++i)
    {
        if (rc::ld32(table + 8 * i) == hash)
            return readString(rc::ld32(table + 8 * i + 4));
    }
    return std::string();
}

bool devLog()
{
#ifdef DEV_TOOLS
    return true;
#else
    return pf::Config::getBool("pad_log", false);
#endif
}

reg32 find(win32::WinApplication* app, x86::CPU& cpu, reg32 package, reg32 hash)
{
    return call(app, cpu, FindObject, { package, hash });
}

void writeText(win32::WinApplication* app, x86::CPU& cpu, reg32 object, const std::string& text, const char* label)
{
    if (!s_buffer)
        s_buffer = (new win32::MemMap(256))->getBlockStart();
    for (size_t i = 0; i <= text.size() && i < 255; ++i)
        rc::st8(s_buffer + reg32(i), uint8_t(i < text.size() ? text[i] : 0));
    call(app, cpu, SetText, { s_buffer }, object);
    rc::st32(object + ObjectFlags, rc::ld32(object + ObjectFlags) | 2 | DirtyFlag);
    if (devLog())
        pf::logInfo("psbar: %s text '%s' (stored '%s', capacity %u)", label, text.c_str(), wideString(rc::ld32(object + 0x64)).c_str(),
                    unsigned(rc::ld32(object + 0x68)));
}

void hideExtras(win32::WinApplication* app, x86::CPU& cpu, reg32 package)
{
    for (reg32 hash : { PsMoney, PsDpad })
    {
        if (const reg32 object = find(app, cpu, package, hash))
            call(app, cpu, HideObject, { object });
    }
    if (!s_help.available)
    {
        for (reg32 hash : { PsHelpWord, PsHelpIcon })
        {
            if (const reg32 object = find(app, cpu, package, hash))
                call(app, cpu, HideObject, { object });
        }
    }
}

// The screen object of the fill's caller: it keeps itself in esi (edi in one caller), and +4 is the package name given to the fill.
reg32 liveScreen(const x86::CPU& cpu, reg32 nameArg)
{
    for (reg32 candidate : { cpu.esi, cpu.edi })
    {
        if (candidate >= 0x00100000 && candidate < 0x7F000000 && (candidate & 3) == 0 && rc::ld32(candidate + 4) == nameArg)
            return candidate;
    }
    return 0;
}

// The help id from the screen's table entry: ours first, then the game's table (28-byte entries, name first, help id at +8).
reg32 tableHelp(const std::string& screen)
{
    if (screen.empty())
        return 0;
    const reg32 hash = ownscreens::hashOfName(screen);
    reg32 entry = ownscreens::entryOf(hash);
    for (reg32 i = 0; !entry && i < 400; ++i)
    {
        const reg32 row = GameScreenTable + 28 * i, name = rc::ld32(row);
        if (!name)
            break;
        if (ownscreens::hashOfName(readString(name)) == hash)
            entry = row;
    }
    return entry ? rc::ld32(entry + 8) : 0;
}

void secondSlot(win32::WinApplication* app, x86::CPU& cpu, reg32 package, bool show)
{
    const reg32 word = find(app, cpu, package, PsSecondWord), icon = find(app, cpu, package, PsSecondIcon);
    if (!word || !icon)
        return;
    if (show && s_second.token)
    {
        const std::string text = tableText(s_second.text);
        writeText(app, cpu, word, text.empty() ? "?" : text, "second word");
        writeText(app, cpu, icon, std::string("$") + s_second.token + "$", "second icon");
        call(app, cpu, ShowObject, { word });
        call(app, cpu, ShowObject, { icon });
    }
    else
    {
        call(app, cpu, HideObject, { word });
        call(app, cpu, HideObject, { icon });
    }
}

reg32 originalBit(win32::WinApplication* app, x86::CPU& cpu, reg32 address, reg32 bit)
{
    return call(app, cpu, address, { bit });
}

}

void install()
{
    if (!pf::Config::getBool("ps2_prompts", true))
        return;
    ownscreens::addReplacement(Bar, [](fepackage::Package& bar)
    {
        fepackage::Package donor;
        Built built;
        if (!ownscreens::loadFromArchive(DonorArchive, Donor, &donor))
        {
            pf::logError("psbar: %s is not in %s; the PC help bar stays as it is", Donor, DonorArchive);
            return false;
        }
        if (!buildBar(bar, donor, &built))
        {
            pf::logError("psbar: building the PS2 bar from %s failed: %s", Donor, built.error.c_str());
            return false;
        }
        pf::logInfo("psbar: PS2 bar group copied from %s (read from %s): %u objects, %u resources merged, %u PC objects hidden", Donor, DonorArchive,
                    unsigned(built.copied), unsigned(built.resourcesAdded), unsigned(built.hiddenPc));
        return true;
    });
}

void beforeFill(win32::WinApplication* app, x86::CPU& cpu)
{
    static const bool enabled = pf::Config::getBool("ps2_prompts", true);
    if (!enabled)
        return;
    const reg32 flags = rc::ld32(cpu.esp + 4), screenName = rc::ld32(cpu.esp + 8);
    const std::string screen = screenName ? readString(screenName) : std::string();
    s_second = secondPrompt(screenName ? screen.c_str() : nullptr, flags, [&](uint32_t bit) { return originalBit(app, cpu, BitMessage, bit); },
                            [&](uint32_t bit) { return originalBit(app, cpu, BitText, bit); });
    rc::st32(cpu.esp + 4, 0);
    const reg32 live = screenName ? liveScreen(cpu, screenName) : 0;
    const reg32 help = live ? rc::ld32(live + 0x28) : tableHelp(screen);
    s_help = helpVerdict(help, help && !tableText(help).empty(), flags);
    static std::string s_helpKey;
    const std::string helpKey = screen + "|" + std::to_string(help) + "|" + std::to_string(flags & TipBit) + "|" + (live ? "live" : "table");
    if (devLog() && helpKey != s_helpKey)
        pf::logInfo("psbar: screen '%s' help id %08x (%s), flags %08x: Help %s, %s help (%s)", screen.c_str(), unsigned(help), live ? "live screen" : "table entry",
                    unsigned(flags), s_help.available ? "shown" : "hidden", s_help.kind, s_help.why);
    s_helpKey = helpKey;
    static std::string s_last;
    static reg32 s_lastFlags = ~0u, s_lastSecond = ~0u;
    if (devLog() && (screen != s_last || flags != s_lastFlags || s_second.text != s_lastSecond))
        pf::logInfo("psbar: fill screen '%s' flags %08x second prompt text %08x token %s", screen.c_str(), unsigned(flags), unsigned(s_second.text),
                    s_second.token ? s_second.token : "none");
    s_last = screen;
    s_lastFlags = flags;
    s_lastSecond = s_second.text;
}

void afterFill(win32::WinApplication* app, x86::CPU& cpu, reg32 bar)
{
    static const bool enabled = pf::Config::getBool("ps2_prompts", true);
    if (!enabled)
        return;
    const reg32 package = rc::ld32(bar + 4);
    static bool s_firstLogged = false;
    if (!s_firstLogged)
    {
        s_firstLogged = true;
        const ownscreens::ReplacementState state = ownscreens::replacementState(Bar);
        pf::logInfo("psbar: first fill, bar class object %08x (persistent, exists before its package): swap %s, package record %s, FEng instance %08x",
                    unsigned(bar), state.done ? "done" : "NOT done", state.recordOurs ? "uses our chunk" : "does NOT use our chunk", unsigned(state.instance));
    }
    const reg32 root = find(app, cpu, package, PsRoot);
    if (!root)
    {
        if (!s_missingLogged)
            pf::logError("psbar: the PS2 bar group is not in the bar package (%08x); the PC bar is hidden and nothing is drawn", unsigned(package));
        s_missingLogged = true;
        return;
    }
    const bool shown = rc::ld8(bar + 0x6C) != 0;
    if (!shown)
    {
        call(app, cpu, HideObject, { root });
        return;
    }
    call(app, cpu, ShowObject, { root });
    hideExtras(app, cpu, package);
    if (const reg32 name = find(app, cpu, package, PsName))
    {
        call(app, cpu, SetFormatted, { package, PsName, StringFormat, ProfileName });
        if (devLog())
            pf::logInfo("psbar: name text '%s'", wideString(rc::ld32(name + 0x64)).c_str());
    }
    if (const reg32 help = s_help.available ? find(app, cpu, package, PsHelpWord) : 0)
    {
        const std::string text = tableText(HelpText);
        writeText(app, cpu, help, text.empty() ? "Help" : text, "help word");
    }
    s_secondShown = s_second.token != nullptr;
    secondSlot(app, cpu, package, true);
#ifdef DEV_TOOLS
    static reg32 s_logged = 0;
    const reg32 key = s_second.text ^ 0x9E3779B9;
    if (key != s_logged)
    {
        s_logged = key;
        const reg32 data = rc::ld32(root + DataOffset);
        pf::logInfo("psbar: PS2 group %08x shown x %.2f y %.2f z %.2f flags %08x, second prompt %s", unsigned(root), data ? rc::ldf32(data + 0x1C) : 0.0f,
                    data ? rc::ldf32(data + 0x20) : 0.0f, data ? rc::ldf32(data + 0x24) : 0.0f, unsigned(rc::ld32(root + ObjectFlags)),
                    s_second.token ? s_second.token : "none");
    }
#endif
}

void visibility(win32::WinApplication* app, x86::CPU& cpu, reg32 bar, bool show)
{
    static const bool enabled = pf::Config::getBool("ps2_prompts", true);
    if (!enabled)
        return;
    const reg32 package = rc::ld32(bar + 4);
    const reg32 root = find(app, cpu, package, PsRoot);
    if (!root)
        return;
    if (!show)
    {
        call(app, cpu, HideObject, { root });
        return;
    }
    call(app, cpu, ShowObject, { root });
    hideExtras(app, cpu, package);
    secondSlot(app, cpu, package, s_secondShown);
}

}}

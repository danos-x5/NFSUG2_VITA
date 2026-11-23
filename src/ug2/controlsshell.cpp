// Options > Controls as a visual shell on three own screens (copies of UI_Options and UI_OptionsController): rows,
// texts, icons and navigation only. Nothing is remapped, saved or changed.

#include "controlsshell.h"
#include "fepackage.h"
#include "controlspack.h"
#include "ownscreens.h"
#include "psmap.h"
#include "psprompt.h"
#include "vitapicture.h"

#include <lib/memmap.h>
#include <platform/log.h>
#include <recomp.h>

#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ug2 { namespace controlsshell {

namespace {

using x86::reg32;
using fepackage::nameHash;

const reg32 NewObject = 0x575620;           // cdecl (bytes)
const reg32 ScreenBase = 0x552780;          // thiscall (screen data): the options screen's base constructor
const reg32 OptionsVtable = 0x795e28;       // 0 destructor, 2 message handler 0x49C690, 4 row builder 0x4B8470
const reg32 OptionsMessage = 0x49c690;      // thiscall (message, 3 more), ret 0x10
const reg32 OptionsDestructor = 0x4cfaf0;
const reg32 ScreenSize = 0x164;
const reg32 RowConstructor = 0x51e230;      // thiscall (row groups x4 of the screen, enabled)
const reg32 AddRow = 0x54ef30;              // thiscall on the screen (row, 1)
const reg32 FinishRows = 0x539650;          // thiscall on the screen (0)
const reg32 HideObject = 0x50ca00;          // cdecl (object)
const reg32 FindObject = 0x5379c0;          // cdecl (package name, hash)
const reg32 SetText = 0x50c900;             // cdecl (text object, text hash)
const reg32 SetFormatted = 0x537b30;        // cdecl (text object, format, argument list): the object's own string
const reg32 FormatString = 0x786e70;        // the game's "%s"
const reg32 SwitchScreen = 0x537980;        // cdecl (source name, target name, 0)
const reg32 MeasureText = 0x50bec0;         // thiscall on a font (wide text, 0): the width on the x87 stack
const reg32 TextTable = 0x8383d8, TextCount = 0x8383d0;
const reg32 TwoValueRow = 0x794138;         // vtable of a two-value row; 17 slots
const reg32 GameMode = 0x850078;
const reg32 PcControllerName = 0x793dcc;    // "UI_Options_PC_Controller.fng", pushed by Options > Controls (0x4B61A5)
const reg32 PcControllerEntryName = 0x7f7e54;   // the screen table's pointer to the same string
const reg32 EventCallback = 0x0050C630;     // cdecl (player, joy event, value)
const reg32 LoadChunks = 0x577ab0;          // cdecl (data, bytes)
const reg32 SetImage = 0x50c990;            // cdecl (image object, texture hash)
const reg32 RowNumber = 0x520020;           // thiscall on the screen (row): its 1-based place in the list
const reg32 ScrollBarUpdate = 0x538080;     // thiscall on the screen's scroll bar (1, rows - 1, row number)
const reg32 ScrollBar = 0x6c, HasScrollBar = 0x146, RowCount = 0x12c;
const reg32 PictureObject = 0xf1534c25;     // CONTROLLER_IMAGE
const char* const PictureTexture = "CTRL_VITA_BODY";
const float PictureShiftX = -37.0f, PictureWidth = 640.0f, PictureHeight = 320.0f;     // controls-overhaul's picture area
const reg32 SlotMessage = 1, SlotUpdate = 3, SlotBase = 16;
const reg32 MessageLeft = 0x9120409e, MessageRight = 0xb5971bf1, MessageAccept = 0xc407210;
const reg32 MessageBack = 0x911ab364, MessageOutroDone = 0xe1fde1d1, ScreenLeaving = 0x138;
const reg32 LabelObject = 0x2c, ValueObject = 0x30, ArrowLeft = 0x4c, ArrowRight = 0x50, HighlightedRow = 0x54;
const reg32 ObjectFlags = 0x1c, ObjectTextHash = 0x60, DataOffset = 0x2c, DataX = 0x1c, DirtyFlag = 0x400000;
const reg32 TextAlign = 0x6c, TextAlignCentre = 1, TextAlignRight = 2;
const reg32 TitleObject = 0x42adb44c;
const reg32 TextControls = 0x04dc6db5, TextJoypad = 0xf91fba4d, TextCustomize = 0x8d5abf58, TextControlType = 0xba7306aa,
            TextNone = 0x001a076f;
const double RowLabelWidth = 225.0;
const float ValueCentreShift = 48.0f;
const int DroppedEvent = -1;

// Fake guest addresses of our native methods.
const reg32 FakeBase = 0x2000;
const reg32 FakeConstructor = FakeBase, FakeDestructor = FakeBase + 0x40, FakeMessage = FakeBase + 0x50, FakeRows = FakeBase + 0x60,
            FakeRowMessage = FakeBase + 0x70, FakeRowUpdate = FakeBase + 0x80, FakeArrowless = FakeBase + 0x100;

enum Screen { ScreenNone, ScreenTop, ScreenJoypad, ScreenCustomize, ScreenCount };
const char* const ScreenNames[ScreenCount] = { "", "UI_Controls.fng", "UI_ControlsJoypad.fng", "UI_ControlsCustomize.fng" };
const char* const ScreenSources[ScreenCount] = { "", "UI_Options.fng", "UI_OptionsController.fng", "UI_Options.fng" };
const reg32 ScreenTitles[ScreenCount] = { 0, TextControls, TextJoypad, TextCustomize };
const Screen ScreenParents[ScreenCount] = { ScreenNone, ScreenNone, ScreenTop, ScreenJoypad };

enum Kind { KindJoypad, KindStick, KindControlType, KindCustomize, KindEntry };

struct Text
{
    reg32 hash;
    const char* plain;
};

const Text StickLabels[6] = { { 0, "Left Dead Zone" }, { 0, "Left Outer Limit" }, { 0, "Left Sensitivity" },
                              { 0, "Right Dead Zone" }, { 0, "Right Outer Limit" }, { 0, "Right Sensitivity" } };
const char* const StickValues[6] = { "10%", "100%", "100%", "10%", "100%", "100%" };

// Customize entries with Control Type 1's controls; empty is "None".
struct Entry
{
    Text label;
    const char* value;
};
const Entry Entries[] =
{
    { { 0, "Steer Left" }, "$BIND_LSTICK$" },
    { { 0, "Steer Right" }, "$BIND_LSTICK$" },
    { { 0, "Free Look" }, "$BIND_RSTICK$" },
    { { 0x619b23b0, nullptr }, "$BIND_CROSS$" },
    { { 0x6ab80afa, nullptr }, "$BIND_L$" },
    { { 0xb70ff4a4, nullptr }, "$BIND_R$" },
    { { 0x5e5c5f2e, nullptr }, "$BIND_CIRCLE$" },
    { { 0xdc1cd97d, nullptr }, "$BIND_TRIANGLE$" },
    { { 0x56afe190, nullptr }, "$BIND_SQUARE$" },
    { { 0x13121940, nullptr }, "$BIND_DPAD_LEFT$" },
    { { 0xd38df194, nullptr }, "$BIND_DPAD_RIGHT$" },
    { { 0x4a1341b3, nullptr }, "Touchscreen" },
    { { 0x6026c91f, nullptr }, "Touchscreen" },
    { { 0x6ef09d0d, nullptr }, "Touchscreen" },
    { { 0x70a30056, nullptr }, "Touchscreen" },
    { { 0xeac74624, nullptr }, "" },
    { { 0x64730aac, nullptr }, "" },
};
const int EntryCount = int(sizeof(Entries) / sizeof(Entries[0]));

struct Row
{
    reg32 row;
    Kind kind;
    int index;      // the stick row or the Customize entry
};

// Customize shows a window of the entries in its rows.
struct Window
{
    int rows = 0, entries = 0, offset = 0, focus = 0;
    int maxOffset() const { return entries > rows ? entries - rows : 0; }
};

Screen s_screen = ScreenNone;
reg32 s_screenObject = 0, s_package = 0;
std::vector<Row> s_rows;
Window s_window;
int s_slots[ScreenCount] = {};      // row slots of each copy (LEFT_ARROW_n objects)
bool s_refresh = false;
int s_settle = 0;
int s_checkIn = 0;                  // frames until the baked edits are checked (log)
reg32 s_vtable = 0, s_rowTable = 0, s_arrowlessTable = 0, s_text = 0, s_textArgument = 0;
win32::MethodPtr s_eventCallback = nullptr;
std::map<reg32, float> s_valueX;     // a value object's x as we last set it
reg32 s_pictureTexture = 0;
std::pair<reg32, int> s_scrollShown;     // highlighted row and window offset the scroll bar shows

reg32 guestMemory(size_t bytes)
{
    return (new win32::MemMap(reg32(bytes)))->getBlockStart();
}

// Guest call restoring only esp, keeping the x87 result.
reg32 call(win32::WinApplication* app, x86::CPU& cpu, reg32 address, reg32 self, std::initializer_list<reg32> args)
{
    const reg32 saved = cpu.esp;
    reg32 sp = saved;
    for (const reg32* it = args.end(); it != args.begin();)
        rc::st32(sp -= 4, *--it);
    rc::st32(sp -= 4, 0);
    cpu.esp = sp;
    cpu.ecx = self;
    app->dynamic_call(address, cpu);
    cpu.esp = saved;
    return cpu.eax;
}

std::string readString(reg32 address)
{
    std::string text;
    for (char c; address && (c = char(rc::ld8(address))) != 0 && text.size() < 200; ++address)
        text += c;
    return text;
}

std::string tableText(reg32 hash)
{
    const reg32 table = rc::ld32(TextTable), count = rc::ld32(TextCount);
    for (reg32 i = 0; table && i < count; ++i)
    {
        if (rc::ld32(table + 8 * i) == hash)
            return readString(rc::ld32(table + 8 * i + 4));
    }
    return std::string();
}

reg32 findObject(win32::WinApplication* app, x86::CPU& cpu, reg32 hash)
{
    return s_package ? call(app, cpu, FindObject, 0, { s_package, hash }) : 0;
}

void setHash(win32::WinApplication* app, x86::CPU& cpu, reg32 object, reg32 hash)
{
    if (object)
        call(app, cpu, SetText, 0, { object, hash });
}

// Sets the object's own string, not a language table text.
void setPlain(win32::WinApplication* app, x86::CPU& cpu, reg32 object, const std::string& text)
{
    if (!object)
        return;
    if (!s_text)
    {
        s_text = guestMemory(256);
        s_textArgument = guestMemory(4);
    }
    const size_t n = text.size() < 255 ? text.size() : 255;
    for (size_t i = 0; i < n; ++i)
        rc::st8(s_text + reg32(i), uint8_t(text[i]));
    rc::st8(s_text + reg32(n), 0);
    rc::st32(s_textArgument, s_text);
    call(app, cpu, SetFormatted, 0, { object, FormatString, s_textArgument });
    rc::st32(object + ObjectFlags, rc::ld32(object + ObjectFlags) | DirtyFlag);
}

void setText(win32::WinApplication* app, x86::CPU& cpu, reg32 object, const Text& text)
{
    if (text.plain)
        setPlain(app, cpu, object, text.plain);
    else
        setHash(app, cpu, object, text.hash);
}

double measure(win32::WinApplication* app, x86::CPU& cpu, reg32 font, const std::string& text)
{
    static reg32 s_buffer = 0;
    if (!s_buffer)
        s_buffer = guestMemory(512);
    const size_t n = text.size() < 250 ? text.size() : 250;
    for (size_t i = 0; i < n; ++i)
        rc::st16(s_buffer + reg32(2 * i), uint8_t(text[i]));
    rc::st16(s_buffer + reg32(2 * n), 0);
    call(app, cpu, MeasureText, font, { s_buffer, 0 });
    return double(cpu.fpu.pop());
}

// One line, cut with "..." when too wide.
std::string fitOneLine(const std::string& text, double limit, const std::function<double(const std::string&)>& width)
{
    if (width(text) <= limit)
        return text;
    std::string s = text;
    while (!s.empty() && width(s + "...") > limit)
        s.pop_back();
    while (!s.empty() && s.back() == ' ')
        s.pop_back();
    return s + "...";
}

void fitLabel(win32::WinApplication* app, x86::CPU& cpu, reg32 label, const Text& text)
{
    const reg32 font = psprompt::lastFont();
    if (!font || !label)
        return;
    const std::string plain = text.plain ? std::string(text.plain) : tableText(text.hash);
    if (plain.empty())
        return;
    const std::string fitted = fitOneLine(plain, RowLabelWidth, [&](const std::string& t) { return measure(app, cpu, font, t); });
    if (fitted != plain)
        setPlain(app, cpu, label, fitted);
}

bool hasArrows(Kind kind)
{
    return kind == KindStick || kind == KindControlType;
}

void hideArrows(win32::WinApplication* app, x86::CPU& cpu, reg32 row)
{
    for (reg32 offset : { ArrowLeft, ArrowRight })
    {
        if (const reg32 arrow = rc::ld32(row + offset))
            call(app, cpu, HideObject, 0, { arrow });
    }
}

void rowTexts(win32::WinApplication* app, x86::CPU& cpu, const Row& r)
{
    const reg32 label = rc::ld32(r.row + LabelObject), value = rc::ld32(r.row + ValueObject);
    switch (r.kind)
    {
    case KindJoypad:
        setHash(app, cpu, label, TextJoypad);
        setPlain(app, cpu, value, "");
        break;
    case KindStick:
        setText(app, cpu, label, StickLabels[r.index]);
        setPlain(app, cpu, value, StickValues[r.index]);
        break;
    case KindControlType:
        setHash(app, cpu, label, TextControlType);
        setPlain(app, cpu, value, "1");
        break;
    case KindCustomize:
        setHash(app, cpu, label, TextCustomize);
        setPlain(app, cpu, value, "");
        break;
    case KindEntry:
    {
        const Entry& e = Entries[r.index];
        setText(app, cpu, label, e.label);
        fitLabel(app, cpu, label, e.label);
        if (*e.value)
            setPlain(app, cpu, value, e.value);
        else
            setHash(app, cpu, value, TextNone);
        break;
    }
    }
}

const Row* rowOf(reg32 row)
{
    for (const Row& r : s_rows)
    {
        if (r.row == row)
            return &r;
    }
    return nullptr;
}

int rowIndex(reg32 row)
{
    for (size_t i = 0; i < s_rows.size(); ++i)
    {
        if (s_rows[i].row == row)
            return int(i);
    }
    return -1;
}

void refreshRows(win32::WinApplication* app, x86::CPU& cpu)
{
    for (const Row& r : s_rows)
        rowTexts(app, cpu, r);
}

void setEntries()
{
    for (size_t i = 0; i < s_rows.size(); ++i)
        s_rows[i].index = s_window.offset + int(i);
    s_refresh = true;
}

bool scrollWindow(Window* w, int direction)
{
    if (direction > 0 && w->focus == w->rows - 1 && w->offset < w->maxOffset())
        ++w->offset;
    else if (direction < 0 && w->focus == 0 && w->offset > 0)
        --w->offset;
    else
        return false;
    return true;
}

// Follows the highlight; a wrap moves the window too.
bool focusWindow(Window* w, int row)
{
    bool moved = false;
    if (w->maxOffset() > 0 && row != w->focus)
    {
        if (w->focus == w->rows - 1 && row == 0 && w->offset == w->maxOffset())
            w->offset = 0, moved = true;
        else if (w->focus == 0 && row == w->rows - 1 && w->offset == 0)
            w->offset = w->maxOffset(), moved = true;
    }
    w->focus = row;
    return moved;
}

void syncFocus()
{
    if (s_screen != ScreenCustomize || !s_screenObject)
        return;
    const int index = rowIndex(rc::ld32(s_screenObject + HighlightedRow));
    if (index >= 0 && focusWindow(&s_window, index))
        setEntries();
}

// Centre alignment; true when the flag changed.
bool centreAlign(reg32 value)
{
    const reg32 align = rc::ld32(value + TextAlign);
    if ((align & (TextAlignCentre | TextAlignRight)) == TextAlignCentre)
        return false;
    rc::st32(value + TextAlign, (align & ~TextAlignRight) | TextAlignCentre);
    return true;
}

// Every value slot centred before any row text is set.
void alignAllValues(win32::WinApplication* app, x86::CPU& cpu)
{
    for (int n = 1; n <= 13; ++n)
    {
        if (const reg32 value = findObject(app, cpu, nameHash("OPTION_DATA_" + std::to_string(n))))
            centreAlign(value);
    }
}

// Entry values centred left of wherever the game last put them.
void centreValues(win32::WinApplication* app, x86::CPU& cpu)
{
    if (s_screen != ScreenCustomize)
        return;
    for (const Row& r : s_rows)
    {
        const reg32 value = rc::ld32(r.row + ValueObject);
        if (r.kind != KindEntry || !value)
            continue;
        const reg32 data = rc::ld32(value + DataOffset);
        const float x = rc::ldf32a(data + DataX);
        const auto last = s_valueX.find(value);
        bool changed = centreAlign(value);
        if (last == s_valueX.end() || last->second != x)
        {
            rc::stf32a(data + DataX, x - ValueCentreShift);
            s_valueX[value] = x - ValueCentreShift;
            changed = true;
        }
        if (changed)
        {
            rc::st32(value + ObjectFlags, rc::ld32(value + ObjectFlags) | DirtyFlag);
            setPlain(app, cpu, value, " ");
            rowTexts(app, cpu, r);
        }
    }
}

// The game's scroll bar placed among all entries, not the slots.
void updateScrollBar(win32::WinApplication* app, x86::CPU& cpu)
{
    if (s_screen != ScreenCustomize || !s_screenObject || !rc::ld8(s_screenObject + HasScrollBar))
        return;
    const reg32 row = rc::ld32(s_screenObject + HighlightedRow);
    if (rowIndex(row) < 0)
        return;
    s_scrollShown = { row, s_window.offset };
    const reg32 number = call(app, cpu, RowNumber, s_screenObject, { row });
    const reg32 last = rc::ld32(s_screenObject + RowCount) - 1 + reg32(EntryCount - s_window.rows);
    call(app, cpu, ScrollBarUpdate, s_screenObject + ScrollBar, { 1, last, number + reg32(s_window.offset) });
}

// The placeholder Vita picture in the screen's picture object.
void showPicture(win32::WinApplication* app, x86::CPU& cpu)
{
    if (!s_pictureTexture)
    {
        const std::vector<uint8_t> rgba = vitapicture::bodyRgba();
        const std::vector<uint8_t> dxt = vitapicture::encodeDxt3(rgba.data(), vitapicture::width(), vitapicture::height());
        const std::vector<uint8_t> pack = controlspack::buildPack(PictureTexture, dxt.data(), dxt.size(), vitapicture::width(),
                                                                  vitapicture::height());
        const reg32 at = guestMemory(pack.size());
        for (size_t i = 0; i < pack.size(); ++i)
            rc::st8(at + reg32(i), pack[i]);
        call(app, cpu, LoadChunks, 0, { at, reg32(pack.size()) });
        s_pictureTexture = controlspack::nameHash(PictureTexture);
        pf::logInfo("controlsshell: placeholder picture %s loaded, pack of %u bytes", PictureTexture, unsigned(pack.size()));
    }
    const reg32 image = findObject(app, cpu, PictureObject);
    if (!image)
        return;
    call(app, cpu, SetImage, 0, { image, s_pictureTexture });
    const reg32 data = rc::ld32(image + DataOffset);
    pf::logInfo("controlsshell: picture object %08x flags %08x at x %.1f y %.1f size %.1f x %.1f", unsigned(image),
                unsigned(rc::ld32(image + ObjectFlags)), rc::ldf32a(data + DataX), rc::ldf32a(data + DataX + 4),
                rc::ldf32a(data + 0x38), rc::ldf32a(data + 0x3c));
}

reg32 rowTable(bool arrows)
{
    if (!s_rowTable)
    {
        s_rowTable = guestMemory(17 * 4);
        s_arrowlessTable = guestMemory(17 * 4);
        for (reg32 i = 0; i < 17; ++i)
        {
            rc::st32(s_rowTable + 4 * i, rc::ld32(TwoValueRow + 4 * i));
            rc::st32(s_arrowlessTable + 4 * i, rc::ld32(TwoValueRow + 4 * i));
        }
        for (reg32 table : { s_rowTable, s_arrowlessTable })
        {
            rc::st32(table + 4 * SlotMessage, FakeRowMessage);
            rc::st32(table + 4 * SlotUpdate, FakeRowUpdate);
        }
        for (reg32 slot : { 4u, 5u, 7u, 8u, 9u, 10u })
            rc::st32(s_arrowlessTable + 4 * slot, FakeArrowless + 0x10 * slot);
    }
    return arrows ? s_rowTable : s_arrowlessTable;
}

void addRow(win32::WinApplication* app, x86::CPU& cpu, reg32 screen, Kind kind, int index)
{
    const reg32 row = call(app, cpu, NewObject, 0, { 0x54 });
    if (!row)
        return;
    call(app, cpu, RowConstructor, row, { screen + 0xfc, screen + 0x10c, screen + 0x114, screen + 0x11c, 1 });
    rc::st32(row, rowTable(hasArrows(kind)));
    s_rows.push_back({ row, kind, index });
    call(app, cpu, AddRow, screen, { row, 1 });
}

// Adds the screen's rows, then finishes them once.
void buildRows(win32::WinApplication* app, x86::CPU& cpu, reg32 screen)
{
    s_rows.clear();
    s_window = Window();
    switch (s_screen)
    {
    case ScreenTop:
        addRow(app, cpu, screen, KindJoypad, 0);
        for (int i = 0; i < 6; ++i)
            addRow(app, cpu, screen, KindStick, i);
        break;
    case ScreenJoypad:
        addRow(app, cpu, screen, KindControlType, 0);
        addRow(app, cpu, screen, KindCustomize, 0);
        break;
    case ScreenCustomize:
    {
        const int slots = s_slots[ScreenCustomize] > 0 ? s_slots[ScreenCustomize] : 8;
        const int shown = slots < EntryCount ? slots : EntryCount;
        for (int i = 0; i < shown; ++i)
            addRow(app, cpu, screen, KindEntry, i);
        s_window.rows = shown;
        s_window.entries = EntryCount;
        break;
    }
    default:
        break;
    }
    call(app, cpu, FinishRows, screen, { 0 });
    for (const Row& r : s_rows)
    {
        if (!hasArrows(r.kind))
            hideArrows(app, cpu, r.row);
    }
    refreshRows(app, cpu);
    centreValues(app, cpu);
    updateScrollBar(app, cpu);
    s_settle = 4;
}

void checkTitle(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 title = findObject(app, cpu, TitleObject);
    const reg32 have = title ? rc::ld32(title + ObjectTextHash) : 0, want = ScreenTitles[s_screen];
    if (title && have != want)
        setHash(app, cpu, title, want);
    pf::logInfo("controlsshell: title object %08x text %08x from the package, %08x wanted%s", unsigned(title), unsigned(have),
                unsigned(want), title && have != want ? ", set" : "");
}

void construct(win32::WinApplication* app, x86::CPU& cpu, Screen which)
{
    const reg32 entry = cpu.esp, data = rc::ld32(entry + 4);
    const x86::CPU saved = cpu;
    const reg32 screen = call(app, cpu, NewObject, 0, { ScreenSize });
    if (screen && !cpu.terminate)
    {
        call(app, cpu, ScreenBase, screen, { data });
        if (!s_vtable)
        {
            s_vtable = guestMemory(5 * 4);
            for (reg32 i = 0; i < 5; ++i)
                rc::st32(s_vtable + 4 * i, rc::ld32(OptionsVtable + 4 * i));
            rc::st32(s_vtable, FakeDestructor);
            rc::st32(s_vtable + 4 * 2, FakeMessage);
            rc::st32(s_vtable + 4 * 4, FakeRows);
        }
        rc::st32(screen, s_vtable);
        for (reg32 b : { 0x14cu, 0x14du, 0x14eu })
            rc::st8(screen + b, 0);
        rc::st32(screen + 0x154, 0);
        rc::st32(screen + 0x158, 0);
        rc::st8(screen + 0x2c, 1);
        rc::st32(screen + 0x134, 8);
        rc::st32(screen + 0x28, 0xed586157);
        s_screen = which;
        s_screenObject = screen;
        s_package = rc::ld32(screen + 4);
        s_valueX.clear();
        if (which == ScreenCustomize)
            alignAllValues(app, cpu);
        if (which == ScreenJoypad)
            showPicture(app, cpu);
        buildRows(app, cpu, screen);
        checkTitle(app, cpu);
        s_checkIn = 10;
        pf::logInfo("controlsshell: %s screen %08x built, %u rows (%d slots), game mode %u", ScreenNames[which],
                    unsigned(screen), unsigned(s_rows.size()), s_slots[which], unsigned(rc::ld32(GameMode)));
    }
    cpu = saved;
    cpu.eax = screen;
    cpu.esp = entry + 4;
}

template <int Which>
void constructScreen(win32::WinApplication* app, x86::CPU& cpu)
{
    construct(app, cpu, Screen(Which));
}

void open(win32::WinApplication* app, x86::CPU& cpu, Screen target)
{
    pf::logInfo("controlsshell: %s opens %s", ScreenNames[s_screen], ScreenNames[target]);
    call(app, cpu, SwitchScreen, 0, { s_package, ownscreens::guestName(ScreenNames[target]), 0 });
}

// Screen message filter: Back leads to the parent screen.
void screenMessage(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 entry = cpu.esp, screen = cpu.ecx, message = rc::ld32(entry + 4);
    if (screen == s_screenObject && message == MessageOutroDone && rc::ld32(screen + ScreenLeaving) == MessageBack)
    {
        const Screen parent = ScreenParents[s_screen];
        if (parent != ScreenNone)
        {
            const x86::CPU saved = cpu;
            pf::logInfo("controlsshell: back from %s to %s", ScreenNames[s_screen], ScreenNames[parent]);
            call(app, cpu, SwitchScreen, 0, { s_package, ownscreens::guestName(ScreenNames[parent]), 0 });
            cpu = saved;
            cpu.esp = entry + 0x14;
            return;
        }
        pf::logInfo("controlsshell: back from %s to the Options menu", ScreenNames[s_screen]);
        s_screen = ScreenNone;
        s_rows.clear();
    }
    app->dynamic_call(OptionsMessage, cpu);
    if (cpu.terminate || s_screen != ScreenCustomize || screen != s_screenObject)
        return;
    const x86::CPU saved = cpu;
    syncFocus();
    updateScrollBar(app, cpu);
    cpu = saved;
}

void screenRows(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 entry = cpu.esp, screen = cpu.ecx;
    const x86::CPU saved = cpu;
    pf::logInfo("controlsshell: row builder slot called on %08x", unsigned(screen));
    if (screen == s_screenObject)
        refreshRows(app, cpu);
    cpu = saved;
    cpu.esp = entry + 4;
}

void screenDestructor(win32::WinApplication* app, x86::CPU& cpu)
{
    if (cpu.ecx == s_screenObject)
    {
        s_screenObject = 0;
        s_screen = ScreenNone;
        s_rows.clear();
    }
    app->dynamic_call(OptionsDestructor, cpu);
}

// Row message: accept opens the next screen, nothing else.
void rowMessage(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 entry = cpu.esp, row = cpu.ecx, message = rc::ld32(entry + 8);
    const x86::CPU saved = cpu;
    const Row* r = rowOf(row);
    if (r && message == MessageAccept && r->kind == KindJoypad)
        open(app, cpu, ScreenJoypad);
    else if (r && message == MessageAccept && r->kind == KindCustomize)
        open(app, cpu, ScreenCustomize);
    else if (r && hasArrows(r->kind) && (message == MessageLeft || message == MessageRight))
        call(app, cpu, rc::ld32(rc::ld32(row) + 4 * SlotBase), row, { message });
    if (r && rowOf(row))
        rowTexts(app, cpu, *rowOf(row));
    cpu = saved;
    cpu.esp = entry + 12;
}

void rowUpdate(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 entry = cpu.esp, row = cpu.ecx;
    const x86::CPU saved = cpu;
    if (const Row* r = rowOf(row))
        rowTexts(app, cpu, *r);
    cpu = saved;
    cpu.esp = entry + 4;
}

// The row's own method, then its arrows hidden again.
template <int Slot>
void arrowlessSlot(win32::WinApplication* app, x86::CPU& cpu)
{
    const reg32 row = cpu.ecx;
    app->dynamic_call(rc::ld32(TwoValueRow + 4 * Slot), cpu);
    if (cpu.terminate)
        return;
    const x86::CPU saved = cpu;
    hideArrows(app, cpu, row);
    cpu = saved;
}

// Customize: up or down at an edge scrolls the entries.
void eventHook(win32::WinApplication* app, x86::CPU& cpu)
{
    const int event = int(rc::ld32(cpu.esp + 8));
    const reg32 value = rc::ld32(cpu.esp + 12);
    if (s_screen == ScreenCustomize && s_window.maxOffset() > 0 && value)
    {
        const int direction = event == psmap::EventPadDown || event == psmap::EventDownAlt ? 1 :
                              event == psmap::EventPadUp || event == psmap::EventUpAlt ? -1 : 0;
        syncFocus();
        Window next = s_window;
        static reg32 s_frame = ~0u;
        static int s_direction = 0;
        const reg32 frame = rc::ld32(0x0086514C);
        if (direction && scrollWindow(&next, direction))
        {
            if (!(frame == s_frame && direction == s_direction))
            {
                s_window = next;
                setEntries();
                const x86::CPU saved = cpu;
                refreshRows(app, cpu);
                updateScrollBar(app, cpu);
                cpu = saved;
            }
            s_frame = frame;
            s_direction = direction;
            rc::st32(cpu.esp + 8, reg32(DroppedEvent));
        }
    }
    s_eventCallback(app, cpu);
}

int countSlots(const fepackage::Package& p)
{
    int n = 0;
    while (!p.find(nameHash("LEFT_ARROW_" + std::to_string(n + 1))).empty())
        ++n;
    return n;
}

// Bakes hidden objects, the title and longer row texts.
void bake(fepackage::Package& p, Screen which, std::initializer_list<reg32> hidden)
{
    size_t count = 0;
    for (reg32 hash : hidden)
        count += p.hide(hash);
    for (size_t title : p.find(TitleObject))
        p.setTag32(title, 0x4853, ScreenTitles[which]);
    for (int n = 1; n <= 13; ++n)
    {
        for (const char* prefix : { "OPTION_NAME_", "OPTION_DATA_" })
        {
            for (size_t o : p.find(nameHash(prefix + std::to_string(n))))
            {
                if (p.tag32(o, 0x6253) < 64)
                    p.setTag32(o, 0x6253, 64);
            }
        }
    }
    s_slots[which] = countSlots(p);
    pf::logInfo("controlsshell: %s baked: %u objects hidden, title text %08x, %d row slots", ScreenNames[which], unsigned(count),
                unsigned(ScreenTitles[which]), s_slots[which]);
}

void bakeOptions(fepackage::Package& p, Screen which)
{
    bake(p, which, { nameHash("ColorCal_Logo"), nameHash("ColorCal_Blurb"), nameHash("Colour_Calibration_Backing"),
                     nameHash("PLAYER"), 0x010cba23, 0x97b71dde, 0xc90dc4ab, 0x444969fd, 0x444969fe });
}

void bakeJoypad(fepackage::Package& p)
{
    bake(p, ScreenJoypad, { nameHash("PLAYER"), 0x010cba23, 0x97b71dde, 0xc90dc4ab, 0x444969fd, 0x444969fe, 0xbaa4198e,
                            0xbaa4198f, 0xbaa41990, 0xbaa41991, 0xbaa41992, 0xbaa41993, 0xbaa41994, 0xbaa41995, 0xbaa41996,
                            0x0f274b7e, 0x0f274b7f, 0x0f274b80, 0x0f274b81, 0x0f274b82 });
    for (size_t image : p.find(PictureObject))
    {
        std::vector<uint8_t>* sa = p.tagData(image, 0x4153);
        if (!sa || sa->size() < 0x40)
            continue;
        float x, size[2] = { PictureWidth, PictureHeight };
        memcpy(&x, &(*sa)[0x1c], 4);
        x += PictureShiftX;
        memcpy(&(*sa)[0x1c], &x, 4);
        memcpy(&(*sa)[0x38], size, 8);
    }
}

// The objects baked hidden, read back from the loaded screen.
void checkHidden(win32::WinApplication* app, x86::CPU& cpu)
{
    const std::vector<reg32> options = { nameHash("ColorCal_Blurb"), nameHash("Colour_Calibration_Backing"), nameHash("PLAYER") };
    const std::vector<reg32> joypad = { 0xbaa4198e, 0x0f274b82, nameHash("PLAYER") };
    int hidden = 0, total = 0;
    for (reg32 hash : s_screen == ScreenJoypad ? joypad : options)
    {
        if (const reg32 object = findObject(app, cpu, hash))
        {
            ++total;
            hidden += (rc::ld32(object + ObjectFlags) & 1) ? 1 : 0;
        }
    }
    pf::logInfo("controlsshell: %s check: %d of %d baked-hidden objects hidden, highlighted row %d of %u", ScreenNames[s_screen],
                hidden, total, rowIndex(rc::ld32(s_screenObject + HighlightedRow)), unsigned(s_rows.size()));
}

}

void configure(win32::WinApplication& app)
{
    app.registerMethod(FakeConstructor + 0x10 * ScreenTop, { "controls screen", constructScreen<ScreenTop> });
    app.registerMethod(FakeConstructor + 0x10 * ScreenJoypad, { "joypad configuration screen", constructScreen<ScreenJoypad> });
    app.registerMethod(FakeConstructor + 0x10 * ScreenCustomize, { "customize screen", constructScreen<ScreenCustomize> });
    app.registerMethod(FakeDestructor, { "controls screen destructor", screenDestructor });
    app.registerMethod(FakeMessage, { "controls screen message", screenMessage });
    app.registerMethod(FakeRows, { "controls screen rows", screenRows });
    app.registerMethod(FakeRowMessage, { "controls row message", rowMessage });
    app.registerMethod(FakeRowUpdate, { "controls row update", rowUpdate });
    app.registerMethod(FakeArrowless + 0x10 * 4, { "controls row layout", arrowlessSlot<4> });
    app.registerMethod(FakeArrowless + 0x10 * 5, { "controls row show", arrowlessSlot<5> });
    app.registerMethod(FakeArrowless + 0x10 * 7, { "controls row focus", arrowlessSlot<7> });
    app.registerMethod(FakeArrowless + 0x10 * 8, { "controls row unfocus", arrowlessSlot<8> });
    app.registerMethod(FakeArrowless + 0x10 * 9, { "controls row select", arrowlessSlot<9> });
    app.registerMethod(FakeArrowless + 0x10 * 10, { "controls row refocus", arrowlessSlot<10> });
    s_eventCallback = app.replaceMethod(EventCallback, eventHook);

    for (int s = ScreenTop; s < ScreenCount; ++s)
    {
        const Screen which = Screen(s);
        ownscreens::addScreen(ScreenNames[s], FakeConstructor + 0x10 * reg32(s));
        ownscreens::addCopy(ScreenSources[s], ScreenNames[s], [which](fepackage::Package& p)
        {
            if (which == ScreenJoypad)
                bakeJoypad(p);
            else
                bakeOptions(p, which);
        });
    }
    // Renames the string Options > Controls pushes; table keeps a copy.
    rc::st32(PcControllerEntryName, ownscreens::guestName("UI_Options_PC_Controller.fng"));
    const char* const top = ScreenNames[ScreenTop];
    for (size_t i = 0; i <= strlen(top); ++i)
        rc::st8(PcControllerName + reg32(i), uint8_t(top[i]));
    pf::logInfo("controlsshell: Options > Controls opens %s (Joypad Configuration, Customize below it)", top);
}

void frame(win32::WinApplication* app, x86::CPU& cpu)
{
    if (s_screen == ScreenNone)
        return;
    const x86::CPU saved = cpu;
    syncFocus();
    if (s_refresh || s_settle > 0)
    {
        s_refresh = false;
        s_settle = s_settle > 0 ? s_settle - 1 : 0;
        refreshRows(app, cpu);
    }
    for (const Row& r : s_rows)
    {
        if (!hasArrows(r.kind))
            hideArrows(app, cpu, r.row);
    }
    centreValues(app, cpu);
    if (s_screen == ScreenCustomize && s_scrollShown != std::make_pair(rc::ld32(s_screenObject + HighlightedRow), s_window.offset))
        updateScrollBar(app, cpu);
    if (s_checkIn > 0 && --s_checkIn == 0)
        checkHidden(app, cpu);
    cpu = saved;
}

}}

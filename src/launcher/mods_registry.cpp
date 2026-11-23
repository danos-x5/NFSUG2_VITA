// The Mods tab's registry: each mod's ini keys and the features the port supports, read through mINI.

#include "mods_registry.h"
#include "exopts.h"
#define MINI_CASE_SENSITIVE
#include "mini/ini.h"
#include <platform/settings.h>
#include <platform/textfile.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace ug2 { namespace mods {

namespace
{

#ifdef DEV_SETTINGS
const char* const BayviewName = "'Bayview's Best' Car Pack";
const char* const BayviewIni = "scripts/NFSU2VitaBayviewsBest.ini";
#define Bayview "Bayview's_Best_Car_Pack_v1.1.1/MODCARS/"

// One car of the pack: on when its six files are there; the game never reads it.
class BayviewCar : public modreq::Setting
{
public:
    BayviewCar(const char* name, const char* car) : Setting(BayviewName, name, BayviewIni, "Cars", car) {}

    bool requirementsMet(const std::string& gameFolder) const override
    {
        for (const std::string& file : files())
            if (!fileExists(gameFolder + "/" + file))
                return false;
        return true;
    }

    std::string requirementsString(const std::string&) const override
    {
        const std::string car = key;
        return "CARS/" + car + "/{GEOMETRY,TEXTURES,VINYLS,PARTS_ANIMATIONS}.BIN, scripts/CarSoundData/CarTypeMapping/" +
               car + ".ini and scripts/UnlimiterData/" + car + ".ini must be in the game folder";
    }

private:
    std::vector<std::string> files() const
    {
        const std::string car = key;
        return { "CARS/" + car + "/GEOMETRY.BIN", "CARS/" + car + "/TEXTURES.BIN", "CARS/" + car + "/VINYLS.BIN",
                 "CARS/" + car + "/PARTS_ANIMATIONS.BIN", "scripts/CarSoundData/CarTypeMapping/" + car + ".ini",
                 "scripts/UnlimiterData/" + car + ".ini" };
    }
};

const BayviewCar s_gs300("Lexus GS300", "GS300");
const BayviewCar s_civic("Honda Civic Si EG6", "CIVICEG");
const BayviewCar s_fx45("Infiniti FX45", "INFIFX45");
#endif

Source s_sources[SourceCount] =
{
    {
        exopts::Name,
        exopts::Version,
        exopts::Author,
        exopts::IniPath,
        {
            { "Hotkeys", "AnyTrackInAnyMode", "36", false },
            { "Hotkeys", "Headlights", "71", false },
            { "Hotkeys", "FreezeCamera", "19", false },
            { "Hotkeys", "UnlockAllThings", "116", false },
            { "Hotkeys", "AutoDrive", "117", false },
            { "Hotkeys", "EnableSaveLoadHotPos", "0", false },
            { "LapControllers", "Minimum", "0", false },
            { "LapControllers", "Maximum", "127", false },
            { "LapControllers", "KOEnabled", "5", false },
            { "OpponentControllers", "Minimum", "0", false },
            { "OpponentControllers", "Maximum", "5", false },
            { "OpponentControllers", "KOEnabled", "5", false },
            { "OpponentControllers", "MaximumLANPlayers", "6", false },
            { "Drift", "MaximumMultiplier", "5", false },
            { "Drift", "PlusSign", "0", false },
            { "Drift", "ShowWithoutMultiplying", "0", false },
            { "Drift", "HideNiftyDriftMessages", "0", false },
            { "Menu", "ShowOutrun", "0", false },
            { "Menu", "ShowMoreRaceOptions", "1", false },
            { "Menu", "ShowOnline", "1", false },
            { "Menu", "ShowSubs", "0", false },
            { "Menu", "ShowSpecialVinyls", "1", false },
            { "Menu", "ShowDebugCarCustomize", "0", false },
            { "Menu", "ShowDebugEventID", "0", false },
            { "Menu", "AnyTrackInAnyRaceMode", "0", false },
            { "Menu", "FreeRunTrackSelect", "1", false },
            { "Menu", "OutrunTrackSelect", "1", false },
            { "Menu", "SplashScreenTimeLimit", "30", false },
            { "Menu", "ShowcaseCamInfiniteYRotation", "0", false },
            { "Menu", "AllowLongerProfileNames", "1", false },
            { "Menu", "DisableTakeover", "0", false },
            { "Gameplay", "EnableHiddenCameraModes", "0", false },
            { "Gameplay", "EnableDebugCamera", "0", false },
            { "Gameplay", "GameSpeed", "1", false },
            { "Gameplay", "WorldAnimationSpeed", "45", false },
            { "Gameplay", "HeadlightsMode", "2", false },
            { "Gameplay", "LowBeamBrightness", "0.75", false },
            { "Gameplay", "HighBeamBrightness", "1", false },
            { "Gameplay", "RemoveRaceBarriers", "0", false },
            { "Gameplay", "RemoveLockedAreaBarriers", "0", false },
            { "Gameplay", "ShowPercentOn1LapRaces", "1", false },
            { "Gameplay", "GameRegion", "0", false },
            { "Gameplay", "StartingCash", "0", false },
            { "Gameplay", "UnlockRegionalCars", "1", true },      // ExtraOptions 5.1.0.1340 [Gameplay] UnlockRegionalCars
            { "Gameplay", "UnlockAllThings", "0", false },
            { "Gameplay", "NeonBrightness", "1", false },
            { "Gameplay", "NosTrailRepeatCount", "8", false },
            { "Weather", "AlwaysRain", "0", false },
            { "Weather", "GeneralRainAmount", "1", false },
            { "Weather", "RoadReflectionAmount", "1", false },
            { "Weather", "RainSize", "0.03", false },
            { "Weather", "RainIntensity", "0.7", false },
            { "Weather", "RainCrossing", "0.02", false },
            { "Weather", "RainSpeed", "0.03", false },
            { "Weather", "RainGravity", "0.35", false },
            { "Fixes", "DisappearingWheelsFix", "1", false },
            { "Fixes", "ExperimentalSplitScreenFix", "0", false },
            { "Fixes", "DamageFix", "1", false },
            { "Fixes", "MegalowPartsFix", "1", false },
            { "Misc", "WindowedMode", "0", false },
            { "Misc", "SkipMovies", "0", false },
            { "Misc", "EnableSound", "1", false },
            { "Misc", "EnableMusic", "1", false },
            { "Misc", "AllowMultipleInstances", "0", false },
        },
        {},
    },
#ifdef DEV_SETTINGS
    {   // https://nfsmods.xyz/mod/5196, v1.1.1: no ini of its own, the port keeps the switches in BayviewIni
        BayviewName,
        "1.1.1",
        "moses",
        BayviewIni,
        {
            { "Cars", "GS300", "0", true },
            { "Cars", "CIVICEG", "0", true },
            { "Cars", "INFIFX45", "0", true },
        },
        {
            "https://nfsmods.xyz/uploads/mods/5196/3ac477-Bayview%27s_Best_Car_Pack_v1.1.1.7z",
            47593787,
            "b8072b5cbe47fb82eb4df2d3845c61788c7cff5f9cf39768077757c6733ed7e2",
            {
                { Bayview "GS300/GS300/GEOMETRY.BIN", "CARS/GS300/GEOMETRY.BIN", 8030252 },
                { Bayview "GS300/GS300/TEXTURES.BIN", "CARS/GS300/TEXTURES.BIN", 374336 },
                { Bayview "GS300/GS300/VINYLS.BIN", "CARS/GS300/VINYLS.BIN", 16193664 },
                { Bayview "GS300/GS300/PARTS_ANIMATIONS.bin", "CARS/GS300/PARTS_ANIMATIONS.BIN", 38352 },
                { Bayview "GS300/CarSoundData/CarTypeMapping/GS300.ini", "scripts/CarSoundData/CarTypeMapping/GS300.ini", 38 },
                { Bayview "GS300/UnlimiterData/GS300.ini", "scripts/UnlimiterData/GS300.ini", 4051 },
                { Bayview "CIVICEG/CIVICEG/GEOMETRY.BIN", "CARS/CIVICEG/GEOMETRY.BIN", 6309436 },
                { Bayview "CIVICEG/CIVICEG/TEXTURES.BIN", "CARS/CIVICEG/TEXTURES.BIN", 643456 },
                { Bayview "CIVICEG/CIVICEG/VINYLS.BIN", "CARS/CIVICEG/VINYLS.BIN", 14964032 },
                { Bayview "CIVICEG/CIVICEG/PARTS_ANIMATIONS.bin", "CARS/CIVICEG/PARTS_ANIMATIONS.BIN", 32624 },
                { Bayview "CIVICEG/CarSoundData/CarTypeMapping/CIVICEG.ini", "scripts/CarSoundData/CarTypeMapping/CIVICEG.ini", 37 },
                { Bayview "CIVICEG/UnlimiterData/CIVICEG.ini", "scripts/UnlimiterData/CIVICEG.ini", 4051 },
                { Bayview "FX45/INFIFX45/GEOMETRY.BIN", "CARS/INFIFX45/GEOMETRY.BIN", 2834384 },
                { Bayview "FX45/INFIFX45/TEXTURES.BIN", "CARS/INFIFX45/TEXTURES.BIN", 442240 },
                { Bayview "FX45/INFIFX45/VINYLS.BIN", "CARS/INFIFX45/VINYLS.BIN", 14445632 },
                { Bayview "FX45/INFIFX45/PARTS_ANIMATIONS.bin", "CARS/INFIFX45/PARTS_ANIMATIONS.BIN", 19488 },
                { Bayview "FX45/CarSoundData/CarTypeMapping/INFIFX45.ini", "scripts/CarSoundData/CarTypeMapping/INFIFX45.ini", 38 },
                { Bayview "FX45/UnlimiterData/INFIFX45.ini", "scripts/UnlimiterData/INFIFX45.ini", 4051 },
            },
        },
    },
#endif
};

#ifdef DEV_SETTINGS
#undef Bayview
const char* const BayviewDescription = "A modpack that features 4, made from scratch, vanilla styled NFSU2 cars.";
const char* const BayviewNote = "Experiment: tests the download only; the cars do not appear in the game yet.";
const char* const BayviewComment = "1 when the car's files are installed";
#endif

const Mod s_mods[Count] =
{
    {   // ExtraOptions 5.1.0.1340 [Gameplay] UnlockRegionalCars
        ExtraOptions,
        modreq::EuCarContent,
        "You can drive Peugeout 106 & Vauxhall Corsa in your NTSC game. Same deal with Acura RSX & Honda Civic in your "
        "PAL game. They will be available at Customize Menu and Career Car Lot. They also appear as AI opponents.",
        "1 lists the Corsa and the Peugeot 106 in the US game, 0 keeps them out",
        { "eu_content" },
    },
#ifdef DEV_SETTINGS
    { BayviewsBest, modreq::SettingCount, BayviewDescription, BayviewComment, { "bayview_cars" }, &s_gs300, BayviewNote },
    { BayviewsBest, modreq::SettingCount, BayviewDescription, BayviewComment, { "bayview_cars" }, &s_civic, BayviewNote },
    { BayviewsBest, modreq::SettingCount, BayviewDescription, BayviewComment, { "bayview_cars" }, &s_fx45, BayviewNote },
#endif
};

}

std::string pathIn(const std::string& folder, const char* relative)
{
    return folder + "/" + relative;
}

bool fileExists(const std::string& file)
{
    struct stat info;
    return stat(file.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

bool isOn(const std::string& value)
{
    const bool hex = value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X');
    return strtol(value.c_str(), nullptr, hex ? 16 : 10) != 0;
}

namespace
{

void readValue(const std::string& file, const Mod& mod, State* s)
{
    std::string text, value;
    const pf::TextFile kind = pf::readTextFile(file.c_str(), &text);
    s->iniFound = kind != pf::TextFile::Missing;
    s->unreadable = kind == pf::TextFile::Unreadable;
    // Extra Options reads the key with a default of 1: a missing section or key is on.
    s->on = kind == pf::TextFile::Readable &&
            (ini::get(file, def(mod).section, def(mod).key, &value) ? isOn(value) : onByDefault(mod));
}

}

bool onByDefault(const Mod& mod)
{
    for (const IniKey& key : s_sources[mod.source].keys)
        if (!strcmp(key.section, def(mod).section) && !strcmp(key.key, def(mod).key))
            return isOn(key.def);
    return true;
}

const Mod& get(Id id)
{
    return s_mods[id];
}

const Source& source(SourceId id)
{
    return s_sources[id];
}

void setDownload(SourceId id, const Download& download)
{
    s_sources[id].download = download;
}

uint64_t Download::installedBytes() const
{
    uint64_t total = 0;
    for (const FileLocation& file : files)
        total += file.bytes;
    return total;
}

std::string fileName(const Source& source)
{
    const char* slash = strrchr(source.iniPath, '/');
    return slash ? slash + 1 : source.iniPath;
}

std::string heading(const Source& source)
{
    return std::string(source.mod) + " " + source.version + " by " + source.author;
}

const char* cfgOverlap(const Mod& mod)
{
    for (const char* key : mod.overlaps)
    {
        if (pf::findSetting(pf::canonicalKey(key)))
            return key;
    }
    return nullptr;
}

namespace ini
{

bool get(const std::string& file, const char* section, const char* key, std::string* value)
{
    mINI::INIStructure data;
    if (!mINI::INIFile(file).read(data) || !data.has(section) || !data.get(section).has(key))
        return false;
    *value = data.get(section).get(key);
    return true;
}


}

State state(const Mod& mod, const std::string& gameFolder)
{
    State s;
    readValue(pathIn(gameFolder, s_sources[mod.source].iniPath), mod, &s);
    s.met = def(mod).requirementsMet(gameFolder);
    s.requirements = def(mod).requirementsString(gameFolder);
    return s;
}

bool runs(const Mod& mod, const std::string& gameFolder)
{
    const State s = state(mod, gameFolder);
    return s.on && s.met;
}

}}

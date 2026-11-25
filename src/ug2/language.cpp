// The game's language: the cfg key language picks one of the game's own languages when its files are in the game folder.

#include "language.h"
#include <lib/registry.h>
#include <platform/config.h>
#include <platform/log.h>
#include <cstring>
#include <dirent.h>
#include <strings.h>

namespace ug2 { namespace language {

const char* const Names[Count] = { "ENGLISH", "FRENCH", "GERMAN", "ITALIAN", "SPANISH", "DUTCH", "SWEDISH", "DANISH",
                                   "KOREAN", "CHINESE", "JAPANESE" };
const char* const RegistryNames[Count] = { "English US", "French", "German", "Italian", "Spanish", "Dutch", "Swedish",
                                           "Danish", "Korean", "Chinese (Traditional)", "Japanese" };

namespace
{

bool findEntry(const std::string& folder, const std::string& name, std::string* found)
{
    DIR* dir = opendir(folder.c_str());
    if (!dir)
        return false;
    bool matched = false;
    while (const dirent* entry = readdir(dir))
    {
        if (strcasecmp(entry->d_name, name.c_str()) == 0)
        {
            *found = entry->d_name;
            matched = true;
            break;
        }
    }
    closedir(dir);
    return matched;
}

bool hasFile(const std::string& gameFolder, const char* folder, const std::string& name)
{
    std::string directory, file;
    return findEntry(gameFolder, folder, &directory) && findEntry(gameFolder + "/" + directory, name, &file);
}

}

int indexOf(const char* value)
{
    for (int i = 0; i < Count; ++i)
    {
        if (strcasecmp(value, Names[i]) == 0)
            return i;
    }
    return -1;
}

bool available(const std::string& gameFolder, int index)
{
    if (index < 0 || index >= Count)
        return false;
    const std::string name = Names[index];
    return hasFile(gameFolder, "LANGUAGES", name + ".BIN") && hasFile(gameFolder, "memcard", "LOCALE_" + name + ".loc");
}

int choose(const char* value, const std::string& gameFolder)
{
    if (!*value || strcasecmp(value, "default") == 0)
        return -1;
    const int index = indexOf(value);
    if (index < 0)
    {
        pf::logError("language: '%s' is not a language of the game, the default stays", value);
        return -1;
    }
    if (!available(gameFolder, index))
    {
        pf::logError("language: LANGUAGES/%s.BIN or memcard/LOCALE_%s.loc is not in the game folder, the default stays",
                     Names[index], Names[index]);
        return -1;
    }
    return index;
}

void configure(const std::string& gameFolder)
{
    const int index = choose(pf::Config::getString("language", "default"), gameFolder);
    if (index < 0)
    {
        pf::logInfo("language: default");
        return;
    }
    win32::registry::Key* game = win32::registry::find(win32::registry::root(win32::HKEY_LOCAL_MACHINE),
                                                       "Software\\EA Games\\Need for Speed Underground 2", true);
    const char* value = RegistryNames[index];
    win32::registry::setOverride(game, "Language", win32::registry::REG_SZ, value, x86::reg32(strlen(value) + 1));
    pf::logInfo("language: %s (LANGUAGES/%s.BIN)", Names[index], Names[index]);
}

}}

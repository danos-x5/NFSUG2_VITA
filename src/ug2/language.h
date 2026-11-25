#ifndef UG2_LANGUAGE_H_
#define UG2_LANGUAGE_H_

#include <string>

// The game's language: the cfg key language picks one of the game's own languages when its files are in the game folder.

namespace ug2 { namespace language {

const int Count = 11;
extern const char* const Names[Count];          // the game's table at 0x7F7020, by language index
extern const char* const RegistryNames[Count];  // the registry's Language value that gives each one

// Language index a value names, or -1.
int indexOf(const char* value);
// Whether the language's files are in the game folder.
bool available(const std::string& gameFolder, int index);
// Index for the game, or -1 for its own.
int choose(const char* value, const std::string& gameFolder);
// After the registry loads: gives the game the chosen language.
void configure(const std::string& gameFolder);

}}

#endif /* !UG2_LANGUAGE_H_ */

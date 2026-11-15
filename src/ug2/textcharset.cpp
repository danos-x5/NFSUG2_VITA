// Which characters the game's line editor accepts for each editor type (filter at 0x4FF310).

#include "textentry.h"

namespace ug2 { namespace textentry {

const char* extraChars(uint32_t type)
{
    switch (type)
    {
    case 4:
        return "@._";
    case 5:
        return " ,@.";
    case 0:
    case 3:
        return " .,_@-`=[]\\;'/<>?:\"{}|+()*&%#!~";
    default:
        return "";
    }
}

const char* title(uint32_t type)
{
    return type == 6 ? "Profile name" : "Text";
}

}}

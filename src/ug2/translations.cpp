// Texts the port added that the game's language files lack, in the game's languages by their English wording.

#include "translations.h"
#include <cstring>

namespace ug2 { namespace translations {

// Wording follows the game's own: Steer, Brake / Reverse, Left, Right, Customize and Sensitivity in its language files.
const Entry Table[] =
{
    { "Steer Left", { "Direction gauche", "Links lenken", "Sterzo a sinistra", "Girar a la izquierda", "Naar links sturen",
                      "Styr vänster", "Styr til venstre" } },
    { "Steer Right", { "Direction droite", "Rechts lenken", "Sterzo a destra", "Girar a la derecha", "Naar rechts sturen",
                       "Styr höger", "Styr til højre" } },
    { "Free Look", { "Regard libre", "Freie Kamera", "Visuale libera", "Vista libre", "Vrij rondkijken", "Fri blick",
                     "Frit udsyn" } },
    { "Brake", { "Freiner", "Bremsen", "Freno", "Frenar", "Remmen", "Bromsa", "Brems" } },
    { "Left Dead Zone", { "Zone morte gauche", "Tote Zone links", "Zona morta sinistra", "Zona muerta izquierda",
                          "Dode zone links", "Dödzon vänster", "Dødzone venstre" } },
    { "Left Outer Limit", { "Limite extérieure gauche", "Außengrenze links", "Limite esterno sinistro",
                            "Límite exterior izquierdo", "Buitengrens links", "Yttre gräns vänster",
                            "Yderste grænse venstre" } },
    { "Left Sensitivity", { "Sensibilité gauche", "Empfindlichkeit links", "Sensibilità sinistra", "Sensibilidad izquierda",
                            "Gevoeligheid links", "Känslighet vänster", "Følsomhed venstre" } },
    { "Right Dead Zone", { "Zone morte droite", "Tote Zone rechts", "Zona morta destra", "Zona muerta derecha",
                           "Dode zone rechts", "Dödzon höger", "Dødzone højre" } },
    { "Right Outer Limit", { "Limite extérieure droite", "Außengrenze rechts", "Limite esterno destro",
                             "Límite exterior derecho", "Buitengrens rechts", "Yttre gräns höger", "Yderste grænse højre" } },
    { "Right Sensitivity", { "Sensibilité droite", "Empfindlichkeit rechts", "Sensibilità destra", "Sensibilidad derecha",
                             "Gevoeligheid rechts", "Känslighet höger", "Følsomhed højre" } },
    { "Custom", { "Personnalisé", "Benutzerdefiniert", "Personalizzato", "Personalizado", "Aangepast", "Anpassad",
                  "Tilpasset" } },
    { "Touchscreen", { "Écran tactile", "Touchscreen", "Touchscreen", "Pantalla táctil", "Touchscreen", "Pekskärm",
                       "Berøringsskærm" } },
};
const int TableCount = int(sizeof(Table) / sizeof(Table[0]));

namespace
{

int s_language = 0;

std::string latin1(const char* utf8)
{
    std::string out;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8); *p; ++p)
    {
        if (*p < 0x80)
            out += char(*p);
        else if ((*p & 0xe0) == 0xc0 && (p[1] & 0xc0) == 0x80)
        {
            const unsigned code = unsigned(*p & 0x1f) << 6 | (p[1] & 0x3f);
            out += code <= 0xff ? char(code) : '?';
            ++p;
        }
        else
            out += '?';
    }
    return out;
}

}

void setLanguage(int language)
{
    s_language = language;
}

int language()
{
    return s_language;
}

std::string translate(const char* english, int language)
{
    if (language >= 1 && language <= 7)
    {
        for (const Entry& entry : Table)
        {
            if (strcmp(entry.english, english) == 0 && *entry.text[language - 1])
                return latin1(entry.text[language - 1]);
        }
    }
    return english;
}

std::string translate(const char* english)
{
    return translate(english, s_language);
}

}}

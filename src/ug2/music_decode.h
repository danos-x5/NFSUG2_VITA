#ifndef UG2_MUSIC_DECODE_H_
#define UG2_MUSIC_DECODE_H_

#include <lib/winapp.h>

// Music on the Vita's hardware MP3 decoder (music_hw_decode): EALayer3 granules repacked into MPEG-1 frames.
namespace ug2 { namespace music
{

// Hooks the MPEG decoder when music_hw_decode=on; before guest code runs.
void configure(win32::WinApplication& app);

}}

#endif /* !UG2_MUSIC_DECODE_H_ */

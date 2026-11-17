#ifndef PF_SWITCHES_H_
#define PF_SWITCHES_H_

// Build switches NATIVE_FUNCTIONS, AUDIO_FAST_PATHS and RENDER_FAST_PATHS (0 or 1) fix each choice at compile time;
// with DEV_SETTINGS the settings file decides, key by key, and the build switch is only the default.

#include <platform/cached.h>
#include <platform/config.h>

#ifdef DEV_SETTINGS
#define NFSU_SWITCH(key, on) (pf::Config::getBool(key, (on) != 0))
#define NFSU_SWITCH_CACHED(key, on) (pf::cachedStatic([] { return pf::Config::getBool(key, (on) != 0); }))
#else
#define NFSU_SWITCH(key, on) ((on) != 0)
#define NFSU_SWITCH_CACHED(key, on) ((on) != 0)
#endif

#define NFSU_NATIVE(key) NFSU_SWITCH_CACHED(key, NATIVE_FUNCTIONS)
#define NFSU_NATIVE_SETTING(key) NFSU_SWITCH(key, NATIVE_FUNCTIONS)
#define NFSU_AUDIO_FAST(key) NFSU_SWITCH(key, AUDIO_FAST_PATHS)
#define NFSU_RENDER_FAST(key) NFSU_SWITCH(key, RENDER_FAST_PATHS)
#define NFSU_RENDER_FAST_CACHED(key) NFSU_SWITCH_CACHED(key, RENDER_FAST_PATHS)

#endif /* !PF_SWITCHES_H_ */

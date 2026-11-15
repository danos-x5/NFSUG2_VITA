#ifndef WINAPI_WAITPROBE_H_
#define WINAPI_WAITPROBE_H_

// wait_probe=on: blocked waits and signals per second, by thread, guest caller and handle.

#include <cstdint>

namespace waitprobe
{

enum Kind : uint8_t { Wait, Signal };

bool enabled();
void record(Kind kind, uint32_t thread, uint32_t caller, uint32_t handle, uint32_t us, bool timedOut);

}

#endif

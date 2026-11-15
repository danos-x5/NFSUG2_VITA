#ifndef UG2_DRIVE_H_
#define UG2_DRIVE_H_

#include <cstddef>
#include <cstdint>

// Blind driver for the free-roam benchmark: synthetic pad input (throttle, random steering bursts, reversing out
// when stuck) from a seeded generator, so runs repeat.

namespace ug2 { namespace drive {

// Before guest code runs: installs the pad poll hook.
void configure(uint32_t seed);

// Game thread: player 0's car is ready to drive, or no longer is.
void start();
void stop();

// Any thread: player 0's position, speed and driving state as text.
void describe(char* text, size_t size);

}}

#endif /* !UG2_DRIVE_H_ */

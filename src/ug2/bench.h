#ifndef UG2_BENCH_H_
#define UG2_BENCH_H_

#include <cstddef>

// Boot race benchmark: frame times from the race start to the finish, logged as "bench: summary".

namespace ug2 { namespace bench {

// Before guest code runs: benchmark_seconds, benchmark_window and benchmark_exit, with their defaults.
void configure(int defaultSeconds, int defaultWindow);

// Render thread, at each window: text appended to its log line.
typedef void (*WindowNote)(char* text, size_t size);
void setWindowNote(WindowNote note);

// Game thread: the boot race started.
void raceStarted();

// Game thread: the boot race finished or was left.
void raceEnded(const char* reason);

// Any thread: the boot race's measurement has ended (time limit or finish).
bool measurementOver();

}}

#endif /* !UG2_BENCH_H_ */

#ifndef PLATFORM_LOG_H_
#define PLATFORM_LOG_H_

namespace pf
{

// A log file under the data directory, plus console output when enabled.
void logInit(const char* logFilePath);
void logShutdown();

// Console output on or off (off by default).
void logSetConsole(bool enabled);

// Info lines on or off (on by default); errors and fatals are always written.
void logSetInfo(bool enabled);

// Info lines are written by a writer thread, not the caller.
void logSetDeferred(bool enabled);
// Writes the deferred lines now.
void logFlush();

void logDebug(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logInfo(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logError(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Appends text, complete lines that already carry their "[level] " prefix and line ends, with one file write.
void logBlock(const char* text, unsigned length);

// logBlock with direct sceIo calls, bypassing the log mutex (stall reports).
void logBlockUnlocked(const char* text, unsigned length);
int logMutexId();
const char* logFilePath();

// Logs, flushes and terminates the process.
[[noreturn]] void fatal(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}

#endif

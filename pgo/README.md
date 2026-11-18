# PGO profile

`nfsu2.proftext` is the clang profile the Vita build uses by default (`NFSU_PGO_DEFAULT=ON`; `-DNFSU_PGO_DEFAULT=OFF` or `-DNFSU_PGO_USE=<indexed .profdata>` to change it). It is the text form with the checkout path replaced by `@NFSU_SOURCE_DIR@`: clang names file-local functions by their absolute source path, so CMake writes a copy with the real path into the build folder and indexes it with `llvm-profdata` at configure time.

Recorded 2026-11-19 on a real Vita from the perf-integration branch (generated with `--float-align all --inline-ftol2 --pure-natives --native-writes`, as `tools/build.py` does): instrumented build (`-DNFSU_PGO_GENERATE=ON`), five runs with audio on and `benchmark_exit=on`: the start of a Resort Loop race (`boot_track=4014`, 3 AI, vita graphics, `music_hw_decode=on`, `audio_mixer_split=on`, 60 s), a URL race (`boot_mode=url`, track 4711, 5 AI, 60 s), a full Resort Loop race with the default settings (3 AI, 257 s), free roam with the blind driver (`boot_mode=freeroam`, 90 s), and the front end into the garage (`boot_steps`, `boot_quit_after=150`). Each run's `ux0:data/NeedForSpeedUnderground2/nfsu.profraw` was fetched, the five merged unweighted with `llvm-profdata merge`, then written as text with `llvm-profdata merge --text` and the absolute source path replaced by `@NFSU_SOURCE_DIR@`. By counts the full race is about half of the profile.

Build in a clean folder after changing the profile: objects compiled against another profile do not link (ThinLTO reports conflicting `ProfileSummary` values).

Refresh it after large code changes, especially after regenerating `src/ug2/generated`: functions whose code changed lose their profile data.

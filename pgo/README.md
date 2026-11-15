# PGO profile

`nfsu2.proftext` is the clang profile the Vita build uses by default (`NFSU_PGO_DEFAULT=ON`; `-DNFSU_PGO_DEFAULT=OFF` or `-DNFSU_PGO_USE=<indexed .profdata>` to change it). It is the text form with the checkout path replaced by `@NFSU_SOURCE_DIR@`: clang names file-local functions by their absolute source path, so CMake writes a copy with the real path into the build folder and indexes it with `llvm-profdata` at configure time.

Recorded 2026-11-15 on a real Vita from the code at commit 61db254: instrumented build (`-DNFSU_PGO_GENERATE=ON`), four runs at `gfx_level=3` with audio on and the AI driving, `benchmark_exit=on`: a city circuit with traffic (`boot_mode=race`, track 4085, 3 AI, `boot_traffic_level=3`, 90 s), and the first 20 s of a drag race (`boot_mode=drag`), a drift race (`boot_mode=drift`) and a URL race (`boot_mode=url`, track 4711, 5 AI). Each run's `ux0:data/NeedForSpeedUnderground2/nfsu.profraw` was fetched, the four merged unweighted with `llvm-profdata merge`, then written as text with `llvm-profdata merge --text` and the absolute source path replaced by `@NFSU_SOURCE_DIR@`. By frames the circuit is about 63% of the profile.

Measured on the Vita against the profile it replaced (URL race only, vita preset, recorded 2026-11-13), same code, fixed workload of 1500 frames at `gfx_level=3`, audio off: URL race 33.85 -> 32.99 ms a frame, city circuit 29.30 -> 28.57 ms; frames over 41.7 ms 171 -> 101 and 25 -> 11.

Build in a clean folder after changing the profile: objects compiled against another profile do not link (ThinLTO reports conflicting `ProfileSummary` values).

Refresh it after large code changes, especially after regenerating `src/ug2/generated`: functions whose code changed lose their profile data. Free roam and the menus are not in the profile.

# PROJECT SAVE FILE (resume point)

Last updated: 2026-08-12 (session with "opencode"/big-pickle)

## Goal
Port `AstroBinCSVGenerator.js` v1.2.5 into a native C++ PCL module
(`module/AstroBinCSVGenerator.pxm.dll`) with byte-identical CSV output
vs. the JS reference, then package and commit.

## Constraints / preferences
- Zero external deps: hand-written JSON parser in `module/ABCGJSON.h` (`std::string`-based, NOT pcl::String).
- Build: MSVC v143 via `C:\PCL\build-module.cmd` -> `C:\PCL\bin\x64\AstroBinCSVGenerator-pxm.dll`.
- Deploy: `C:\PCL\deploy-module.ps1` copies to `C:\Program Files\PixInsight\bin`. NOT self-elevating;
  requires PixInsight CLOSED + UAC. Elevated launch pattern (works):
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName='powershell.exe'; $psi.Arguments='-NoProfile -ExecutionPolicy Bypass -File "C:\PCL\deploy-module.ps1"'; $psi.Verb='RunAs'; $psi.UseShellExecute=$true
    [System.Diagnostics.Process]::Start($psi).WaitForExit()
  Success is written to `C:\PCL\deploy.log` ("DEPLOY_OK").
- Reference harness runs on Node v22.14.0 (V8 TimSort). `tests/make_test_fits.py` is pure Python (no astropy).
- The unsigned module loads/executes on this machine (no signing gate seen); live GUI testing is the validation path.
- USER DEFERRED COMMIT EARLIER ("Don't commit yet") -> MUST ask again before committing.

## Current status (resume point)
- **Crash root cause found and FIXED.** Engine rebuilt (hash `0A9DBA9B...`) and DEPLOYED
  (deployed DLL hash verified = `0A9DBA9B...`; old DBG build was `69BC7617...`, original release `E5D5EB53...`).
- **AWAITING USER**: relaunch PixInsight, re-run Generate on `C:/Users/KC3EL/Desktop/New folder`
  (12 FITS files / 2 groups). Then compare produced CSV against `tests/real_expected.csv` (must be byte-identical).
- If more files needed: `tests/expected.csv` is the 37-file/17-group golden.

## ROOT CAUSE (critical)
PCL 2.10.4 `pcl::GenericString::DetachFromData()` (String.h:4271) does `m_data->Detach()` with NO null check.
Its move ctor sets `s.m_data = nullptr` (String.h:565). `std::sort` / `std::swap` on containers of
`pcl::String` (or structs containing String members) move-assign into the moved-from object ->
`Transfer()` -> `DetachFromData()` -> NULL deref (AV at 0x0, 0xC0000005). Dtor IS null-safe (String.h:660-666),
so plain vector moves/reallocation do NOT crash. `std::vector<String>` growth is SAFE.

Crash chain that started the hunt: `Generate()` -> `LoadFilterDatabase()` (OK) ->
`CollectFiles()` -> `std::sort(found)` on `std::vector<pcl::String>` -> AV.
Full proof chain in `C:\Users\KC3EL\AppData\Local\Temp\abcg_asan\` (ASAN parser harness clean,
`find_test.cpp` standalone repro with real PCL, `sort_test.cpp` minimal crash, `seh_test.cpp` SEH/DbgHelp stack dump).

## Fix applied (module/AstroBinCSVGeneratorEngine.cpp)
Replaced std::sort/std::stable_sort on String-bearing elements with INDEX-BASED sorts
(sort `std::vector<size_t>` indices, then copy-construct new container; String copy-ctor is refcount-safe):
- `CollectFiles`: `std::sort( found.begin(), found.end() )` -> index sort (now ~line 2015).
- `AggregateFrames`: `std::stable_sort( results.begin(), results.end(), comparator )` -> index stable sort
  with identical comparator semantics (sessionDate, then filter, then gain) (~line 1879).
- `DetectSessions` line 1734 `std::stable_sort( dated, ... )` sorts `FrameData*` POINTERS -> already safe, untouched.
- Removed all temporary `DBG:` Console instrumentation.
- Verified: no `std::swap/rotate/reverse/unique/nth_element/partition/remove_if` on String types anywhere in module;
  remaining `std::move` uses are whole-container moves (safe) or ABCGJSON std::string moves (safe).
- Standalone validated (modes 12/13 of seh_test.exe): correct ordering + stability, no crash.

## Module file layout
- `module/AstroBinCSVGeneratorEngine.cpp/.h` — engine logic (ported JS).
- `module/ABCGJSON.h` — hand-written JSON parser (std::string based).
- `module/AstroBinCSVGeneratorModule.cpp` — version 1/2/5, banner.
- `module/windows/vc17/AstroBinCSVGenerator.vcxproj` — build config (`/O2 /arch:AVX2 /D__PCL_AVX2 /D__PCL_FMA`).
- `module/*.h/.cpp` — Interface, Instance, Process, Parameters, etc.

## Parity / tests
- `tests/expected.csv` (37 files/17 groups) and `tests/real_expected.csv` (12 files/2 groups) — goldens; regenerate with ZERO git diff.
- `tests/inspect_real.py` — inspects real test data.
- Node v22.14.0 = reference V8; v22 regex/Intl not used (plain operations only).

## Packaging (done)
- `docs/MODULE.md`, README module section, `build-module-package.ps1` -> `dist/AstroBinCSVGenerator-1.2.5.zip` (`dist/` gitignored).

## Other facts
- Filter DB: `C:\Users\KC3EL\PixInsight\AstroBinFilters.json` (432,215 B; lastUpdated=2026-08-11T17:42:37.503Z; 2494 filters). Parser clean under ASAN.
- Test data dir: `C:/Users/KC3EL/Desktop/New folder` (14 items, 12 FITS).
- PCL build is StandardAllocator (`PCL-pxi.lib`); free-list NOT the issue, plain null deref is.
- ASAN cannot link against non-instrumented `PCL-pxi.lib` (LNK2038 x70) — expected; use SEH/DbgHelp dumps for local debugging.

## Next steps
1. USER: run PixInsight test (see Current status). Expect: module loads, "filter DB loaded 2494", no crash, CSV written.
2. Diff produced CSV vs `tests/real_expected.csv`; if identical, run the 37-file suite too (compare vs `tests/expected.csv`).
3. If any mismatch: reconcile (JS harness regen with `node`, then fix parity).
4. Rebuild package zip if needed (`build-module-package.ps1`) since DLL changed after 1.2.5 zip was made.
5. ASK USER before committing; commit message style = short imperative (match repo history).

# Migration Plan: C:\PCL → AstrobinPixinsight repo

Status: APPROVED by user (2026-08-13)
Decisions: (1) migrate source/tooling only — everything someone needs to set up and
build for themselves; (2) new content lives under `tools/`; (3) scripts rewritten to
repo-relative paths (repo becomes the workspace); (4) delete C:\PCL after the new
toolchain build is verified; (5) perform a full rebuild verification during migration.

## Goal
Bring the valuable source/tooling out of the local 3.3 GB build workspace `C:\PCL`
(not a git repo, no history) into the main repo so anyone can clone and build the
AstroBinCSVGenerator PixInsight module. Exclude regenerable build artifacts and
transient diagnostics. C:\PCL becomes legacy and is deleted after verification.

## Key facts / constraints
- Repo: C:\Users\KC3EL\AstrobinPixinsight, branch `Process-Icon`, remotes
  `origin` (Robowarrior834/AstrobinPixinsight) and `upstream` (SteveGreaves/AstroBinUploader).
- C:\PCL holds 471 files / 3.3 GB. Everything under `build/`, `lib/x64/`, `bin/x64/`,
  `crash/`, `dumpstack` binaries (exe/ilk/pdb/dmp/txt), and all top-level `*.txt`/`*.log`
  is regenerable/transient → EXCLUDED.
- The module vcxproj (`module/windows/vc17/AstroBinCSVGenerator.vcxproj`) is already
  env-var driven: it reads `$(PCLINCDIR)`, `$(PCLSRCDIR)`, `$(PCLLIBDIR64)`,
  `$(PCLBINDIR64)`; `OutDir`/`OutputFile` = `$(PCLBINDIR64)\...`. No vcxproj change needed;
  only the .cmd wrappers that set these vars need rewriting.
- build-module.cmd currently passes `/p:IntDir=C:\PCL\build\AstroBinCSVGenerator\Release\`.
- Existing repo files that hardcode `C:\PCL` paths (11 refs) must be updated:
  - `build-module-package.ps1:19,25`
  - `PROGRESS.md:12-18`
  - `README.md:21`
  - `docs/MODULE.md:15,18,21`
  - `module/AstroBinCSVGeneratorInterface.cpp:50` (comment only)
- `.gitignore` already ignores `build/`, `lib/`, `lib64/`, `dist/`, `*.log` at any depth
  (no leading slash), so `tools/build` and `tools/lib` are covered automatically.
- PROGRESS.md note: MUST ask user before committing.

## Phase 1 — Migrate tooling into tools/
Create `tools/` in the repo root and add:

| New repo file | Source (C:\PCL) | Action |
|---|---|---|
| `tools/build-toolchain.cmd` | `build-toolchain.cmd` | Rewrite repo-relative |
| `tools/build-module.cmd` | `build-module.cmd` | Rewrite repo-relative |
| `tools/deploy-module.ps1` | `deploy-module.ps1` | Rewrite repo-relative |
| `tools/check-module.js` | `check-module.js` | Copy as-is |
| `tools/find-holder.ps1` | `find-holder.ps1` | Copy as-is |
| `tools/dumpbin-module.cmd` | `dumpbin-module.cmd` | Rewrite DLL path to `%~dp0bin\x64\...` |
| `tools/dumpbin-imports.cmd` | `dumpbin-imports.cmd` | Rewrite DLL path to `%~dp0bin\x64\...` |
| `tools/ref/**` (21 files) | `ref/` | Copy as-is (Binarize*, NetworkService* + vcxproj, NoOperation*, FluxCalibrationInterface.cpp) |
| `tools/dumpstack/build.cmd` | `dumpstack/build.cmd` | Rewrite paths to `%~dp0` |
| `tools/dumpstack/test.cmd` | `dumpstack/test.cmd` | Rewrite paths to `%~dp0` |
| `tools/dumpstack/dumpstack.cpp` | `dumpstack/dumpstack.cpp` | Copy as-is |
| `tools/dumpstack/testcrash.cpp` | `dumpstack/testcrash.cpp` | Copy as-is |

### Script rewrite details

**tools/build-toolchain.cmd**
- Locate script dir: `set "PCLROOT=%~dp0"` (tools/), derive outputs:
  `set "BUILDDIR=%PCLROOT%build"`, `set "PCLLIBDIR64=%PCLROOT%lib\x64"`,
  `set "PCLBINDIR64=%PCLROOT%bin"`.
- VS install: try `vswhere` first, e.g.
  `"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`,
  fall back to `C:\Program Files\Microsoft Visual Studio\2022\Preview`, else error out.
  Then `call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat"`.
- PixInsight source: `set "PI=%PIXINSIGHT_HOME%"`, default `C:\Program Files\PixInsight`;
  `PCLINCDIR=%PI%\include`, `PCLSRCDIR=%PI%\src`,
  `PCL3RDPARTY=%PI%\src\3rdparty`.
- Keep the original loop: build `cminpack lcms lz4 RFC6234 zlib zstd` via msbuild,
  then `%PCLSRCDIR%\pcl\windows\vc17\PCL.vcxproj`, each with
  `/t:Build -m /p:Configuration=Release /p:Platform=x64 /p:IntDir=%BUILDDIR%\%%L\Release\`.
- Print output location + `dir %PCLLIBDIR64%`.

**tools/build-module.cmd**
- Same VS + PixInsight detection as above.
- `set "PCLROOT=%~dp0"`, `set "PCLLIBDIR64=%PCLROOT%lib\x64"`,
  `set "PCLBINDIR64=%PCLROOT%bin\x64"`,
  `set "BUILDDIR=%PCLROOT%build\AstroBinCSVGenerator"`.
- `msbuild "%PCLROOT%..\module\windows\vc17\AstroBinCSVGenerator.vcxproj" /t:Build -m /p:Configuration=Release /p:Platform=x64 /p:IntDir=%BUILDDIR%\Release\ /v:m %*`

**tools/deploy-module.ps1**
- `$Dll = Join-Path $PSScriptRoot 'bin\x64\AstroBinCSVGenerator-pxm.dll'`
- Copy to `C:\Program Files\PixInsight\bin\AstroBinCSVGenerator-pxm.dll`, verify length,
  write `DEPLOY_OK` to `Join-Path $PSScriptRoot 'deploy.log'`.

**tools/dumpstack/build.cmd & test.cmd**
- Replace every `C:\PCL\dumpstack\...` with `%~dp0...` (fe/exe outputs and inputs).
- Keep `vcvars64.bat` detection consistent with the other scripts (accept hardcoded Preview
  path for now, matching original; or reuse the vswhere snippet).

## Phase 2 — .gitignore + stray artifact cleanup
Append to `.gitignore`:
- `tools/bin/`
- `*.obj`
- `*.pdb`
- `*.ilk`
- `*.dmp`
- `tools/dumpstack/*.exe`

Delete the stray untracked files at repo root: `dumpstack.obj`, `testcrash.obj`,
`vc140.pdb` (build droppings; they become ignored by the new rules anyway).

## Phase 3 — Fix existing C:\PCL references
- `build-module-package.ps1`:
  - :19 → `$DllPath = Join-Path $PSScriptRoot 'tools\bin\x64\AstroBinCSVGenerator-pxm.dll'`
  - :25 error message → "run tools\build-module.cmd first"
- `PROGRESS.md` (:12-18): rewrite build/deploy steps to
  `tools\build-module.cmd` → `tools\bin\x64\...dll`, `tools\deploy-module.ps1`,
  success log at `tools\deploy.log`. Keep the elevated PowerShell launch pattern but
  point it at the repo script.
- `README.md:21`: `tools\build-module.cmd` → `tools\bin\x64\AstroBinCSVGenerator-pxm.dll`
- `docs/MODULE.md` (:15,:18,:21): same repo-relative path updates.
- `module/AstroBinCSVGeneratorInterface.cpp:50`: update the comment's diagnostics log path
  from `C:\PCL\module-diagnostics.log` to a repo-relative one (e.g. `tools\module-diagnostics.log`).

## Phase 4 — Add tools/SETUP.md
Document for someone building from a fresh clone:
- Prerequisites: PixInsight 1.9.4 installed WITH source + include dirs
  (`C:\Program Files\PixInsight\src` and `\include`); Visual Studio 2022 with
  MSVC v143 (C++ workload); optional `PIXINSIGHT_HOME` env override.
- Build steps:
  1. `tools\build-toolchain.cmd` — builds PCL-pxi.lib + 3rdparty libs into
     `tools\lib\x64` (long). Outputs: `tools\build`, `tools\lib\x64`, `tools\bin`.
  2. `tools\build-module.cmd` — builds `tools\bin\x64\AstroBinCSVGenerator-pxm.dll`.
  3. `tools\deploy-module.ps1` — elevated copy of the DLL into
     `C:\Program Files\PixInsight\bin` (PixInsight must be CLOSED; success = `DEPLOY_OK` in `tools\deploy.log`).
- Notes: vcxproj defines `__PCL_AVX2`/`__PCL_FMA` — requires an AVX2/FMA-capable CPU
  (edit the vcxproj if not). Unsigned module loads locally; distribution requires
  PixInsight module signing keys (see README).
- Mention `tools/ref/` (PixInsight reference module sources) and `tools/dumpstack/`
  (crash-dump stack-walker test harness) for developers.

## Phase 5 — Full rebuild verification (user approved)
1. `tools\build-toolchain.cmd` — expect success + `PCL-pxi.lib` etc. in `tools\lib\x64`.
2. `tools\build-module.cmd` — expect `tools\bin\x64\AstroBinCSVGenerator-pxm.dll`.
3. `tools\deploy-module.ps1` — expect `DEPLOY_OK` in `tools\deploy.log`.
(Existing local PCL toolchain build means libs already exist under the new tools/lib; if
toolchain libs exist at C:\PCL\lib\x64, the FULL rebuild step rebuilds them under tools/.)
Optionally verify DLL loads in PixInsight.

## Phase 6 — Teardown & commit
1. After verification passes, delete `C:\PCL` (user approved).
2. `git status` review; stage only migration-related files (the existing uncommitted
   module changes — the crash fix — remain separate and untouched).
3. ASK the user before committing (per PROGRESS.md). Commit message style: short
   imperative, matching repo history (e.g. "migrate PCL toolchain into tools/").

## Explicitly excluded from migration (stay out of git)
- `C:\PCL\build\**` — all .obj/.lib/.recipe/.tlog intermediates
- `C:\PCL\lib\x64\**` — compiled .lib files
- `C:\PCL\bin\x64\**` — compiled DLL + icons copy (icon is already in module/icons/)
- `C:\PCL\crash\**` — crash dumps (.dmp)
- `C:\PCL\dumpstack\*.exe/.ilk/.pdb/.dmp/*.txt`, `build.log`, `out.txt`, `pi.txt`,
  `selfout.txt`, `selftest.dmp`, `test.dmp`, `wer.txt`
- All top-level `C:\PCL\*.txt` and `*.log` (deploy.log, dumps-reg*.txt, gui-diag.log,
  module-diagnostics.log, pi-err.txt, run-err*.txt, etc.)

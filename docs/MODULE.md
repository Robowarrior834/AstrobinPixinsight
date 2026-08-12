# AstroBin CSV Generator — Native Process Module

A native C++ (PCL 2.10.4) process module that implements the AstroBin CSV
Generator **v1.2.5** algorithm as a byte-for-byte port of the JavaScript script.
It reads FITS/XISF light frames and writes an AstroBin-compatible acquisition
CSV file.

- Module version: **1.2.5** (matches the ported JS algorithm version)
- Platform: Windows x64 (PixInsight 1.8.8+)
- License: GPL v3

## Build (maintainers)

Prerequisites: Visual Studio 2022 (v143), PixInsight SDK / PCL headers on
`C:\PCL`, and the `build-module.cmd` wrapper at `C:\PCL`.

```
C:\PCL\build-module.cmd
```

Output: `C:\PCL\bin\x64\AstroBinCSVGenerator-pxm.dll`

The version number lives in `module\AstroBinCSVGeneratorModule.cpp`
(`MODULE_VERSION_*` defines). Bump the header comments in `module\*.cpp/h` and
the `$ModuleVersion` variable in `build-module-package.ps1` in sync.

## Install (users)

1. Close PixInsight completely.
2. Copy `AstroBinCSVGenerator-pxm.dll` into the PixInsight `bin` folder
   (e.g. `C:\Program Files\PixInsight\bin`).
3. Restart PixInsight.

The module appears under **Process → Utility → AstroBin CSV Generator**.

> **Signing:** PixInsight refuses to load **unsigned** modules. A properly
> signed module embeds a PixInsight-specific signature that is not an
> Authenticode signature, so ordinary code-signing checks do not apply. Sign
> the DLL with your license-based PixInsight module signing keys before
> distributing it (see the signing section in the root `README.md`). An
> unsigned DLL can still be copied in for local testing on a machine with
> "Load unsigned modules" warnings disabled, but this is not recommended.

## Usage

Select the process under **Process → Utility → AstroBin CSV Generator**, or run
it as an instance from another process. Configure the parameters, then execute.

### Parameters

| Parameter            | Type    | Default     | Description |
| -------------------- | ------- | ----------- | ----------- |
| `inputDirectory`     | string  | —           | Folder containing the FITS/XISF light frames. |
| `outputDirectory`    | string  | input dir   | Folder where the CSV is written. |
| `outputFileName`     | string  | `acquisition.csv` | Name of the generated CSV. |
| `recursive`          | bool    | `true`      | Recurse into subfolders. |
| `sessionGapHours`    | double  | `5`         | Minimum gap (h) between frames that starts a new session. |
| `shiftOvernight`     | bool    | `true`      | Shift sessions that start before local noon to the previous calendar day (module extension; the JS script always shifts unless `useObservingDate` is set). |
| `useObservingDate`   | bool    | `false`     | Use the observed date as-is instead of applying the overnight shift. |
| `defaultGain`        | int     | `0`         | Gain used when frames have no `GAIN` header. |
| `defaultTemperature` | double  | `-10`       | Cooling used when frames have no `CCD-TEMP` header. |
| `defaultFilter`      | string  | —           | Filter label used when `useDefaultFilter` is enabled. |
| `useDefaultFilter`   | bool    | `false`     | Apply `defaultFilter` when frames carry no filter. |
| `keywordOverrides`   | string  | `{}`        | JSON `{ "canonical": "alternative" }` mapping (e.g. `{ "GAIN": "ISO" }`); the canonical keyword is read from the alternative one when the canonical is absent. |
| `filterMap`          | string  | —           | JSON `{ "name": "id" }` extra filter→AstroBin-ID map (defaults are built in). |
| `filterDatabasePath` | string  | —           | Path to AstroBin's filter database JSON (e.g. PixInsight's `AstroBinFilters.json`) used for ID resolution. |
| `overrideFilePath`   | string  | —           | Optional per-file overrides CSV (see below). |
| `siteName`           | string  | `My Site`   | Site name. |
| `siteLatitude`       | double  | `0`         | Site latitude. |
| `siteLongitude`      | double  | `0`         | Site longitude. |
| `siteElevation`      | double  | `0`         | Site elevation. |
| `bortle`             | int     | `4`         | Bortle scale used when frames carry no `BORTLE` header. |
| `sqm`                | double  | `21`        | SQM value used when frames carry no `SQM` header. |
| `focalLength`        | double  | `540`       | Focal length (mm) used when frames carry no `FOCALLEN` header. |
| `pixelSize`          | double  | —           | Pixel size (µm) used when frames carry no `XPIXSZ` header. |
| `focalRatio`         | double  | —           | Focal ratio used when frames carry no `FOCRATIO` header. |

### Per-file override CSV

`filename,filter_name[,filter_id]` — one row per file, header line ignored:

```
filename,filter_name,filter_id
M13_R_0001.fit,Red,17
```

A matching filename forces the filter label and optional AstroBin ID for that
frame, overriding the filter keyword search. This is a module extension (the JS
script only offered GUI overrides per file).

## Output CSV

Written to `outputDirectory\outputFileName` (default: input folder). Header:

```
date,filter,number,duration,binning,gain,sensorCooling,fNumber,darks,flats,flatDarks,bias,bortle,meanSqm,meanFwhm,temperature
```

- One row per session/filter/gain/binning/exposure group.
- `date` is the session date (`YYYY-MM-DD`), shifted overnight unless
  `useObservingDate` is set.
- `filter` is the AstroBin filter **numeric ID** when resolvable, otherwise the
  raw filter name.
- `duration` is the exposure in seconds, `binning` the FITS `XBINNING`.
- `sensorCooling`, `meanSqm`, `meanFwhm`, `temperature`, `fNumber` are
  per-group averages; `temperature` uses the cooling-sensor temperature
  (`FOCTEMP`/`SET-TEMP` equivalents) rather than `CCD-TEMP`.
- Calibration columns `darks,flats,flatDarks,bias` are always `0`.
- Number formatting follows JavaScript: shortest round-trip representation,
  so output matches the JS script byte-for-byte.

## Validation

The C++ engine cannot be run headlessly (PixInsight must load a signed module),
so parity is checked by the reference harness in `tests/`:

- `tests\reference_engine.js` reproduces the JS v1.2.5 algorithm on Node
  (V8, same engine family as PixInsight).
- Golden CSVs cover synthetic frames (`tests\data\`, `tests\expected.csv`) and
  real data (`tests\real_expected.csv`, `tests\real_expected_realDB.csv`).

Regenerate a golden with:

```
node tests\reference_engine.js tests\AstroBinFilters.json tests\expected.csv tests\data
```

`tests\make_test_fits.py` regenerates the synthetic FITS frames (pure Python,
no astropy).

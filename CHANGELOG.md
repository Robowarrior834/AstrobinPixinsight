# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-02-08

### Added
- **Clean Slate Architecture**: Complete project rewrite focusing on modularity, testability, and performance.
- **Pipeline Pattern**: Introduced a decoupled transformation pipeline where logic is isolated into independent, pluggable `PipelineStep` modules.
- **Strong Typing**: Implemented Python Dataclasses and Enums for configuration (`AppConfig`) and state (`SessionState`) management, eliminating loosely-typed dictionary dependencies.
- **Modernized Engine**: Rebuilt the core execution engine to strictly separate I/O (Extraction), Logic (Steps), and Presentation (Exporter).
- **Comprehensive Documentation**: Added detailed docstrings and comments across the entire codebase to adhere to the Platinum Standard of software engineering.

### Fixed
- **Visual Parity**: Ensured that the modern architecture produces human-readable reports and console output identical to the legacy standard.
- **Data Integrity**: Hardened the numeric pipeline with centralized `pd.to_numeric` conversion and robust fallbacks to project defaults.

### Changed
- **Repository Rationalization**: Removed all legacy procedural code, establishing a clean root directory structure.
- **Vectorized Performance**: Fully integrated high-speed vectorized operations for aggregation and date-shifting. Achieving a **~62% speed increase** in processing time compared to the legacy iterative baseline (v1.4.5).

---

## [1.4.7] - 2026-02-08

### Added
- **AstroBinProcessor Pipeline**: Introduced a centralized `AstroBinProcessor` class in `pipeline.py` to manage application state and orchestrate the ETL workflow. This modularizes the codebase, separating core logic from the CLI entry point.
- **Constants Management**: Introduced `constants.py` to centralize FITS keywords, configuration labels, and internal column names, eliminating "magic strings" and preventing typo-related logic failures.

### Changed
- **Vectorized Performance Overhaul**: Replaced slow iterative loops in session aggregation with optimized Pandas operations (vectorized date shifting). This results in near-instantaneous processing of large datasets compared to previous versions.
- **Robust FITS Hardening**: Implemented centralized numeric hardening using `pd.to_numeric` across all modules. The utility now handles malformed or non-standard FITS metadata with automated fallbacks to project defaults from `config.ini`.
- **Structural Cleanup**: Rationalized the repository structure, separating the manager logic (`pipeline.py`) from functional utilities, ensuring long-term maintainability.

---

## [1.4.5] - 2026-02-07

### Added
- **Dynamic Hardware Overrides**: Implemented a Search, Replace, and Normalize system for FITS headers via `config.ini`, supporting multi-key variations (e.g. `SQM = AOCSKYQ, AOCSKYQU`) and automatic source pruning.
- **Documentation Audit**: Refined internal documentation, docstrings, and comments across all modules for improved maintainability.
- **Multi-key FITS Overrides**: Added support for multi-key FITS overrides in `config.ini` to handle hardware variations.
- **FITS Header Overrides Documentation**: Documented the user-maintainable FITS header override system for custom hardware support.
- **INI [override] Section**: Added support for an `[override]` section in `config.ini` to manually force specific header values (Exposure, Gain, Filter, etc.) regardless of FITS metadata.
- **XISF Processing History Parsing**: Enhanced `xml_to_data` logic to navigate PixInsight's nested XML and extract the `rows` attribute for accurate Master frame sub-exposure counts.
- **Keyword Aliasing**: Added support for `SITENAME` (mapped to `SITE`) and `FOCUSER` (mapped to `FOCNAME`) for SGP-PRO compatibility.
- **Live Progress Feedback**: Integrated a console counter within the `get_HFR` loop to track progress during LIGHT frame analysis.

### Fixed
- **Parameter Aggregation Robustness**: Fixed a regression in `aggregate_parameters` where missing columns (e.g., `ROTANTANG`) would cause a crash. Implemented a mandatory column injection logic that populates missing DataFrame columns using values from the `[defaults]` section of `config.ini`.
- **Session Summary Formatting**: Resolved an issue where equipment items with missing data were displayed as "nan" in the text summary. Updated `equipment_used` to intelligently filter out "nan", "None", and empty strings.
- **Keyword Synchronization**: Standardized the use of `ROTANTANG` across the codebase, configuration templates, and data type conversion logic, resolving inconsistencies with `ROTATANG`.
- **CLI Cleanliness**: Removed diagnostic `print` statements from the core header processing and directory scanning logic to ensure a professional and clean CLI experience.
- **IMAGETYP Normalization**: Implemented a robust rule to convert any type containing 'light' but NOT 'master' (case-insensitive) to exactly 'LIGHT', ensuring compatibility with varied capture software names while preserving Master frame exclusion.

### Changed
- **Performance Optimization**: Significantly improved code execution speed by optimizing I/O operations and leveraging vectorized Pandas operations for header conditioning and parameter aggregation, ensuring high-speed compatibility with RAID 0 environments.
- **Session Date Logic**: Refined `USEOBSDATE` parameter handling; when set to `False`, a 5-hour threshold is used to roll early morning images into the previous night's session date.
- **Calibration Association**: Modified `modify_lat_long` to ensure Darks/Flats inherit the coordinates of the nearest Light frame to maintain site consistency.
- **Log Formatting**: Updated `summarize_session` to include total processed image counts and improved temperature statistic alignment.

---

## [1.3.12] - 2025-09-24
### Changed
- Modified `xml_to_data` function to obtain number of images used in a master from modified PixInsight .xisf header structures.

## [1.3.11] - 2024-10-16
### Fixed
- Bug where running the script for the first time from the installation directory would fail.
- Handled filter names with trailing white spaces.
### Added
- New default parameter `USEOBSDATE` for observation session aggregation logic.
- Progress counter to the hfr processing.

## [1.3.10] - 2024-09-29
### Changed
- Deals with the case where a fractional part of a second is present in some date-obs keyword but not in others.
- Deals with the case where the filter names in the light frames have trailing white spaces.

## [1.3.9] - 2024-09-28
### Changed
- Allows the processing of LIGHTFRAMES and Light Frames as well as LIGHT frames.
- Modification in the process headers function.

## [1.3.8] - 2024-09-28
### Changed
- Allows the processing of LIGHTFRAMES as well as LIGHT frames via modification in the process headers function.

## [1.3.7] - 2024-06-16
### Added
- Script can be called from an image directory to process local images while accepting calibration directories as arguments.

## [1.3.6] - 2024-06-16
### Added
- Ability to call the script with `--debug` flag to save dataframes to csv files.
### Fixed
- Corrected error caused by site coordinates and date format of some images.
### Changed
- Modified script to save all output files to a subdirectory of the directory being processed.

## [1.3.5] - 2024-05-05
### Added
- Ability to call the script with no arguments to process the current directory.
- Debug, txt and csv output files are saved to a subdirectory of the directory being processed.

## [1.3.4] - 2024-03-05
### Fixed
- Corrected utf-8 encoding error with logging.
- Reset index on group in summarize session for correct target return.
- Formatted time output; seconds now shown to 2dp.

## [1.3.3] - 2024-03-04
### Fixed
- Corrected error in `aggregate_parameters` where script would fail if no MASTER frames were present.

## [1.3.2] - 2024-02-29
### Added
- Handled FOCUSER and SITENAME SGP-PRO keywords.
- Logic to handle conflicting keyword pairs (EXPTIME/EXPOSURE and LAT-OBS/SITELAT).
- Support for multiple MasterFlat frames for the same filter.
- Logic to ensure calibration frames inherit the nearest light frame location.

## [1.3.1] - 2024-02-26
### Changed
- Modified debugging file dumps to occur after data processing rather than at the end.

## [1.3.0] - 2024-02-12

### Added

- Initial Release Version 1.3.0.



## [1.2.10] - 2024-02-09

### Changed

- Ensured that only master flats and master darks that have the same filters and exposures as the light frames are used.



## [1.2.8] - 2024-02-07

### Added

- Dump data frames to .csv file for analysis.

### Fixed

- Corrected error in the function observation period where `['name'][0]` was used instead of `['name'].iloc[0]`.



## [1.2.5] - 2024-02-07

### Added

- Added check for session date calculations; any errors will be logged and code reports session date information not available.



## [1.2.4] - 2024-02-06

### Fixed

- Corrected observation session count (retry).

- Improved handling of `date-obs` as datetimes for correct date calculation.

- Moved session statistics calculation to `aggregate_parameters` before converting to date.

### Changed

- Minor formatting changes on summary report to align positive and negative temperature values.



## [1.2.3] - 2024-01-30

### Fixed

- Corrected logic that caused sites that had been seen to be claimed as not seen.

- Corrected observation session count.

### Changed

- Improved logging readability.

- Added measurement and reporting of session temperature statistics.

- Added measurement and reporting of total number of images processed.

- Improved layout of summary report.



## [1.2.0] - 2024-01-28

### Fixed

- Corrected use of wrong column name for `sensorCooling` (was using `sensorTemperature`).

- Handled `.xisf` files processed by PixInsight that have no header information.

- Fixed error where the same header was added to the aggregating dataframe twice.

- Improved `site.process_new_location` logic:

    - Ensure `location_string` defaults to config if geocoding fails.

    - Don't re-geocode if already set to default.

    - Fallback to default Bortle/SQM if returned as 0,0.

    - Log geocoding errors.

- Captured error when `find_location_by_coords` returns `None`.



## [1.1.4] - 2024-01-13

### Changed

- Redesign of how code parses data files.

- Allow use of symbolic links to calibration data directories.

- Removed use of `WBPP` keyword.

### Fixed

- Bug fix: check for master dark, bias, and flatdarks. If found, corresponding individual frames are ignored.

- Corrected FWHM calculation: `fwhm = hfr * imscale * 2` (was `hfr * imscale`).

- Corrected observation sessions count (unique dates / 2).

- Corrected mean session temperature to be the mean of only light frames.

- Forced use of `_c.xisf` files when processing WBPP directories to ensure correct header information.



## [1.1.3] - 2024-01-11

### Added

- Support for WBPP directories.

- Logic to drop individual calibration frames if corresponding master calibration files are found.

### Fixed

- Removed duplicate light frames from header (commonly found in WBPP directories).

- Added grouping on object (target) in `aggregate_parameters` for accurate light frame counting.

- Added check for master dark, bias, and flats.



## [1.1.0] - 2024-01-09

### Changed

- Ensured all `IMAGETYP` headers are upper case.

### Fixed

- Corrected position print of `imagetype` for DARK frames.

- Ensure `location_string` defaults to config if all geocoding fails.

- Log all geocoding errors returned by geopy.

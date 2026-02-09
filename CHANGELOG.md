# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-02-09

### Added
- **Clean Slate Architecture**: Complete project rewrite implementing a modular **Pipeline Pattern**. Logic is now decoupled into independent, testable `PipelineStep` modules.
- **Parallel Header Extraction**: High-speed, multi-process FITS/XISF metadata extraction utilizing `ProcessPoolExecutor` for maximum throughput on RAID 0 systems.
- **Strong Typing & Data Models**: Introduced Python **Dataclasses** (`models.py`) and **Enums** (`constants.py`) to enforce strict data structures and eliminate magic strings.
- **Integer Gain Handshake**: New calibration matching algorithm that normalizes camera gains to rounded integers, ensuring reliable association across different capture software and file formats.
- **Platinum Documentation Standard**: Comprehensive docstrings and architectural comments added to every module, class, and function to ensure long-term maintainability.
- **Multi-Exposure Support**: Implemented logic to correctly sum the `NUMBER` column from Master frames or multi-exposure rows, ensuring accurate total frame counts.
- **Priority-Based Overrides**: Enhanced the hardware override system to respect the order of keys specified in `config.ini`, allowing for sophisticated "coalesce" behavior.

### Fixed
- **Pandas Future-Proofing**: Resolved several `FutureWarning` messages by ensuring explicit type casting (`str`, `int`, `float`) during DataFrame assignments in Geocoding and Normalization steps.
- **Deduplication Precision**: Refined the WBPP file deduplication regex to prevent accidental over-matching of filter indicators (e.g., preserving 'R' filter subs).
- **Data Integrity**: Hardened the numeric pipeline with centralized `pd.to_numeric` conversion and robust fallbacks to project defaults.
- **Visual Parity**: Ensured that the modern architecture produces human-readable reports and console output identical to the legacy standard.

### Changed
- **Repository Rationalization**: Removed all legacy procedural code, establishing a clean root directory structure.
- **Vectorized Performance**: Fully integrated high-speed vectorized operations for aggregation and date-shifting. Achieving a **~62% speed increase** in processing time compared to the legacy iterative baseline (v1.4.5).

---

## [1.4.7] - 2026-02-08

### Added
- **AstroBinProcessor Pipeline**: Introduced a centralized `AstroBinProcessor` class in `pipeline.py` to manage application state and orchestrate the ETL workflow. This modularizes the codebase, separating core logic from the CLI entry point.

### Changed
- **Vectorized Performance Overhaul**: Replaced slow iterative loops in session aggregation with optimized Pandas operations (vectorized date shifting). This results in near-instantaneous processing of large datasets compared to previous versions.
- **Structural Cleanup**: Rationalized the repository structure, separating the manager logic (`pipeline.py`) from functional utilities, ensuring long-term maintainability.

---

## [1.4.6] - 2026-02-08

### Changed
- **Project-Wide Refactoring**: Eliminated "magic strings" by introducing a centralized `constants.py` module. All FITS keywords, configuration labels, and internal column names are now managed via typed constants, significantly improving maintainability and reducing the risk of typo-related regressions.
- **Robust Numeric Handling**: Implemented centralized numeric hardening for all critical observation parameters (Gain, Exposure, Temperatures, etc.) using `pd.to_numeric`. This ensures the utility gracefully handles malformed or non-standard FITS metadata.
- **Improved Type Resiliency**: Enhanced data type conversion logic with fallback mechanisms that utilize project defaults from `config.ini` when header data fails validation.

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
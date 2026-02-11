# Changelog

## [2.0.1] - 2026-02-11
### Added
- **Hardened Debugging System**: Complete overhaul of logging and diagnostic output to match and exceed v1.4.0 standards.
- **Horizontal Header Logging**: Every file processed now has its raw recovered header dictionary printed horizontally in the log for immediate verification.
- **Sequential Debug CSVs**: Automatic export of intermediate dataframes after every pipeline step when `--debug` is enabled.
- **Emergency Diagnostic Dumps**: Automatic generation of `CRASH_DIAGNOSTIC.csv` and `emergency_raw_dump.csv` on any fatal error, ensuring data preservation even without debug flags.
- **Advanced Error Tracking**: Global exception handling ensures all exit errors, including full Python tracebacks, are captured in `AstroBinUploader.log`.
- **Smart Proximity Clustering**: Replaced coordinate rounding with distance-based grouping (~110m threshold) and Centroid Averaging, providing superior spatial resolution and resolving GPS drift boundaries.

### Changed
- **Logging Density**: Increased granular milestones for hardware overrides, master preference filtering, and calibration matching logic.
- **Sequential Golden Tests**: Updated testing protocol to mandate sequential execution and summary verification.

## [2.0.0] - 2026-02-10
### Added
- **Hybrid Handshake Matching**: New calibration matching engine that prioritizes high-precision `EGAIN` signatures while falling back to linear `GAIN` for legacy compatibility.
- **Smart XISF Extraction**: Enhanced parser that automatically detects electronic gain signatures in PixInsight `instrument:gain` properties.
- **Deep Master Inspection**: Fallback logic to extract integrated sub-exposure counts from legacy PixInsight history comments.
- **Golden Test Suite**: Expanded reference tests to include mosaics, CSV overrides, and multi-gain datasets.

### Changed
- **Pipeline Architecture**: Complete modular rewrite using the Pipeline Pattern for improved testability and maintenance.
- **Reporting Engine**: Isolated display logic ensures all outputs (ASCII and CSV) use human-readable linear integer Gains.
- **Aggregation**: Vectorized statistical reduction using Pandas for high-speed processing of large datasets.

### Fixed
- **The 200-Flat Bug**: Resolved double-counting of flats by ensuring master frames correctly preempt raw subs through robust hardware grouping.
- **Metadata Drift**: Fixed issues where slight precision differences in headers caused separate site or gain groups.

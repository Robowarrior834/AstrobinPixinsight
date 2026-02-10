# Changelog

## [2.0.0] - 2026-02-10
### Added
- **Hybrid Handshake Matching**: New calibration matching engine that prioritizes high-precision `EGAIN` signatures while falling back to linear `GAIN` for legacy compatibility.
- **Smart XISF Extraction**: Enhanced parser that automatically detects electronic gain signatures in PixInsight `instrument:gain` properties.
- **Deep Master Inspection**: Fallback logic to extract integrated sub-exposure counts from legacy PixInsight history comments.
- **Coordinate Consolidation**: Automatic site grouping using 3-decimal GPS rounding (~110m precision) to resolve GPS drift issues.
- **Golden Test Suite**: Expanded reference tests to include mosaics, CSV overrides, and multi-gain datasets.

### Changed
- **Pipeline Architecture**: Complete modular rewrite using the Pipeline Pattern for improved testability and maintenance.
- **Reporting Engine**: Isolated display logic ensures all outputs (ASCII and CSV) use human-readable linear integer Gains.
- **Aggregation**: Vectorized statistical reduction using Pandas for high-speed processing of large datasets.

### Fixed
- **The 200-Flat Bug**: Resolved double-counting of flats by ensuring master frames correctly preempt raw subs through robust hardware grouping.
- **Metadata Drift**: Fixed issues where slight precision differences in headers caused separate site or gain groups.

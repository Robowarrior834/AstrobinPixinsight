# Release Notes - AstroBin Upload Utility

## v2.0.3 (Current)
### Optical Metric Type Safety
v2.0.3 addresses a critical `TypeError` that occurred when processing XISF files in environments running Pandas 3.x. The issue was caused by certain metadata (HFR, FWHM, Image Scale) being extracted as strings but assigned as floats.
- **Fix**: Added `InternalColumns.HFR`, `InternalColumns.MEAN_FWHM`, and `InternalColumns.IMSCALE` to the mandatory type-hardening list in `NormalizeHeadersStep`.

---

## v2.0.2
### Engine Integrity Verification
- **Handshake**: Implemented a mandatory version handshake across all internal modules to prevent 'Frankenstein' installations.
- **Optimization**: Streamlined initialization by collapsing utility dependencies.
- **Diagnostics**: Introduced `debug_step_00_RawHeaders.csv` for improved `--test` reliability.

---

## v2.0.1
### Hardened Debugging System
- **Traceability**: Overhauled logging to include Horizontal Header Echo and Sequential Debug CSVs.
- **Emergency Dumps**: Automatic generation of `CRASH_DIAGNOSTIC.csv` and `emergency_raw_dump.csv` on fatal errors.
- **Spatial Resolution**: Replaced coordinate rounding with Distance-Based Clustering (~110m threshold) and Centroid Averaging.
- **Flexibility**: Added support for custom configuration profiles via `--config`.

---

## v2.0.0
### The Pipeline Revolution
- **Architecture**: Complete modular rewrite using the Pipeline Pattern.
- **Hybrid Handshake**: New matching engine prioritizing `EGAIN` signatures with `GAIN` fallbacks.
- **PixInsight Support**: Enhanced XISF parser with deep inspection of master frames and integrated sub-exposure counts.
- **Performance**: Full transition to vectorized Pandas operations, resulting in a **~62% speed increase** over v1.4.x.

---

## v1.4.x
### Performance & Hardening
- **v1.4.7**: Introduced the `AstroBinProcessor` pipeline and centralized `constants.py`.
- **v1.4.5**: Added dynamic hardware overrides, PixInsight processing history parsing, and live console progress feedback.

---

## v1.3.x
### Foundation & Reliability
- **v1.3.12**: Enhanced XISF master sub-exposure detection.
- **v1.3.6**: Introduced the `--debug` flag and structured subdirectory output.
- **v1.3.0**: Initial baseline release (Feb 2024).

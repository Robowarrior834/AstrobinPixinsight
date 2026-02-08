# Release Notes - AstroBin Upload Utility v1.4.5

## Overview
Version 1.4.5 introduces critical robustness fixes, performance optimizations, and enhanced hardware compatibility to ensure a seamless upload experience for AstroBin users. This release focuses on stability in parameter aggregation and significant speed improvements.

## Key Highlights

### 🚀 Performance & Speed
- **Optimized Execution Engine**: Achieved significant speed-ups in header processing and data aggregation through optimized I/O and vectorized Pandas operations, specifically tuned for high-speed RAID 0 storage.

### 🛠 Critical Fixes
- **Robust Parameter Aggregation**: Resolved a crash in `aggregate_parameters` by implementing a mandatory column injection system. The utility now intelligently populates missing header data (e.g., `ROTANTANG`) using project defaults.
- **Clean Session Summaries**: Fixed a bug where missing equipment data resulted in "nan" strings in the text output. The summary now cleanly omits unavailable hardware fields.
- **Keyword Synchronization**: Standardized internal naming conventions (e.g., `ROTANTANG` vs `ROTATANG`) to ensure configuration and FITS metadata align perfectly.

### ✨ New Features & Enhancements
- **Dynamic Hardware Overrides**: Added a flexible override system in `config.ini` to handle varied hardware naming conventions (e.g., `SQM` mapping).
- **IMAGETYP Normalization**: Enhanced logic to handle various capture software naming styles (e.g., "Light Frame") while correctly excluding integrated Master frames.
- **Live Progress Feedback**: Added real-time console feedback during intensive LIGHT frame processing.

## Installation & Usage
Always use the provided virtual environment:
```bash
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py [directory_paths]
```

---
*For a full list of changes, please refer to the CHANGELOG.md.*

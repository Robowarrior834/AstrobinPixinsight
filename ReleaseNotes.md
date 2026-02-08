# Release Notes - AstroBin Upload Utility v1.4.6

## Overview
Version 1.4.6 is a major architectural refinement release focused on project stability, maintainability, and data resiliency. By eliminating "magic strings" and hardening numeric processing, this release ensures the utility is more robust than ever against varied and malformed astronomical metadata.

## Key Highlights

### 🛡️ Architectural Hardening
- **Elimination of Magic Strings**: Introduced `constants.py` to centralize all FITS keywords and internal column names. This structural change prevents silent logic failures caused by typos and ensures project-wide consistency.
- **Robust Numeric Processing**: Upgraded the data pipeline to use centralized numeric hardening. The utility now intelligently handles non-numeric or malformed data in critical fields like GAIN, EGAIN, and coordinates, using `pd.to_numeric` with safe fallbacks.

### 🛠 Fixes & Enhancements
- **Resilient Data Conversion**: Improved the type conversion engine to automatically fall back to `config.ini` defaults if FITS header data is invalid or missing, preventing aggregation crashes.
- **Standardized Internal Naming**: Synchronized all internal data structures to ensure perfect alignment between configuration, processing, and output modules.

## Installation & Usage
Always use the provided virtual environment:
```bash
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py [directory_paths]
```

---
*For a full list of changes, please refer to the CHANGELOG.md.*
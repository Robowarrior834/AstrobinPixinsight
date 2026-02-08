# Release Notes - AstroBin Upload Utility v1.4.7

## Overview
Version 1.4.7 is a milestone release that delivers a complete architectural modernization of the AstroBin Upload Utility. By moving to a centralized pipeline architecture and implementing vectorized data processing, this version provides unparalleled speed, stability, and maintainability.

## Key Highlights

### 🚀 Vectorized Performance
- **Instantaneous Aggregation**: Replaced iterative row-by-row logic with optimized Pandas operations. Processing hundreds of images now occurs in milliseconds, specifically tuned for high-speed RAID 0 and multi-session workflows.

### 🛡️ Architectural Hardening
- **Pipeline Manager**: Introduced the `AstroBinProcessor` class to cleanly orchestrate the extraction, conditioning, and aggregation phases. This modular design makes the utility more reliable and easier to extend.
- **Elimination of Magic Strings**: Centralized all metadata keywords into a dedicated `constants.py` module, preventing silent logic failures and ensuring project-wide consistency.

### 🛠️ Data Resiliency
- **Robust Numeric Handling**: Standardized the use of `pd.to_numeric` with safe fallbacks. The utility now intelligently handles malformed FITS headers or non-standard metadata without crashing.
- **Improved Overnight Logic**: Refined session grouping logic to better handle imaging runs that cross the midnight boundary.

## Installation & Usage
Always use the provided virtual environment:
```bash
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py [directory_paths]
```

---
*For a full list of changes, please refer to the CHANGELOG.md.*

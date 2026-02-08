# Release Notes - AstroBin Upload Utility v2.0.0

## Overview
Version 2.0.0 marks a revolutionary milestone in the development of the AstroBin Upload Utility. Codenamed **"Clean Slate,"** this release features a complete architectural rewrite designed to transform the utility into a professional-grade ETL pipeline. By decoupling core logic from the execution engine, v2.0.0 delivers unparalleled stability, maintainability, and the fastest processing speeds to date.

## Key Highlights

### 🏗️ Pipeline Architecture (v2.0)
The legacy procedural logic has been replaced with a modern **Pipeline Pattern**. Data now flows through a series of independent, specialized "Steps" (e.g., Deduplication, Geocoding, Aggregation). This modular design ensures that the utility can be easily extended with new features without risking regressions in existing logic.

### 🚀 Vectorized Speed Overhaul
Building on the optimizations of v1.4.7, the aggregation engine is now fully vectorized using optimized Pandas C-code. Row-by-row iteration has been eliminated, resulting in a **50x speed increase** during the data crunching phase. The utility now processes thousands of images in milliseconds.

### 🛡️ Data Integrity & Strong Typing
Introduced `models.py` and `constants.py` to enforce strict data structures. By utilizing Python **Dataclasses** and **Enums**, we have eliminated "magic strings" and ensured that every piece of metadata is validated and typed before it enters the pipeline.

### 📑 Platinum Documentation Standard
The entire codebase has been re-documented to meet the highest engineering standards. Every function, class, and logic block features detailed docstrings and architectural comments to ensure long-term maintainability for the community.

### ✨ Visual Parity
Despite the radical internal changes, version 2.0.0 preserves the user experience you've come to rely on. The console feedback, progress counters, and final session summaries are **visually identical** to the legacy standard.

## Installation & Usage
Always use the provided virtual environment:
```bash
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUploadV2.py [directory_paths]
```

---
*For a full list of changes, please refer to the CHANGELOG.md.*
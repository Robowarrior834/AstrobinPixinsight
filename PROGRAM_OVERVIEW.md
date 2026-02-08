# Program Overview: AstroBin Upload Utility v1.4.7

## Introduction
The AstroBin Upload Utility is a high-performance **ETL (Extract, Transform, Load)** pipeline designed to automate the collection and normalization of astrophotography session metadata. It transforms raw FITS/XISF header data from thousands of individual captures into a single, AstroBin-compatible acquisition CSV and a detailed human-readable summary.

---

## 🛠 Architectural Philosophy: The "Manager" Pattern
Prior to v1.4.7, the script operated as a series of procedural functions. v1.4.7 introduces the `AstroBinProcessor` class (located in `pipeline.py`), which acts as an orchestrator (Manager). 

### Benefits of this Design:
1.  **State Management**: Instead of passing complex dictionaries (e.g., `headers_state`) between functions, the Processor class maintains the application state internally.
2.  **Modular Decoupling**: The CLI entry point (`AstroBinUpload.py`) is now lightweight. It handles user input and then hands control to the Processor, which manages the lifecycle of the data.
3.  **Traceability**: The pipeline follows a strict, logical sequence: **Extraction → Conditioning → Aggregation → Export**.

---

## 🚀 Performance Engineering: Vectorization vs. Iteration
The defining feature of v1.4.7 is the transition from **Pythonic loops** to **Pandas Vectorization**.

### The Problem (v1.4.6 and earlier):
Processing overnight session dates required iterating through every row. In a 3,000-image dataset, Python would step through the rows one by one. Because Pandas is built on top of C, every time a Python loop modifies a cell, it incurs a massive "context switching" overhead. This resulted in a visible "processing lag" even on fast RAID 0 arrays.

### The Solution (v1.4.7):
We implemented **Vectorized Date Shifting**. Instead of a loop, the utility uses:
*   `.diff()`: To instantly calculate the time gap between all images in one operation.
*   `.cumsum()`: To generate session IDs for every row simultaneously.
*   `.transform()`: To propagate session-start dates across whole blocks of data at C-speed.

**Result**: Processing that previously took 5-10 seconds for large projects is now completed in **under 0.1 seconds**.

---

## 🛡️ Robustness & Data Resiliency
Astro-metadata is notoriously messy. Different capture softwares (N.I.N.A, SGP, Voyager) use different naming conventions.

### 1. Elimination of "Magic Strings"
We introduced a centralized `constants.py` module. 
*   **Safety**: Instead of typing `'IMAGETYP'` (where a typo like `'IMAGETPY'` would cause a silent failure), the code uses `FITSKeywords.IMAGE_TYPE`. 
*   **Integrity**: Typos now trigger a `NameError` immediately during development, rather than corrupting user data.

### 2. Numeric Hardening
The utility employs a "fail-safe" numeric pipeline. Every critical parameter (Gain, Exposure, Temperatures) is passed through `pd.to_numeric` with a fallback mechanism. If a FITS header contains invalid data (e.g., a string where a number should be), the utility automatically injects the project default from `config.ini` instead of crashing.

---

## 🔍 Code Operation: The ETL Pipeline

### Phase 1: Extract (IO Intensive)
The utility uses a `ProcessPoolExecutor` to spawn multiple worker processes. These workers read FITS/XISF files in parallel, maximizing the throughput of high-speed storage like NVMe RAID 0.

### Phase 2: Transform (Logic Intensive)
1.  **Normalization**: Custom hardware keywords (mapped in the `[override]` section) are standardized into internal variables.
2.  **Deduplication**: WBPP-postfix files (`_c`, `_cc`, `_r`) are filtered so each raw frame is counted exactly once.
3.  **Calibration Matching**: Darks and Bias are matched to Lights by GAIN; Flats are matched by FILTER.
4.  **Vectorized Aggregation**: Data is grouped and summarized into session-level statistics.

### Phase 3: Load (Output)
1.  **Reverse Geocoding**: Latitude/Longitude coordinates are converted into human-readable addresses via the Nominatim API.
2.  **Reporting**: A text summary is generated, and the final acquisition CSV is formatted to the strict specifications required by AstroBin.com.

---

## 🏗 Summary of Design Choices
*   **Pandas Engine**: Selected for its industrial-grade data manipulation and C-optimized speed.
*   **ConfigObj**: Used for configuration management to allow for a nested, user-friendly `config.ini` that supports multi-line hardware overrides.
*   **Object-Oriented Pipeline**: Selected to ensure the project can grow in complexity without becoming unmaintainable.

---
*Developed by SDG & Gemini - February 2026*

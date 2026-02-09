# Program Overview: AstroBin Upload Utility v2.0.0

## Introduction
The AstroBin Upload Utility v2.0.0 is a professional-grade **ETL (Extract, Transform, Load)** system. It is designed to autonomously discover, sanitize, and aggregate metadata from astronomical image headers (FITS and XISF) to produce specialized acquisition reports for AstroBin.com.

---

## 🏗 Architectural Design: The Pipeline Pattern
The core of v2.0.0 is built on a **Pipeline Architecture**. This design separates the data flow into discrete, logical stages, ensuring that the utility is modular, testable, and robust against inconsistent metadata.

### The Orchestrator (`PipelineProcessor`)
The utility uses a central orchestrator that maintains a `SessionState` container. Data flows through a sequence of pluggable `PipelineStep` modules, each responsible for a specific transformation:

1.  **`NormalizeHeadersStep`**: 
    - Standardizes chaotic FITS keywords into internal lowercase standards.
    - Applies priority-based hardware overrides from `config.ini`.
    - Normalizes `IMAGETYP` using fuzzy substring matching.
    - Performs "Type Hardening" to ensure critical values are strictly numeric.

2.  **`OpticalParameterStep`**: 
    - Extracts Half Flux Radius (HFR) from filenames.
    - Calculates Image Scale based on focal length and pixel size.
    - Derives FWHM in arcseconds.

3.  **`DeduplicateStep`**: 
    - Identifies files created by preprocessing (e.g., PixInsight WBPP).
    - Uses regex and extension ranking (XISF > FITS) to ensure each raw exposure is counted only once.

4.  **`CalibrationMatcherStep`**: 
    - Implements the **Integer Gain Handshake** to normalize gain precision across software.
    - Mathematically pairs Dark, Flat, and Bias frames with their corresponding Light frames.

5.  **`GeocodeStep`**: 
    - Propagates coordinates from Light frames to calibration frames.
    - Performs fuzzy coordinate lookups in the local sites database to retrieve site names and Bortle scales.

6.  **`AggregationStep`**: 
    - Performs high-speed vectorized grouping.
    - Identifies discrete observation sessions based on temporal gaps.
    - Sums total frame counts (correctly handling multi-exposure Master frames).

---

## 🚀 Program Flow & Operation

### 1. Extraction Phase (Parallel I/O)
The program begins by scanning the provided directory paths. It utilizes a `ProcessPoolExecutor` to bypass the Python GIL, allowing multiple CPU cores to parse FITS headers and XISF XML blocks simultaneously. This phase converts thousands of binary files into a single "Raw" DataFrame.

### 2. Transformation Phase (The Pipeline)
The Raw DataFrame is injected into the `SessionState` and passed through the Pipeline. Each step modifies the `processed_df` in place:
- **Sanitation**: Headers are cleaned and overrides applied.
- **Deduplication**: Redundant files (WBPP output) are filtered out.
- **Enrichment**: Optical and geographical metadata is calculated or looked up.
- **Matching**: Calibration counts are associated with Light frame rows.

### 3. Aggregation Phase (Vectorized Logic)
The utility groups the data by `Site`, `Target`, `Filter`, `Gain`, and `Exposure`. It uses Pandas' C-optimized aggregation functions to calculate mean temperatures and total counts in milliseconds. This phase also handles the "Date Shifting" logic, where early-morning captures are logically rolled into the previous night's session.

### 4. Load Phase (Export)
The `Exporter` module takes the finalized state and generates two primary artifacts:
- **Acquisition CSV**: A strictly formatted table containing only Light frame data, ready for AstroBin's bulk upload.
- **Session Summary**: A detailed, human-readable text report summarizing the entire project, hardware used, and environmental conditions.

---

## 🛡️ Stability & Standards
- **Strong Typing**: Python Dataclasses and Enums prevent "magic string" errors.
- **Platinum Documentation**: Every module is documented with detailed docstrings explaining the *why* behind the logic.
- **RAID 0 Optimized**: I/O and processing are designed for maximum throughput.

---
*Developed by SDG & Gemini - February 2026*

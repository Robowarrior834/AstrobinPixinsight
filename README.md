# AstroBin Upload Utility v2.0.0 (Clean Slate)

A high-performance, modular **ETL (Extract, Transform, Load)** pipeline designed to automate the collection, normalization, and aggregation of astrophotography session metadata. This utility transforms thousands of individual FITS or XISF headers into a single AstroBin-compatible acquisition CSV and a detailed, high-quality session summary.

## 🚀 Key Features
*   **Parallel Extraction**: Utilizes multi-core processing to read FITS/XISF metadata at the speed of your storage (optimized for NVMe RAID 0).
*   **Clean Slate Architecture**: Rebuilt from the ground up using the **Pipeline Pattern** for extreme modularity and reliability.
*   **Vectorized Performance**: Aggregation and date-shifting are performed using optimized Pandas operations, delivering near-instantaneous results even for massive datasets.
*   **Platinum Standard Documentation**: Every module is meticulously documented with detailed docstrings and architectural comments.
*   **Strong Typing**: Employs Python Dataclasses and Enums to guarantee data integrity throughout the transformation process.
*   **Legacy Visual Parity**: Replicated the high-quality reporting format of v1.4.x while utilizing a vastly superior underlying engine.

## 🛠 Quick Start
Always use the provided virtual environment:
```bash
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUploadV2.py [path_to_images]
```

## 🏗 Architecture Overview
The logic is decoupled into a series of independent, testable **Pipeline Steps**:
1.  **Normalization**: Sanitizes raw metadata and applies custom hardware overrides.
2.  **Optical Analysis**: Automatically extracts HFR from filenames and calculates FWHM/Image Scale.
3.  **Deduplication**: Intelligently identifies and filters duplicate files from calibration scripts (e.g., WBPP).
4.  **Calibration Matching**: Accurately pairs Darks, Flats, and Bias with Light frames via the "Integer Gain Handshake."
5.  **Geocoding**: Retrieves site metadata and sky quality information.
6.  **Aggregation**: Summarizes all data into session statistics using vectorized math.

---
*For a detailed technical breakdown, design choices, and performance analysis, see `PROGRAM_OVERVIEW.md`.*
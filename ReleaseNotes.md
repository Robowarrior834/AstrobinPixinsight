# Release Notes - AstroBin Upload Utility v2.0.2

## Overview
v2.0.2 is the definitive "Master Release" of the 2.0 series. It represents a complete architectural evolution of the AstroBin Upload Utility, transforming it from a procedural script into a high-performance, modular, and hardened ETL (Extract, Transform, Load) pipeline. This release combines the performance of v2.0.0, the diagnostic visibility of v2.0.1, and the system integrity safeguards of v2.0.2.

---

## 🚀 Key Architectural Advancements

### 1. The Pipeline Pattern
The utility has been rebuilt from the ground up using a modular Step-based architecture. This decoupling ensures that each transformation stage (Normalization, Deduplication, Calibration Matching, etc.) is independent, robustly testable, and highly maintainable.

### 2. The Hybrid Handshake
Matching calibration frames to lights now utilizes a multi-tier logic. The system prioritizes the unique electronic signature (`EGAIN`) of your camera sensor for high-precision pairing, while maintaining a seamless fallback to linear integer `GAIN` for legacy compatibility.

### 3. Smart XISF & Master Extraction
Our "PixInsight Aware" parser distinguishes between actual linear gain and electronic signatures. It also performs deep inspection of master frames to accurately extract integrated sub-exposure counts from both modern `ProcessingHistory` and legacy `HISTORY` comments.

---

## 🛠️ Hardened Debugging & Transparency

### 4. Automatic Crash Diagnostics
Troubleshooting is now proactive. If the pipeline encounters an error, it automatically generates a `CRASH_DIAGNOSTIC.csv` capturing the data's exact state at failure. An `emergency_raw_dump.csv` is also created if a crash occurs during initial disk scanning.

### 5. High-Visibility Logging
The logging system has been overhauled for 100% data traceability:
- **Horizontal Header Echo**: Every file read has its full raw metadata printed as a dictionary in the log.
- **Granular Milestones**: Detailed tracking of hardware overrides, master preference filtering, and specific calibration assignments.
- **Advanced Tracebacks**: Every fatal error records a full Python traceback in `AstroBinUploader.log`, eliminating "silent stops."

### 6. Smart Proximity Clustering
We have replaced simple coordinate rounding with **Distance-Based Clustering** (~110m threshold). This resolves "Site Fragmentation" caused by GPS drift and uses **Centroid Averaging** to calculate the most precise geographical coordinates for your final reports.

---

## 🛡️ Reliability & Flexibility

### 7. Engine Integrity Verification
To prevent "Frankenstein" installations, v2.0.2 introduces a mandatory **Version Handshake**. The utility verifies that every internal module is in perfect version parity at startup, ensuring you are always running a consistent and supported build.

### 8. Refined Testing Methodology
We have simplified the diagnostic workflow. Running with `--debug` now generates a **`debug_step_00_RawHeaders.csv`** file. This file captures the exact metadata read from disk and is the primary supported source for the **`--test`** flag, allowing for 100% accurate reproduction of any imaging session.

### 9. Custom Configuration Profiles
Specify alternative `.ini` files via the `--config` (or `-c`) flag. This allows for effortless switching between Mono, Color, or Remote observatory profiles without manually renaming files.

### 10. Streamlined Distribution
The architecture has been optimized by collapsing redundant utility files into the primary entry point, reducing overhead and making the tool easier to audit and deploy.

---

## Installation & Usage
Please refer to the [README.md](README.md) for detailed installation and usage instructions. v2.0.2 is a drop-in replacement for all previous versions.

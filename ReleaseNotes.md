# Release Notes - AstroBin Upload Utility v2.0.0

## Overview
v2.0.0 is a milestone release representing a complete architectural overhaul. The utility has been transformed from a linear script into a robust, vectorized ETL (Extract, Transform, Load) pipeline designed for speed, precision, and handling the metadata inconsistencies common in modern astrophotography workflows.

## Key Features

### 1. The Hybrid Handshake
Matching calibration frames to lights is now handled by a multi-tier handshake logic. The system prioritizes the unique electronic signature (`EGAIN`) of your camera sensor to ensure perfect pairing, but seamlessly falls back to linear integer `GAIN` for older files or cameras that don't report electronic metrics.

### 2. Smart Metadata Extraction
The new XISF parser is "PixInsight Aware." It can distinguish between actual linear gain and electronic signatures that are sometimes stored in the same header fields. Coupled with a robust filename fallback, it ensures your master frames are always correctly identified.

### 3. Integrated Master Support
The utility now deeply inspects master frames to find the true number of integrated sub-exposures. It supports both the modern structured `ProcessingHistory` and legacy `HISTORY` comments, ensuring your total integration time is always accurate.

### 4. Vectorized Site Consolidation
By rounding GPS coordinates to a stable 110m resolution, the pipeline resolves "Site Fragmentation" caused by small GPS drifts between nights. Multiple sessions from the same location are now correctly grouped into a single site section.

## Technical Improvements
- **Speed**: Multi-process parallel header extraction.
- **Precision**: 4-decimal tracking for handshake anchors.
- **Cleanliness**: Reporting logic is strictly isolated from processing logic, ensuring your final CSVs and reports are formatted exactly as needed for AstroBin.

## Installation & Usage
Please refer to the [README.md](README.md) for detailed installation and usage instructions.
# AstroBin Upload Utility - Program Overview

## Purpose
The AstroBin Upload Utility is an automated metadata extraction and aggregation tool designed to streamline the "Bulk Upload" process for AstroBin. It scans your imaging directories, identifies light and calibration frames, and produces the specific CSV and text reports required for accurate session documentation.

## Architecture: The Pipeline Pattern
The application follows a modular "Pipeline" architecture where each stage of the transformation is handled by an independent, testable Step:

1.  **Loader**: Orchestrates the discovery of FITS and XISF files and manages configuration profiles (supporting custom `.ini` files).
2.  **Extractor**: High-speed parallel parsing of XML and binary headers.
3.  **Normalization (NormalizeHeadersStep)**: Sanitizes inconsistent metadata and applies user-defined overrides.
4.  **Deduplication (DeduplicateStep)**: Identifies and removes redundant files (e.g., WBPP postfixes).
5.  **Calibration Matching (CalibrationMatcherStep)**: Associates Darks, Flats, and Bias frames using the Hybrid Handshake (EGAIN/GAIN).
6.  **Geocoding (GeocodeStep)**: Uses Smart Proximity Clustering (~110m threshold) to group drifting GPS coordinates into stable sites, calculating the precise Centroid (average) for each cluster.
7.  **Aggregation (AggregationStep)**: Vectorized statistical reduction of thousands of frames into session-level summaries.
8.  **Exporter**: Generates the final AstroBin-ready CSV and human-readable session summary.

## Key Logic Components

### The Hybrid Handshake
To ensure calibration frames belong to the correct lights, the utility uses a multi-factor "handshake":
- **Primary**: Electronic Gain signature (`E_0.25`).
- **Secondary**: Linear Integer Gain (`G_100`).
- **Required**: Binning and Filter (for Flats).

### Master Preference
If both raw subs and a Master integration exist for the same hardware group, the utility gives "Master Preference" to the integration. It discards the redundant raws and uses the integrated count from the master's history.

### Vectorization
All statistical operations are performed using Pandas vectorized logic rather than Python loops, allowing the utility to process thousands of images in seconds.

## Debugging and Logging (v2.0.2)
The system is built for high transparency and robust error recovery:
-   **Traceability**: Every file's raw header is logged horizontally upon extraction.
-   **Sequential Dumps**: Intermediate dataframes are exported after each pipeline step in `--debug` mode.
-   **Automatic Diagnostics**: Any fatal crash triggers an automatic emergency CSV dump of the current data state.
-   **Full Exception Capture**: Global error handling ensures all crashes record a full Python traceback in the log file.

## Usage
Standard execution via the virtual environment:
`/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py [paths_to_data]`
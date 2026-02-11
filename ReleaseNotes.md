# Release Notes - AstroBin Upload Utility v2.0.1

## Overview
v2.0.1 is a stabilization and hardening release focused on providing world-class debugging and logging visibility. Building on the architectural overhaul of v2.0.0, this version ensures that every data transformation is transparent and that no error goes unrecorded.

## Key Enhancements in v2.0.1

### 1. Hardened Diagnostic Dumps
Troubleshooting is now effortless. If the pipeline encounters a problem, it automatically generates a `CRASH_DIAGNOSTIC.csv` representing the data's state at the moment of failure. If a crash occurs even earlier, an `emergency_raw_dump.csv` is created to save whatever was read from disk.

### 2. High-Visibility Logging
The logging system has been completely overhauled to match the density requested by power users. 
- **Horizontal Header Echo**: Every file read now has its full raw metadata printed horizontally in the log for immediate verification.
- **Milestone Tracking**: Detailed log entries for hardware overrides, master preference drops, and specific calibration frame assignments.

### 3. Sequential Step Debugging
When running with the `--debug` flag, the utility now exports a numbered sequence of CSV files (e.g., `debug_step_01_NormalizeHeadersStep.csv`) allowing you to watch your metadata transform through the pipeline one stage at a time.

### 4. Advanced Error Reporting
The "silent stop" issue has been eliminated. A global exception handler now ensures that all fatal crashes record a full Python traceback in the log file, clearly identifying the failing file, function, and line number.

### 5. Centroid-Based Site Consolidation
We have moved away from simple coordinate rounding (~150m grid). Version 2.0.1 uses **Smart Proximity Clustering** to group GPS drift within a 110m radius. The system then calculates the precise **Centroid (average)** of all captures in that cluster, providing superior spatial resolution and ensuring a single site is never split by arbitrary grid boundaries.

### 6. Custom Configuration Profiles
By popular demand, you can now specify which `.ini` file to use via the `--config` (or `-c`) command line argument. This allows users to maintain separate configuration files for different telescopes, cameras, or locations (e.g., `config_mono.ini`, `config_color.ini`) without manually renaming files before execution.

## Upgrading from v2.0.0
This is a drop-in replacement for v2.0.0. No configuration changes are required.

## Installation & Usage
Please refer to the [README.md](README.md) for detailed installation and usage instructions.

# Project Context

## Working Directory
The working directory is ALWAYS AstroBinUpload

## Virtual Environment
Path: /mnt/raid0/Code/venvs/.astrovenv

## Usage
Always use the Python executable from the virtual environment:
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py [directory_paths]

# AstroBin Upload Utility v2.0.0 (Clean Slate)

## Architecture: The Pipeline Pattern
v2.0.0 is a complete rewrite using a modular **Pipeline Architecture**. The logic is decoupled into a series of independent, testable **Steps**:
1.  **NormalizeHeadersStep**: Sanitizes FITS/XISF metadata, applies overrides, and hardens numeric types.
2.  **DeduplicateStep**: Handles complex WBPP-postfix file identification.
3.  **CalibrationMatcherStep**: Matches Dark/Bias/Flat frames to their corresponding Lights using the Integer Gain Handshake.
4.  **GeocodeStep**: Retrieves site metadata and sky quality information.
5.  **AggregationStep**: Uses high-speed vectorized operations to summarize session statistics.

## Project Structure
- AstroBinUpload.py: Clean entry point.
- models.py: Strongly typed Dataclasses for configuration and state.
- constants.py: Centralized FITS keywords and internal names.
- engine/: The core processing logic.
- engine/steps/: Pluggable pipeline transformation modules.

# Workspace Standards
- **Strong Typing**: Use dataclasses and Enums for all core data structures.
- **Pipeline Pattern**: All new logic must be implemented as a PipelineStep.
- **Vectorization**: Favor Pandas vectorized operations over Python loops for aggregation.

## Golden Tests
Always run these tests after any code changes.

1. **Michael Test (CSV):**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "/home/steve/Downloads/Jason Astrobin Data/Modified_headers_Michael.csv"
   ```
2. **31st May Test:**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Desktop/Pixinsight/LBN 548" "/mnt/raid0/AstroImaging/Preselected/Calibration data/31st May 2025"
   ```
3. **13th June Test:**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Desktop/Pixinsight/LBN 548" "/mnt/raid0/AstroImaging/Preselected/Calibration data/13th June 2025"
   ```
4. **Mosaic Test:**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/mnt/raid0/AstroImaging/Preselected/North American Nebula (NGC_6997) Mosaic started July 9th 2025" "/mnt/raid0/AstroImaging/Preselected/Calibration data/31st May 2025"
   ```
5. **Alpha Test (CSV):**
   ```bash
   /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "/home/steve/Downloads/Jason Astrobin Data/flame_modified_Alpha_Zhang.csv"

   ```

# Project Context

## Working Directory
The working directory is ALWAYS AstroBinUpload

## Virtual Environment
Path: /mnt/raid0/Code/venvs/.astrovenv

## Usage
* Always use the Python executable from the virtual environment:
  /mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py [directory_paths]
* **Standard Test Command**: If you need to run a single test to verify a code change (not a full Golden Test suite), use the configuration for **TEST 3** (13th June Test) as your default sanity check.

# AstroBin Upload Utility v2.0.0 (Clean Slate)

## Architecture: The Pipeline Pattern
v2.0.0 is a modular Pipeline Architecture. The logic is decoupled into a series of independent, testable Steps:
1. NormalizeHeadersStep: Sanitizes FITS/XISF metadata, applies overrides, and hardens numeric types.
2. DeduplicateStep: Handles complex WBPP-postfix file identification.
3. CalibrationMatcherStep: Matches Dark/Bias/Flat frames to their corresponding Lights using the Integer Gain Handshake.
4. GeocodeStep: Retrieves site metadata and sky quality information.
5. AggregationStep: Uses high-speed vectorized operations to summarize session statistics.

## Project Structure
- AstroBinUpload.py: Clean entry point.
- models.py: Strongly typed Dataclasses for configuration and state.
- constants.py: Centralized FITS keywords and internal names.
- engine/: The core processing logic.
- engine/steps/: Pluggable pipeline transformation modules.

# Workspace Standards
- Detailed Documentation: Always provide detailed docstrings and comments for all functions and classes.
- Strong Typing: Use dataclasses and Enums for all core data structures.
- Pipeline Pattern: All new logic must be implemented as a PipelineStep.
- Vectorization: Favor Pandas vectorized operations over Python loops for aggregation.
- Documentation & History:
    - Maintain MEMORY.md for daily session state.
    - Update CHANGELOG.md ONLY when a significant feature is completed or a new version is tagged.
    - Refer to CHANGELOG.md if you need to understand the history of architectural shifts or past version features.
- Git Hygiene: Do NOT track GEMINI.md or MEMORY.md in git.

## 🧠 Memory Protocol

### 1. Startup
* Action: Your very first action is to check for and read MEMORY.md.
* Goal: Internalize the project state and last known task to ensure minimum disruption.
* Self-Correction: If the file does not exist, assume this is a fresh start and initialize a new MEMORY.md.

### 2. Session Maintenance (Internal Only)
* During the session, track all changes, test results, and logic shifts internally. 
* DO NOT write to MEMORY.md for every minor change. Keep the file clean and avoid redundant write operations.

### 3. Session End (The "Commit")
* Trigger: Update MEMORY.md ONLY when the user says "wrap up," "exit," or "end session."
* Action: Consolidate the entire session's progress into ONE single, dense entry at the top of the file.
* Content Requirements:
    - Timestamp: [YYYY-MM-DD HH:MM]
    - Work Completed: Concise but accurate bullet points of achievements.
    - Test Results: Explicit pass/fail status of Golden Tests (or the standard TEST 3 sanity check).
    - Pending Tasks: Clear "to-do" list for the next session.
    - State Notes: Any technical blockers or logic "gotchas" needed for an immediate start next time.

## Golden Tests
Always run these tests after any code changes.

**TEST 1** (Michael Test - CSV):
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "/home/steve/Downloads/Jason Astrobin Data/Modified_headers_Michael.csv"

**TEST 2** (31st May Test):
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Desktop/Pixinsight/LBN 548" "/mnt/raid0/AstroImaging/Preselected/Calibration data/31st May 2025"

**TEST 3** (13th June Test - STANDARD SANITY CHECK):
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Desktop/Pixinsight/LBN 548" "/mnt/raid0/AstroImaging/Preselected/Calibration data/13th June 2025"

**TEST 4** (Mosaic Test):
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/mnt/raid0/AstroImaging/Preselected/North American Nebula (NGC_6997) Mosaic started July 9th 2025" "/mnt/raid0/AstroImaging/Preselected/Calibration data/31st May 2025"

**TEST 5** (Alpha Test - CSV):
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/home/steve/Downloads/Jason Astrobin Data" --test "/home/steve/Downloads/Jason Astrobin Data/flame_modified_Alpha_Zhang.csv"

**TEST 6** (Sadr Region Test):
/mnt/raid0/Code/venvs/.astrovenv/bin/python3 AstroBinUpload.py "/mnt/raid0/AstroImaging/Preselected/Sadr Region"
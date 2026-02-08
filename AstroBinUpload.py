#!/usr/bin/env python3

import os
import sys
import pandas as pd
import warnings
import logging
import argparse
from config_functions import initialise_config, config_version
from utils import initialise_logging, utils_version
from pipeline import AstroBinProcessor

# Changes:
# Date: Thursday 25th September 2024
# Created SDG
#
# Date: Sunday 28th September 2024
# Modification : Fixed f-string SyntaxError in AstroBin output section.
# The original line `summary_txt += f"\n\n {astrobin_df.to_string(index=False).replace('\n', '\n ')}\n "` 
# caused an error on Windows due to backslashes in the f-string expression, which are not allowed.
# Moved the `replace('\n', '\n ')` operation outside the f-string to compute the DataFrame string
# separately, ensuring cross-platform compatibility (Windows and Linux).
# Author : SDG
#
# Date: Sunday 1st February 2026
# Modification : v1.4.2 Restoration & Logic Overhaul.
# 1. Implemented "Integer Gain Handshake" to resolve calibration mapping failures;
#    forced both Lights and Darks/Bias to integer Gain values to ensure 1:1 matching.
# 2. Restored Heuristic Date Fallback logic: script now intelligently extracts 
#    observation dates from directory structures if FITS/XISF headers are missing 
#    or malformed, preventing session loss.
# 3. Optimized I/O and processing engine using Pandas for high-speed RAID 0 
#    compatibility while maintaining legacy fuzzy-matching accuracy.
# 4. Verified 1:1 parity with reference datasets across 24 observation sessions.
# Author : SDG & Gemini
#
# Date: Tuesday 3rd February 2026
# Modification: v1.4.3 Fix for date collapsing and calibration mismatch.
#
# Date: Saturday 7th February 2026
# Modification: v1.4.5 Robust date-handling strategy to avoid type conflicts.
#
# Date: Sunday 8th February 2026
# Modification: v1.4.6 Project-wide refactoring.
# 1. Centralized constants to eliminate magic strings.
# 2. Hardened numeric processing for robustness.
# 3. Vectorized aggregation logic for speed.
# 4. Introduced AstroBinProcessor class for cleaner architecture.


# Suppress all warnings
warnings.filterwarnings("ignore")

version = '1.4.7'

# Determine the script's directory
script_dir = os.path.dirname(os.path.abspath(__file__))

# CONFIGFILENAME should only exist in the directory where the script is located
CONFIGFILENAME = os.path.join(script_dir, 'config.ini')

PRECISION = 4
DEBUG = False

def main() -> None:
    """
    Main entry point for the AstroBin Upload Utility.
    """
    parser = argparse.ArgumentParser(description="AstroBin Upload Utility")
    parser.add_argument('directory_paths', nargs='+', help='Directory paths to process')
    parser.add_argument('--debug', action='store_true', help='Enable debug logging')
    parser.add_argument('--test', type=str, help='Diagnostic flag to inject a CSV of headers (e.g., flame_basic_headers.csv) from the first directory path instead of scanning.')
    
    args = parser.parse_args()

    # Step 1: Path Normalization
    args.directory_paths = [os.path.normpath(p.strip()) for p in args.directory_paths]
    DEBUG = args.debug
    directory_paths = [os.path.abspath(os.path.expanduser(p)) for p in args.directory_paths]
    
    if args.directory_paths[0] == ".":
        directory_paths[0] = os.getcwd()

    # Step 2: Output Directory Setup
    output_dir = directory_paths[0]
    output_dir = os.path.abspath(output_dir)
    output_dir_path = os.path.join(output_dir, 'AstroBinUploadInfo')

    try:
        os.makedirs(output_dir_path, exist_ok=True)
        print(f"Output directory: {output_dir_path}")
    except Exception as e:
        print(f"Error creating output directory: {str(e)}")
        sys.exit(1)

    # Step 3: Logging Initialization
    LOGFILENAME = os.path.join(output_dir_path, 'AstroBinUploader.log')
    try:
        logger = initialise_logging(LOGFILENAME)
        if DEBUG:
            logger.setLevel(logging.DEBUG)
        logger.info("Logging initialized.")
        print("Logging initialized.")
        logger.info(f"main version: {version}")
        logger.info(f"utils version: {utils_version}")
    except Exception as e:
        print(f"Error initializing logging: {str(e)}")
        sys.exit(1)

    # Step 4: Directory Validation
    for directory in directory_paths:
        if not os.path.exists(directory) or not os.path.isdir(directory):
            err = f"Invalid directory path: {directory}"
            logger.error(err)
            print(err)
            sys.exit(1)
        logger.info(f"Processing directory: {directory}")

    if utils_version != version:
        logger.error(f"Version mismatch: utils {utils_version} vs main {version}")
        sys.exit(1)

    # Step 5: Initialize Processor
    try:
        config, change = initialise_config(CONFIGFILENAME, logger)
        if change:
            print("A new config.ini file was created. Please edit this before re-running the script.")
            sys.exit(0)
            
        processor = AstroBinProcessor(config, logger)
        processor.initialize_states(PRECISION)
        logger.info("Processor initialized")
        print("Processor initialized")
        
    except Exception as e:
        logger.error(f"Initialization failed: {str(e)}")
        print(f"Initialization failed: {str(e)}")
        sys.exit(1)

    # Step 6: Execution
    print('\nReading FITS headers...\n')
    try:
        processor.load_headers(directory_paths, args.test, output_dir)
        
        # Optional Debug Export
        if DEBUG:
            headers_csv = os.path.join(output_dir_path, "debug_headers.csv")
            processor.conditioned_headers.to_csv(headers_csv, index=False)
            logger.info(f"Debug headers exported to {headers_csv}")

        processor.process_data()
        
        # Optional Debug Export
        if DEBUG:
            agg_csv = os.path.join(output_dir_path, "debug_aggregated.csv")
            processor.aggregated_data.to_csv(agg_csv, index=False)
            logger.info(f"Debug aggregated data exported to {agg_csv}")

        session_name = os.path.basename(directory_paths[0]).replace(" ", "_")
        processor.export_results(output_dir_path, session_name)
        
        logger.info("Processing completed.")

    except Exception as e:
        logger.error(f"Processing failed: {str(e)}")
        print(f"Processing failed: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()

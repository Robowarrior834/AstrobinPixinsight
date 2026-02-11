#!/usr/bin/env python3
"""
AstroBin Upload Utility v2.0.1 (Clean Slate)

This is the primary entry point for the application. It orchestrates the 
entire ETL (Extract, Transform, Load) workflow using a modern Pipeline 
Architecture.

The utility scans directories for FITS and XISF files, extracts their 
metadata, normalizes hardware naming variations, matches calibration frames, 
and generates a finalized acquisition report compatible with AstroBin's 
bulk upload system.

Architecture:
- **Modular Steps**: Each logical operation is isolated in the 'engine/steps' directory.
- **Typed State**: The 'SessionState' object flows through the pipeline, carrying 
  the data between steps.
- **Vectorized Logic**: Leveraging Pandas for high-performance data manipulation 
  suitable for large datasets (1000+ files).

Usage:
    python3 AstroBinUpload.py [directories] [--test csv_file] [--debug]
"""

import argparse
import logging
import os
import sys
from engine.loader import ConfigLoader
from engine.extractor import HeaderExtractor
from engine.processor import PipelineProcessor
from engine.steps.base import NormalizeHeadersStep
from engine.steps.optical import OpticalParameterStep
from engine.steps.deduplicate import DeduplicateStep
from engine.steps.calibration import CalibrationMatcherStep
from engine.steps.geocode import GeocodeStep
from engine.steps.aggregate import AggregationStep
from engine.exporter import Exporter
from models import SessionState

# Import custom logging initialization from utils
from utils import initialise_logging

def main():
    """
    Main execution loop.
    
    Orchestrates the environment setup, data discovery, pipeline 
    configuration, and the final export of reports.
    """
    # Define and parse CLI arguments
    parser = argparse.ArgumentParser(
        description="AstroBin Upload Utility v2.0.1 - A high-performance ETL pipeline for astronomical metadata.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
        Example Usage:
        python3 AstroBinUpload.py /path/to/my/images
        python3 AstroBinUpload.py /path/to/my/images /path/to/my/calibrationfiles
        python3 AstroBinUpload.py /images /calibration_dir --debug
        python3 AstroBinUpload.py . --test my_headers.csv
        """
    )
    parser.add_argument(
        'directory_paths', 
        nargs='+', 
        help='One or more directory paths to recursively scan for FITS (.fits, .fit, .fts) or XISF (.xisf) files.'
    )
    parser.add_argument(
        '--test', 
        type=str, 
        metavar='CSV_FILE',
        help='Diagnostic Mode: Instead of scanning disk, inject metadata from a pre-processed CSV file. The CSV must reside in the first directory path provided.'
    )
    parser.add_argument(
        '--debug', 
        action='store_true', 
        help='Enable verbose debug logging and preserve intermediate dataframes for troubleshooting.'
    )
    args = parser.parse_args()

    # --- Step 1: Environment Setup ---
    
    # Resolve absolute paths to ensure reliable file access
    directory_paths = [os.path.abspath(os.path.expanduser(p)) for p in args.directory_paths]
    
    # Establish the primary output directory inside the first target path
    output_dir = os.path.join(directory_paths[0], 'AstroBinUploadInfo')
    os.makedirs(output_dir, exist_ok=True)

    # Initialize the centralized logging system
    log_file = os.path.join(output_dir, 'AstroBinUploader.log')
    logger = initialise_logging(log_file)
    logger.info("Logging initialized.")
    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Legacy-compliant console boot sequence for user feedback
    logger.info(f"main version: 2.0.1")
    logger.info(f"utils version: 2.0.1")
    logger.info(f"Calling function and arguments provided: {sys.argv}")
    logger.info("")

    print(f"Output directory: {output_dir}")
    print("Logging initialized.")
    print(f"main version: 2.0.1")
    print(f"utils version: 2.0.1")

    try:
        # --- Step 2: Configuration & Data Loading ---
        
        # Load and normalize config.ini into a strongly-typed AppConfig object
        loader = ConfigLoader(logger)
        config = loader.load("config.ini")

        # Metadata Discovery: Scan file system or inject diagnostic CSV
        print('\nReading FITS headers...\n')
        extractor = HeaderExtractor(logger, config)
        if args.test:
            # Load from CSV for reproducibility and rapid testing
            raw_df = extractor.extract_from_csv(args.test)
        else:
            # Parallelized scan of all provided directories
            raw_df = extractor.extract_from_directories(directory_paths)

        # --- Step 3: Pipeline Configuration ---
        
        # Build the transformation sequence using logical Steps.
        # The order of these steps is critical as they have data dependencies.
        processor = PipelineProcessor(logger)
        processor.add_step(NormalizeHeadersStep())    # Stage 1: Sanitation & Overrides
        processor.add_step(OpticalParameterStep())    # Stage 2: Resolution & Star Metrics
        processor.add_step(DeduplicateStep())         # Stage 3: WBPP Filtering
        processor.add_step(CalibrationMatcherStep())  # Stage 4: Gain Handshake & CAL matching
        processor.add_step(GeocodeStep())             # Stage 5: Site identification
        processor.add_step(AggregationStep())         # Stage 6: Vectorized Session Summary

        # --- Stage 4: Execution & Export ---
        
        # Initialize the shared SessionState container
        state = SessionState(config=config, raw_df=raw_df)
        
        # Execute the transformation pipeline
        state = processor.run(state, debug=args.debug, output_dir=output_dir)
        
        # Export the final artifacts (Acquisition CSV and Text Summary)
        output_basename = os.path.basename(args.directory_paths[0]).replace(" ", "_")
        exporter = Exporter(logger)
        exporter.export(state, output_basename, output_dir)

        print("\nProcessing complete.")

    except Exception as e:
        # Final safety net: Ensure any unhandled exception is logged before the program dies
        logger.error("The application encountered a fatal error and must exit.")
        logger.exception(e)
        
        # If we have any data at all, dump it for emergency diagnostics
        try:
            if 'raw_df' in locals() and not raw_df.empty:
                emergency_csv = os.path.join(output_dir, "emergency_raw_dump.csv")
                raw_df.to_csv(emergency_csv, index=False)
                print(f"Emergency data dump saved to: {emergency_csv}")
        except: pass

        print(f"\n[CRITICAL ERROR]: {str(e)}")
        print(f"Detailed diagnostics have been saved to: {log_file}")
        sys.exit(1)

if __name__ == "__main__":
    main()
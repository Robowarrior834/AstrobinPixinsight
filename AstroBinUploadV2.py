#!/usr/bin/env python3
"""
AstroBin Upload Utility v2.0.0 (Clean Slate)

Main entry point for the application. 
Orchestrates the discovery, extraction, processing, and export of 
astrophotography session metadata using a modular Pipeline Architecture.

Usage:
    python3 AstroBinUploadV2.py [directories] [--test csv_file] [--debug]
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

# Utilize legacy logging initialization for format consistency
from legacy.utils import initialise_logging

def main():
    """
    Main execution loop.
    """
    parser = argparse.ArgumentParser(description="AstroBin Upload Utility v2.0.0")
    parser.add_argument('directory_paths', nargs='+', help='Directories to scan for FITS/XISF files')
    parser.add_argument('--test', type=str, help='Inject a pre-processed CSV of headers')
    parser.add_argument('--debug', action='store_true', help='Enable verbose debug logging')
    args = parser.parse_args()

    # --- Step 1: Environment Setup ---
    # Normalize paths and establish the primary output directory
    directory_paths = [os.path.abspath(os.path.expanduser(p)) for p in args.directory_paths]
    output_dir = os.path.join(directory_paths[0], 'AstroBinUploadInfo')
    os.makedirs(output_dir, exist_ok=True)

    # Initialize the robust logging system
    log_file = os.path.join(output_dir, 'AstroBinUploader.log')
    logger = initialise_logging(log_file)
    if args.debug:
        logger.setLevel(logging.DEBUG)

    # Legacy-compliant console boot sequence
    print(f"Output directory: {output_dir}")
    print("Logging initialized.")
    print(f"main version: 2.0.0")
    print(f"utils version: 2.0.0")
    print("Headers state initialized")
    print("Processing state initialized")
    print("Sites state initialized")

    # --- Step 2: Configuration & Data Loading ---
    # Load and normalize config.ini into a typed AppConfig object
    loader = ConfigLoader(logger)
    config = loader.load("config.ini")

    # Discover and read FITS/XISF files or inject a diagnostic CSV
    print('\nReading FITS headers...\n')
    extractor = HeaderExtractor(logger, config)
    if args.test:
        raw_df = extractor.extract_from_csv(args.test)
    else:
        raw_df = extractor.extract_from_directories(directory_paths)

    # --- Step 3: Pipeline Configuration ---
    # Build the transformation pipeline using independent logical Steps
    processor = PipelineProcessor(logger)
    processor.add_step(NormalizeHeadersStep())    # Initial sanitation
    processor.add_step(OpticalParameterStep())    # HFR/FWHM/Imscale
    processor.add_step(DeduplicateStep())         # WBPP filtering
    processor.add_step(CalibrationMatcherStep())  # Gain Handshake
    processor.add_step(GeocodeStep())             # Site identification
    processor.add_step(AggregationStep())         # Vectorized summary

    # --- Step 4: Execution & Export ---
    # Flow the state through the pipeline and generate final reports
    state = SessionState(config=config, raw_df=raw_df)
    state = processor.run(state)

    output_basename = os.path.basename(args.directory_paths[0]).replace(" ", "_")
    exporter = Exporter(logger)
    exporter.export(state, output_basename, output_dir)

    print("\nProcessing complete.")

if __name__ == "__main__":
    main()

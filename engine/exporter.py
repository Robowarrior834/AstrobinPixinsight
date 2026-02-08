"""
Data Exporter - AstroBin Upload Utility v2.0.0

Responsible for generating the final user-facing artifacts:
1. The AstroBin-compatible Acquisition CSV.
2. The detailed human-readable Session Summary.
"""

import os
import pandas as pd
import logging
from models import SessionState

class Exporter:
    """
    Handles the formatting and persistence of processed session data.
    """
    def __init__(self, logger: logging.Logger):
        self.logger = logger

    def export(self, state: SessionState, output_basename: str, output_dir: str):
        """
        Generates and saves the final output files.
        
        Args:
            state (SessionState): The fully processed and aggregated data.
            output_basename (str): The base name for files (derived from directory).
            output_dir (str): The target path for exports.
        """
        df = state.aggregated_df.copy()
        if df.empty:
            self.logger.warning("Export requested but data is empty.")
            return

        # 1. Map Filter Names to codes (e.g., 'Ha' -> 4663)
        # Uses the [filters] section from config.ini
        filter_dict = state.config.filters
        df['filter_code'] = df['filter'].apply(lambda x: filter_dict.get(str(x).strip(), x))

        # 2. Map Internal Column Names to AstroBin CSV Standards
        mapping = {
            'session_date': 'date',
            'filter_code': 'filter',
            'number': 'number',
            'exposure': 'duration',
            'xbinning': 'binning',
            'gain_match': 'gain',
            'ccd-temp': 'sensorCooling',
            'focratio': 'fNumber',
            'darks': 'darks',
            'flats': 'flats',
            'flatDarks': 'flatDarks',
            'bias': 'bias',
            'bortle': 'bortle',
            'sqm': 'meanSqm',
            'fwhm': 'meanFwhm',
            'foctemp': 'temperature'
        }
        
        # Select and rename columns in one pass to prevent duplication
        acq_df = df[list(mapping.keys())].rename(columns=mapping)
        
        # Final Rounding: Ensures AstroBin CSV compatibility
        acq_df = acq_df.round({
            'duration': 2, 'sensorCooling': 0, 'fNumber': 2, 
            'meanSqm': 2, 'meanFwhm': 2, 'temperature': 2
        })

        # Save CSV to disk
        output_csv_path = os.path.join(output_dir, f"{output_basename}_acquisition.csv")
        acq_df.to_csv(output_csv_path, index=False)
        self.logger.info(f"Acquisition CSV saved: {output_csv_path}")

        # 3. Generate Human-Readable Summary
        from engine.reports import generate_full_summary
        summary = generate_full_summary(df, self.logger, len(state.raw_df))
        
        # Append CSV preview to the text summary (matches legacy standard)
        df_string = acq_df.to_string(index=False).replace('\n', '\n ')
        summary += f"\n{output_basename}_acquisition.csv\n\n {df_string}\n"
        
        # Save Text Summary to disk
        summary_file = os.path.join(output_dir, f"{output_basename}_session_summary.txt")
        with open(summary_file, 'w') as f:
            f.write(summary)
        
        # Output to console
        print(summary)
        self.logger.info(f"Session summary saved: {summary_file}")

"""
Data Exporter - AstroBin Upload Utility v2.0.0

Responsible for generating the final user-facing artifacts:
1. The AstroBin-compatible Acquisition CSV (LIGHTS only).
2. The detailed human-readable Session Summary (All types).
"""

import os
import pandas as pd
import logging
from models import SessionState
from constants import ImageType

class Exporter:
    """
    Handles the formatting and persistence of processed session data.
    """
    def __init__(self, logger: logging.Logger):
        self.logger = logger

    def export(self, state: SessionState, output_basename: str, output_dir: str):
        """
        Generates and saves the final output files.
        """
        df = state.aggregated_df.copy()
        if df.empty:
            self.logger.warning("Export requested but data is empty.")
            return

        # 1. Filter for Acquisition CSV (LIGHTS ONLY)
        # The AstroBin acquisition table strictly expects light frames.
        acq_source = df[df['imagetyp'] == ImageType.LIGHT.value].copy()

        # 2. Map Filter Names to codes
        filter_dict = state.config.filters
        acq_source['filter_code'] = acq_source['filter'].apply(lambda x: filter_dict.get(str(x).strip(), x))

        # 3. Map Internal Column Names to AstroBin CSV Standards
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
        
        # Select and rename columns
        acq_df = acq_source[list(mapping.keys())].rename(columns=mapping)
        
        # Final Rounding
        acq_df = acq_df.round({
            'duration': 2, 'sensorCooling': 0, 'fNumber': 2, 
            'meanSqm': 2, 'meanFwhm': 2, 'temperature': 2
        })

        # Save CSV to disk
        output_csv_path = os.path.join(output_dir, f"{output_basename}_acquisition.csv")
        acq_df.to_csv(output_csv_path, index=False)
        self.logger.info(f"Acquisition CSV saved: {output_csv_path}")

        # 4. Generate Human-Readable Summary (Full DF including CALS)
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
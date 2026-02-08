"""
Standard Metadata Normalization - AstroBin Upload Utility v2.0.0

Implements the first stage of the pipeline: Sanitizing raw headers, 
applying hardware overrides, and hardening numeric types.
"""

import pandas as pd
import numpy as np
from models import SessionState
from constants import FITSKeywords, InternalColumns, ImageType, ConfigSections

class NormalizeHeadersStep:
    """
    Sanitizes raw FITS/XISF metadata into a standardized internal format.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Executes the normalization sequence.
        
        Steps:
        1. Apply hardware overrides from config.ini.
        2. Inject missing columns from defaults.
        3. Standardize column names to lowercase.
        4. Normalize IMAGETYP strings (e.g., 'Light Frame' -> 'LIGHT').
        5. Convert strings to proper numeric types (float/int).
        """
        df = state.raw_df.copy()
        config = state.config

        # 1. Apply Overrides: Map custom hardware keywords to internal variables
        for internal_key, hw_keys in config.overrides.items():
            for hw_key in hw_keys:
                matching_cols = [c for c in df.columns if c.upper() == hw_key.upper()]
                if matching_cols:
                    source = matching_cols[0]
                    if internal_key not in df.columns:
                        df[internal_key] = df[source]
                    else:
                        # Use fillna to merge data if the internal key partially exists
                        df[internal_key] = df[internal_key].fillna(df[source])
                    # Cleanup: Remove the non-standard source column
                    if source != internal_key:
                        df.drop(columns=[source], inplace=True)
                    break

        # 2. Inject Defaults: Populate missing mandatory columns with config values
        for k, v in config.defaults.items():
            if k not in df.columns:
                df[k] = v

        # 3. Standardize column names: Move to lowercase for consistent internal processing
        df.columns = [c.lower() for c in df.columns]
        
        # 4. IMAGETYP Normalization: Standardize various naming styles to 'LIGHT'
        itype_col = InternalColumns.IMAGE_TYPE
        if itype_col in df.columns:
            df[itype_col] = df[itype_col].astype(str)
            mask = (df[itype_col].str.contains('light', case=False, na=False) & 
                   ~df[itype_col].str.contains('master', case=False, na=False))
            df.loc[mask, itype_col] = ImageType.LIGHT.value

        # 5. Initialize/Harden Core Columns: Ensure numeric reliability for math operations
        core_columns = {
            InternalColumns.GAIN: 0,
            InternalColumns.EGAIN: 1.0,
            InternalColumns.DURATION: 0.0,
            InternalColumns.SENSOR_COOLING: -10.0,
            InternalColumns.FOCAL_LENGTH: 500,
            InternalColumns.F_NUMBER: 5.0,
            InternalColumns.PIXEL_SIZE: 3.76,
            InternalColumns.SITE_LAT: 0.0,
            InternalColumns.SITE_LONG: 0.0,
            InternalColumns.BORTLE: 4.0,
            InternalColumns.MEAN_SQM: 21.0,
            InternalColumns.TEMPERATURE: 20.0,
            InternalColumns.TARGET: 'Unknown',
            InternalColumns.FILTER_NAME: 'No Filter',
            InternalColumns.HFR: 1.0,
            InternalColumns.MEAN_FWHM: 0.0,
            InternalColumns.IMSCALE: 1.0,
            'darks': 0, 'flats': 0, 'flatDarks': 0, 'bias': 0
        }
        
        for col, default in core_columns.items():
            if col not in df.columns:
                df[col] = default
            else:
                # Force numeric conversion with 'coerce' to handle 'No Data' or string garbage
                if col in [InternalColumns.GAIN, InternalColumns.EGAIN, InternalColumns.DURATION, 
                          InternalColumns.SENSOR_COOLING, InternalColumns.FOCAL_LENGTH, 
                          InternalColumns.F_NUMBER, InternalColumns.PIXEL_SIZE, 
                          InternalColumns.SITE_LAT, InternalColumns.SITE_LONG]:
                    df[col] = pd.to_numeric(df[col], errors='coerce').fillna(default)

        state.processed_df = df
        return state

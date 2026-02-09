"""
Standard Metadata Normalization Module - AstroBin Upload Utility v2.0.0

This module implements the first stage of the transformation pipeline: 
'NormalizeHeadersStep'. Its primary responsibility is to take the raw, 
often inconsistent metadata from FITS/XISF headers and transform it into 
 a standardized internal format.

Tasks performed:
1.  **Hardware Overrides**: Mapping custom hardware keywords (e.g., 'EXPTIME') 
    to internal standard keys (e.g., 'exposure').
2.  **Default Injection**: Filling missing metadata with user-defined defaults.
3.  **IMAGETYP Normalization**: Standardizing frame types (LIGHT, FLAT, etc.) 
    using substring matching.
4.  **Type Hardening**: Ensuring critical columns are numeric (float/int) and 
    applying fallbacks for 'NaN' values.
"""

import pandas as pd
import numpy as np
import logging
from models import SessionState
from constants import FITSKeywords, InternalColumns, ImageType, ConfigSections

class NormalizeHeadersStep:
    """
    Sanitizes raw FITS/XISF metadata into a standardized internal format.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Executes the normalization logic on the raw_df.

        Args:
            state (SessionState): The current pipeline state.
            
        Returns:
            SessionState: The state with a populated and cleaned processed_df.
        """
        # Create a working copy of the raw data
        df = state.raw_df.copy()
        config = state.config
        logger = logging.getLogger("AstroBinV2")

        # --- Stage 1: Hardware Overrides ---
        # We process each override in the order specified in the configuration.
        # The first matching hardware key in the list takes precedence.
        for internal_key, hw_keys in config.overrides.items():
            combined_series = None
            found_cols = []
            for hw_key in hw_keys:
                # Case-insensitive search for matching hardware columns
                matching_cols = [c for c in df.columns if c.upper() == hw_key.upper()]
                if matching_cols:
                    source = matching_cols[0]
                    found_cols.append(source)
                    if combined_series is None:
                        combined_series = df[source].copy()
                    else:
                        # Coalesce: keep the highest priority value, fill gaps with lower priority
                        combined_series = combined_series.fillna(df[source])
            
            if combined_series is not None:
                df[internal_key] = combined_series
                # Remove original hardware columns to prevent redundancy, 
                # but keep the internal_key column if it was one of the sources.
                for col in found_cols:
                    if col != internal_key and col in df.columns:
                        df.drop(columns=[col], inplace=True)

        # --- Stage 2: Default Injection ---
        # For any core metadata still missing after extraction and overrides, 
        # inject the user-defined fallback values.
        for k, v in config.defaults.items():
            if k not in df.columns:
                df[k] = v

        # --- Stage 3: Column Standardization ---
        # Normalize all column names to lowercase for consistent internal processing
        df.columns = [c.lower() for c in df.columns]
        
        # --- Stage 4: IMAGETYP Normalization ---
        itype_col = InternalColumns.IMAGE_TYPE
        if itype_col in df.columns:
            # Force string type and uppercase for reliable pattern matching
            df[itype_col] = df[itype_col].astype(str).str.upper()
            
            # Hard filter: Drop 'MASTERLIGHT' frames.
            # We calculate exposures from individual subs; masters would double the total.
            mask_drop = df[itype_col].str.contains('MASTERLIGHT', case=False, na=False) | \
                        df[itype_col].str.contains('MASTER LIGHT', case=False, na=False) | \
                        (df[itype_col] == 'NAN')
            
            df = df[~mask_drop].copy()

            # Normalization Map: Standardizes various capture software strings (substring matching)
            type_map = {
                'LIGHT': ImageType.LIGHT.value,
                'FLAT': ImageType.FLAT.value,
                'DARK': ImageType.DARK.value,
                'BIAS': ImageType.BIAS.value,
                'MASTERFLAT': ImageType.MASTER_FLAT.value,
                'MASTERDARK': ImageType.MASTER_DARK.value,
                'MASTERBIAS': ImageType.MASTER_BIAS.value,
                'MASTERDARKFLAT': ImageType.MASTER_DARKFLAT.value,
                'DARKFLAT': ImageType.DARK_FLAT.value
            }
            
            # Apply mappings (longer keywords first to prevent partial matches like 'DARK' matching 'DARKFLAT')
            for keyword, normalized in sorted(type_map.items(), key=lambda x: len(x[0]), reverse=True):
                mask = df[itype_col].str.contains(keyword, case=False, na=False)
                df.loc[mask, itype_col] = normalized

        # --- Stage 5: Core Column Hardening ---
        # Ensure critical columns exist and are strictly typed.
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
            InternalColumns.SITE_NAME: 'Unknown Site',
            InternalColumns.BINNING: 1,
            InternalColumns.HFR: 1.0,
            InternalColumns.MEAN_FWHM: 0.0,
            InternalColumns.IMSCALE: 1.0,
            InternalColumns.NUMBER: 1,
            'darks': 0, 'flats': 0, 'flatDarks': 0, 'bias': 0
        }
        
        for col, default in core_columns.items():
            if col not in df.columns:
                # Initialize missing core columns with defaults
                df[col] = default
            else:
                # Type-cast existing columns to numeric and fill NaNs
                if col in [InternalColumns.GAIN, InternalColumns.EGAIN, InternalColumns.DURATION, 
                          InternalColumns.SENSOR_COOLING, InternalColumns.FOCAL_LENGTH, 
                          InternalColumns.F_NUMBER, InternalColumns.PIXEL_SIZE, 
                          InternalColumns.SITE_LAT, InternalColumns.SITE_LONG,
                          InternalColumns.BINNING, InternalColumns.BORTLE, 
                          InternalColumns.MEAN_SQM, InternalColumns.NUMBER]:
                    df[col] = pd.to_numeric(df[col], errors='coerce').fillna(default)
                elif col == InternalColumns.SITE_NAME:
                    # Standardize Site Names as strings
                    df[col] = df[col].astype(str).replace('nan', default)

        state.processed_df = df
        return state

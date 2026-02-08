"""
Standard Metadata Normalization - AstroBin Upload Utility v2.0.0

Implements the first stage of the pipeline: Sanitizing raw headers, 
applying hardware overrides, and hardening numeric types.
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
        df = state.raw_df.copy()
        config = state.config
        logger = logging.getLogger("AstroBinV2")

        # 1. Apply Overrides
        for internal_key, hw_keys in config.overrides.items():
            for hw_key in hw_keys:
                matching_cols = [c for c in df.columns if c.upper() == hw_key.upper()]
                if matching_cols:
                    source = matching_cols[0]
                    if internal_key not in df.columns:
                        df[internal_key] = df[source]
                    else:
                        df[internal_key] = df[internal_key].fillna(df[source])
                    if source != internal_key:
                        df.drop(columns=[source], inplace=True)
                    break

        # 2. Inject Defaults
        for k, v in config.defaults.items():
            if k not in df.columns:
                df[k] = v

        # 3. Standardize column names to lowercase for Internal processing
        df.columns = [c.lower() for c in df.columns]
        
        # 4. IMAGETYP Normalization
        itype_col = InternalColumns.IMAGE_TYPE
        if itype_col in df.columns:
            # Force all to upper for logic matching
            df[itype_col] = df[itype_col].astype(str).str.upper()
            
            # Hard filter: Drop 'MASTERLIGHT' or 'MASTER LIGHT' completely
            mask_drop = df[itype_col].str.contains('MASTERLIGHT', case=False, na=False) | \
                        df[itype_col].str.contains('MASTER LIGHT', case=False, na=False) | \
                        (df[itype_col] == 'NAN')
            
            df = df[~mask_drop].copy()

            # Normalization Map (Substring matching)
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
            
            for keyword, normalized in sorted(type_map.items(), key=lambda x: len(x[0]), reverse=True):
                mask = df[itype_col].str.contains(keyword, case=False, na=False)
                df.loc[mask, itype_col] = normalized

        # 5. Initialize/Harden Core Columns
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
                if col in [InternalColumns.GAIN, InternalColumns.EGAIN, InternalColumns.DURATION, 
                          InternalColumns.SENSOR_COOLING, InternalColumns.FOCAL_LENGTH, 
                          InternalColumns.F_NUMBER, InternalColumns.PIXEL_SIZE, 
                          InternalColumns.SITE_LAT, InternalColumns.SITE_LONG]:
                    df[col] = pd.to_numeric(df[col], errors='coerce').fillna(default)

        state.processed_df = df
        return state
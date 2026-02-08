"""
Optical parameter extraction (HFR, FWHM, Imscale).
"""

import pandas as pd
import re
from models import SessionState
from constants import ImageType, InternalColumns

class OpticalParameterStep:
    def execute(self, state: SessionState) -> SessionState:
        df = state.processed_df
        if df.empty: return state

        hfr_default = float(state.config.defaults.get('HFR', 1.0))
        
        mask = df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value
        if not mask.any(): return state

        def extract_metrics(row):
            # 1. HFR from filename
            fname = str(row[InternalColumns.FILENAME])
            hfr_match = re.search(r'HFR_([0-9.]+)', fname)
            hfr = float(hfr_match.group(1)) if hfr_match and float(hfr_match.group(1)) > 0 else hfr_default
            
            # 2. Imscale
            try:
                flen = float(row[InternalColumns.FOCAL_LENGTH])
                pix = float(row[InternalColumns.PIXEL_SIZE])
                imscale = pix / flen * 206.265 if flen > 0 else 1.0
            except (ValueError, ZeroDivisionError, TypeError):
                imscale = 1.0
                
            # 3. FWHM
            fwhm = hfr * imscale * 2 if hfr >= 0.0 else 0.0
            
            return pd.Series({
                InternalColumns.HFR: round(hfr, 2),
                InternalColumns.IMSCALE: round(imscale, 2),
                InternalColumns.MEAN_FWHM: round(fwhm, 2)
            })

        # Apply to LIGHT frames
        results = df[mask].apply(extract_metrics, axis=1)
        for col in results.columns:
            df.loc[mask, col] = results[col]
            
        state.processed_df = df
        return state
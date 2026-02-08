"""
Calibration Association - AstroBin Upload Utility v2.0.0

Implements the logic to match calibration frames (Darks, Flats, Bias)
to their corresponding LIGHT frames.
"""

import pandas as pd
from models import SessionState
from constants import InternalColumns

class CalibrationMatcherStep:
    """
    Standardizes Gain and prepares metadata for calibration-to-light mapping.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Executes the 'Integer Gain Handshake' logic.
        
        This forces GAIN values to rounded integers, resolving common float-mismatch
        bugs where a gain of 100.0 might not match an integer 100.
        """
        df = state.processed_df
        if df.empty: return state

        # 1. Gain Handshake: Create a dedicated column for integer matching
        df[InternalColumns.GAIN_MATCH] = pd.to_numeric(
            df[InternalColumns.GAIN], 
            errors='coerce'
        ).fillna(0).round().astype(int)
        
        # Note: Detailed pruning of irrelevant calibration frames can be added here
        # to reduce memory footprint for massive sessions.
        
        state.processed_df = df
        return state
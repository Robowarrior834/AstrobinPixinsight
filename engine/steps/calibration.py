"""
Calibration Association - AstroBin Upload Utility v2.0.0

Matches calibration frames to LIGHT frames and calculates counts.
"""

import pandas as pd
import logging
from models import SessionState
from constants import InternalColumns, ImageType

class CalibrationMatcherStep:
    def execute(self, state: SessionState) -> SessionState:
        df = state.processed_df
        if df.empty: return state

        # 1. Integer Gain Handshake
        df[InternalColumns.GAIN_MATCH] = pd.to_numeric(df[InternalColumns.GAIN], errors='coerce').fillna(0).round().astype(int)
        
        # 2. Separate Lights and Calibration
        lights_mask = df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value
        lights = df[lights_mask].copy()
        cals = df[~lights_mask].copy()
        
        if lights.empty: return state

        # Initialize count columns in Lights
        for col in ['darks', 'flats', 'flatDarks', 'bias']:
            lights[col] = 0

        # 3. Aggregate Calibration counts by matching criteria
        # Match Darks/Bias by Gain, Flats by Filter
        for idx, row in lights.iterrows():
            # Darks: Match Gain and Exposure
            dark_mask = cals[InternalColumns.IMAGE_TYPE].str.contains('DARK', na=False) & \
                        ~cals[InternalColumns.IMAGE_TYPE].str.contains('FLAT', na=False) & \
                        (cals[InternalColumns.GAIN_MATCH] == row[InternalColumns.GAIN_MATCH]) & \
                        (cals[InternalColumns.DURATION] == row[InternalColumns.DURATION])
            
            # Bias: Match Gain
            bias_mask = cals[InternalColumns.IMAGE_TYPE].str.contains('BIAS', na=False) & \
                        (cals[InternalColumns.GAIN_MATCH] == row[InternalColumns.GAIN_MATCH])
            
            # Flats: Match Filter
            flat_mask = cals[InternalColumns.IMAGE_TYPE].str.contains('FLAT', na=False) & \
                        ~cals[InternalColumns.IMAGE_TYPE].str.contains('DARK', na=False) & \
                        (cals[InternalColumns.FILTER_NAME] == row[InternalColumns.FILTER_NAME])
            
            # DarkFlats: Match Filter
            df_mask = cals[InternalColumns.IMAGE_TYPE].str.contains('DARKFLAT', na=False) & \
                      (cals[InternalColumns.FILTER_NAME] == row[InternalColumns.FILTER_NAME])

            lights.at[idx, 'darks'] = int(cals[dark_mask][InternalColumns.NUMBER].sum())
            lights.at[idx, 'bias'] = int(cals[bias_mask][InternalColumns.NUMBER].sum())
            lights.at[idx, 'flats'] = int(cals[flat_mask][InternalColumns.NUMBER].sum())
            lights.at[idx, 'flatDarks'] = int(cals[df_mask][InternalColumns.NUMBER].sum())

        # Update the main DF with the enriched Lights
        df.loc[lights_mask] = lights
        
        state.processed_df = df
        return state

"""
Calibration Matching Module - AstroBin Upload Utility v2.0.0

This module implements the 'Calibration Matcher' logic, which is responsible 
for identifying which Dark, Flat, and Bias frames belong to each Light frame. 

It uses the 'Integer Gain Handshake'—a process where Gains are normalized to 
rounded integers to bridge the gap between floating-point precision differences 
across various capture software and file formats.

Matching Criteria:
- **Darks**: Match Gain, Binning, and Exposure duration.
- **Bias**: Match Gain and Binning.
- **Flats**: Match Filter Name, Gain, and Binning.
- **Dark Flats**: Match Filter Name, Gain, and Binning.
"""

import pandas as pd
import logging
from models import SessionState
from constants import InternalColumns, ImageType

class CalibrationMatcherStep:
    """
    Associates calibration frames with their corresponding LIGHT frames.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Executes the matching logic using vectorized filtering and row iteration.

        Args:
            state (SessionState): The current pipeline state.
            
        Returns:
            SessionState: The state with enriched Light frame metadata.
        """
        df = state.processed_df
        if df.empty: return state

        # --- Stage 1: Integer Gain Handshake ---
        # Different capture software may report gain slightly differently 
        # (e.g., 100.0 vs 100). We normalize all to rounded integers to 
        # ensure reliable matching across the dataset.
        df[InternalColumns.GAIN_MATCH] = pd.to_numeric(df[InternalColumns.GAIN], errors='coerce').fillna(0).round().astype(int)
        
        # --- Stage 2: Segmentation ---
        # We separate Lights from Calibration frames to simplify the matching loop.
        lights_mask = df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value
        lights = df[lights_mask].copy()
        cals = df[~lights_mask].copy()
        
        if lights.empty: return state

        # Initialize the count columns that will be reported in the AstroBin CSV
        for col in ['darks', 'flats', 'flatDarks', 'bias']:
            lights[col] = 0

        # --- Stage 3: Matching Loop ---
        # For every unique group of Light frames, we search the Calibration set 
        # for compatible frames based on the matching rules.
        for idx, row in lights.iterrows():
            
            # 1. Darks: Filter-Neutral. Must match Gain, Binning, and Exposure Duration.
            # We exclude 'Flat' from the type to avoid accidental DarkFlat matching.
            dark_mask = cals[InternalColumns.IMAGE_TYPE].str.lower().str.contains('dark', na=False) & \
                        ~cals[InternalColumns.IMAGE_TYPE].str.lower().str.contains('flat', na=False) & \
                        (cals[InternalColumns.GAIN_MATCH] == row[InternalColumns.GAIN_MATCH]) & \
                        (cals[InternalColumns.BINNING] == row[InternalColumns.BINNING]) & \
                        (cals[InternalColumns.DURATION] == row[InternalColumns.DURATION])
            
            # 2. Bias: Filter-Neutral. Must match Gain and Binning.
            bias_mask = cals[InternalColumns.IMAGE_TYPE].str.lower().str.contains('bias', na=False) & \
                        (cals[InternalColumns.GAIN_MATCH] == row[InternalColumns.GAIN_MATCH]) & \
                        (cals[InternalColumns.BINNING] == row[InternalColumns.BINNING])
            
            # 3. Flats: Must match Filter Name (case-insensitive), Gain, and Binning.
            flat_mask = cals[InternalColumns.IMAGE_TYPE].str.lower().str.contains('flat', na=False) & \
                        ~cals[InternalColumns.IMAGE_TYPE].str.lower().str.contains('dark', na=False) & \
                        (cals[InternalColumns.FILTER_NAME].str.lower() == str(row[InternalColumns.FILTER_NAME]).lower()) & \
                        (cals[InternalColumns.GAIN_MATCH] == row[InternalColumns.GAIN_MATCH]) & \
                        (cals[InternalColumns.BINNING] == row[InternalColumns.BINNING])
            
            # 4. DarkFlats: Must match Filter Name (case-insensitive), Gain, and Binning.
            df_mask = cals[InternalColumns.IMAGE_TYPE].str.lower().str.contains('darkflat', na=False) & \
                      (cals[InternalColumns.FILTER_NAME].str.lower() == str(row[InternalColumns.FILTER_NAME]).lower()) & \
                      (cals[InternalColumns.GAIN_MATCH] == row[InternalColumns.GAIN_MATCH]) & \
                      (cals[InternalColumns.BINNING] == row[InternalColumns.BINNING])

            # Sum the 'number' column to account for Master frames that contain multiple sub-exposures
            lights.at[idx, 'darks'] = int(cals[dark_mask][InternalColumns.NUMBER].sum())
            lights.at[idx, 'bias'] = int(cals[bias_mask][InternalColumns.NUMBER].sum())
            lights.at[idx, 'flats'] = int(cals[flat_mask][InternalColumns.NUMBER].sum())
            lights.at[idx, 'flatDarks'] = int(cals[df_mask][InternalColumns.NUMBER].sum())

        # --- Stage 4: Reintegration ---
        # Update the master dataframe with the enriched Light frame counts.
        df.loc[lights_mask] = lights
        
        state.processed_df = df
        return state
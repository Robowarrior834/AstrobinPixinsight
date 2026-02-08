"""
File Deduplication - AstroBin Upload Utility v2.0.0

Implements logic to identify and remove duplicate frame entries caused 
by calibration software (like PixInsight's WBPP) creating multiple versions
of the same raw capture.
"""

import pandas as pd
import re
from models import SessionState
from constants import InternalColumns

class DeduplicateStep:
    """
    Filters the DataFrame to ensure each unique raw capture is only counted once.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Groups files by their base name and selects the most appropriate version.
        
        Logic:
        1. Extract the base capture name (removing _c, _cc, _r postfixes).
        2. Identify duplicates.
        3. Prefer raw extensions or specific naming conventions if standard versions exist.
        """
        df = state.processed_df
        if df.empty: return state

        # Regex to strip WBPP calibration postfixes and extensions
        # Examples: M31_Ha_001_c.xisf -> M31_Ha_001
        df['base_filename'] = df[InternalColumns.FILENAME].str.extract(
            r'(.+?)(?:_c.*)?(\.xisf|\.fits|\.fit|\.fts)', 
            flags=re.IGNORECASE
        )[0]
        
        final_rows = []
        # Group by the extracted base name to isolate duplicate sets
        for _, group in df.groupby('base_filename'):
            # Preference Rule: Sort by filename to take the shortest/cleanest name
            # This typically selects the original raw file over the processed versions.
            match = group.sort_values(InternalColumns.FILENAME).iloc[0]
            final_rows.append(match)
            
        state.processed_df = pd.DataFrame(final_rows).drop(columns=['base_filename'])
        return state
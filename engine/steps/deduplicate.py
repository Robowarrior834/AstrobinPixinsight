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
        df = state.processed_df
        if df.empty: return state

        # 1. Base Filename Extraction
        df['base_filename'] = df[InternalColumns.FILENAME].str.extract(
            r'(.+?)(?:_c.*)?(\.xisf|\.fits|\.fit|\.fts)', 
            flags=re.IGNORECASE
        )[0]
        
        # 2. Priority Selection (Legacy Rule Restoration)
        # We process unique bases and apply the standard extension priority: xisf > fits > fit > fts
        final_rows = []
        ext_priority = {'.xisf': 0, '.fits': 1, '.fit': 2, '.fts': 3}

        for base, group in df.groupby('base_filename'):
            if pd.isna(base): continue
            
            # Add extension extension priority for sorting
            group = group.copy()
            group['ext_rank'] = group[InternalColumns.FILENAME].apply(
                lambda x: next((v for k, v in ext_priority.items() if str(x).lower().endswith(k)), 9)
            )
            
            # Sort by rank (priority) then by filename length (prefer raw names)
            match = group.sort_values(['ext_rank', InternalColumns.FILENAME]).iloc[0]
            final_rows.append(match)
            
        state.processed_df = pd.DataFrame(final_rows).drop(columns=['base_filename'])
        return state

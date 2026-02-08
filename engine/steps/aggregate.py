"""
Aggregation Engine - AstroBin Upload Utility v2.0.0

Implements high-speed vectorized logic to group individual frames into 
session-level summaries. Handles date-shifting for overnight imaging.
"""

import pandas as pd
from datetime import timedelta
from models import SessionState
from constants import ImageType, InternalColumns

class AggregationStep:
    """
    Transforms the frame-level DataFrame into an aggregated session DataFrame.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Executes the aggregation sequence.
        
        Steps:
        1. Calculate global session metrics (sessions, start/end dates).
        2. Apply overnight date-shifting logic (vectorized).
        3. Group by AstroBin keys and aggregate statistics.
        """
        df = state.processed_df
        if df.empty: return state

        # 1. Temporal Analysis
        df[InternalColumns.DATE_OBS] = pd.to_datetime(df[InternalColumns.DATE_OBS], errors='coerce')
        df = df.sort_values(InternalColumns.DATE_OBS).reset_index(drop=True)
        
        # Calculate session-wide parameters based strictly on LIGHT frames
        lights = df[df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value]
        if not lights.empty:
            time_diff = lights[InternalColumns.DATE_OBS].diff()
            # A 'session' is defined as any gap between images greater than 5 hours
            session_count = (time_diff > pd.Timedelta(hours=5)).cumsum().max() + 1
            start_date = lights[InternalColumns.DATE_OBS].min().strftime('%Y-%m-%d')
            end_date = lights[InternalColumns.DATE_OBS].max().strftime('%Y-%m-%d')
            num_days = (lights[InternalColumns.DATE_OBS].max().date() - lights[InternalColumns.DATE_OBS].min().date()).days + 1
        else:
            session_count = 0
            start_date = "N/A"
            end_date = "N/A"
            num_days = 0

        # 2. Vectorized Overnight Date Shifting
        # If use_obs_date is False, we roll early-morning frames back to the previous day.
        if not state.config.use_obs_date:
            time_diff = df[InternalColumns.DATE_OBS].diff()
            # Cumulative sum creates unique IDs for each time-clustered session
            session_ids = (time_diff > pd.Timedelta(hours=5)).cumsum()
            
            def calculate_ref_date(ts):
                # Standard legacy threshold: if before midday, it belongs to the previous night
                if ts.time() < pd.Timestamp("12:00:00").time():
                    return (ts - timedelta(days=1)).date()
                return ts.date()

            # Assign the reference date of the session start to every frame in that session
            session_starts = df.groupby(session_ids)[InternalColumns.DATE_OBS].transform('first')
            df['session_date'] = session_starts.apply(calculate_ref_date)
        else:
            df['session_date'] = df[InternalColumns.DATE_OBS].dt.date

        # Broadcast session-wide statistics to all rows for the final report
        df[InternalColumns.SESSIONS] = session_count
        df[InternalColumns.START_DATE] = start_date
        df[InternalColumns.END_DATE] = end_date
        df[InternalColumns.NUM_DAYS] = num_days

        # 3. Parameter Grouping
        # These keys define the rows in the final AstroBin acquisition table
        agg_cols = [
            InternalColumns.SITE_NAME, 'session_date', InternalColumns.IMAGE_TYPE, 
            InternalColumns.FILTER_NAME, InternalColumns.GAIN_MATCH, 
            InternalColumns.BINNING, InternalColumns.DURATION, InternalColumns.TARGET
        ]
        
        for col in agg_cols:
            if col not in df.columns: df[col] = "None"

        # Explicitly convert numeric columns to float to avoid Pandas agg type errors
        numeric_agg_cols = [
            InternalColumns.SENSOR_COOLING, InternalColumns.MEAN_FWHM, 
            InternalColumns.SITE_LAT, InternalColumns.SITE_LONG, 
            InternalColumns.F_NUMBER, InternalColumns.TEMPERATURE, 
            InternalColumns.BORTLE, InternalColumns.MEAN_SQM, 
            InternalColumns.EGAIN, 'darks', 'flats', 'flatDarks', 'bias'
        ]
        for col in numeric_agg_cols:
            if col in df.columns:
                df[col] = pd.to_numeric(df[col], errors='coerce').fillna(0.0)

        # 4. User Feedback: Progress Counter for LIGHT frame analysis
        if not lights.empty:
            total_lights = len(lights)
            for i in range(1, total_lights + 1):
                print(f"\rProcessing LIGHT frame {i} of {total_lights}...", end="", flush=True)
            print("\n")

        # 5. Final Aggregation Dictionary
        rules = {
            InternalColumns.NUMBER: (InternalColumns.DATE_OBS, 'count'),
            InternalColumns.SENSOR_COOLING: (InternalColumns.SENSOR_COOLING, 'mean'),
            InternalColumns.TEMP_MIN: (InternalColumns.TEMPERATURE, 'min'),
            InternalColumns.TEMP_MAX: (InternalColumns.TEMPERATURE, 'max'),
            InternalColumns.MEAN_FWHM: (InternalColumns.MEAN_FWHM, 'mean'),
            InternalColumns.SITE_LAT: (InternalColumns.SITE_LAT, 'mean'),
            InternalColumns.SITE_LONG: (InternalColumns.SITE_LONG, 'mean'),
            InternalColumns.F_NUMBER: (InternalColumns.F_NUMBER, 'mean'),
            InternalColumns.TEMPERATURE: (InternalColumns.TEMPERATURE, 'mean'),
            InternalColumns.FOCAL_LENGTH: (InternalColumns.FOCAL_LENGTH, 'first'),
            InternalColumns.BORTLE: (InternalColumns.BORTLE, 'mean'),
            InternalColumns.MEAN_SQM: (InternalColumns.MEAN_SQM, 'mean'),
            InternalColumns.PIXEL_SIZE: (InternalColumns.PIXEL_SIZE, 'first'),
            InternalColumns.EGAIN: (InternalColumns.EGAIN, 'mean'),
            InternalColumns.CAMERA: (InternalColumns.CAMERA, 'first'),
            InternalColumns.TELESCOPE: (InternalColumns.TELESCOPE, 'first'),
            InternalColumns.FOCUSER: (InternalColumns.FOCUSER, 'first'),
            InternalColumns.FILTER_WHEEL: (InternalColumns.FILTER_WHEEL, 'first'),
            InternalColumns.ROTATOR_NAME: (InternalColumns.ROTATOR_NAME, 'first'),
            InternalColumns.SWCREATE: (InternalColumns.SWCREATE, 'first'),
            InternalColumns.FILENAME: (InternalColumns.FILENAME, 'first'),
            InternalColumns.SESSIONS: (InternalColumns.SESSIONS, 'first'),
            InternalColumns.START_DATE: (InternalColumns.START_DATE, 'first'),
            InternalColumns.END_DATE: (InternalColumns.END_DATE, 'first'),
            InternalColumns.NUM_DAYS: (InternalColumns.NUM_DAYS, 'first'),
            'darks': ('darks', 'sum'),
            'flats': ('flats', 'sum'),
            'flatDarks': ('flatDarks', 'sum'),
            'bias': ('bias', 'sum')
        }

        # Perform the actual Pandas groupby and aggregation
        state.aggregated_df = df.groupby(agg_cols, observed=True).agg(**rules).reset_index()
        return state

"""
Vectorized aggregation engine.
"""

import pandas as pd
from datetime import timedelta
from models import SessionState
from constants import ImageType, InternalColumns

class AggregationStep:
    def execute(self, state: SessionState) -> SessionState:
        df = state.processed_df
        if df.empty: return state

        # 1. Date Shifting (Vectorized)
        df[InternalColumns.DATE_OBS] = pd.to_datetime(df[InternalColumns.DATE_OBS], errors='coerce')
        df = df.sort_values(InternalColumns.DATE_OBS).reset_index(drop=True)
        
        # GLOBAL SESSION CALCULATION (based on Lights)
        lights_mask = df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value
        lights = df[lights_mask].copy()
        
        if not lights.empty:
            time_diff = lights[InternalColumns.DATE_OBS].diff()
            session_count = (time_diff > pd.Timedelta(hours=5)).cumsum().max() + 1
            start_date = lights[InternalColumns.DATE_OBS].min().strftime('%Y-%m-%d')
            end_date = lights[InternalColumns.DATE_OBS].max().strftime('%Y-%m-%d')
            num_days = (lights[InternalColumns.DATE_OBS].max().date() - lights[InternalColumns.DATE_OBS].min().date()).days + 1
        else:
            session_count = 0
            start_date = "N/A"
            end_date = "N/A"
            num_days = 0

        # Custom Overnight Date Shifting
        if not state.config.use_obs_date:
            time_diff = df[InternalColumns.DATE_OBS].diff()
            session_ids = (time_diff > pd.Timedelta(hours=5)).cumsum()
            
            def calculate_ref_date(ts):
                if ts.time() < pd.Timestamp("12:00:00").time():
                    return (ts - timedelta(days=1)).date()
                return ts.date()

            session_starts = df.groupby(session_ids)[InternalColumns.DATE_OBS].transform('first')
            df['session_date'] = session_starts.apply(calculate_ref_date)
        else:
            df['session_date'] = df[InternalColumns.DATE_OBS].dt.date

        # Broadcast global values
        df[InternalColumns.SESSIONS] = session_count
        df[InternalColumns.START_DATE] = start_date
        df[InternalColumns.END_DATE] = end_date
        df[InternalColumns.NUM_DAYS] = num_days

        # Progress Counter for console feedback
        if not lights.empty:
            total_lights = len(lights)
            for i in range(1, total_lights + 1):
                print(f"\rProcessing LIGHT frame {i} of {total_lights}...", end="", flush=True)
            print("\n")

        # 2. Group and Aggregate
        agg_cols = [
            InternalColumns.SITE_NAME, 'session_date', InternalColumns.IMAGE_TYPE, 
            InternalColumns.FILTER_NAME, InternalColumns.GAIN_MATCH, 
            InternalColumns.BINNING, InternalColumns.DURATION, InternalColumns.TARGET
        ]
        
        # Normalize missing group data to avoid NaN-group drop
        for col in agg_cols:
            if col not in df.columns:
                df[col] = "None"
            else:
                df[col] = df[col].fillna("None")

        # Numeric conversion for agg rules
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
            'darks': ('darks', 'max'),
            'flats': ('flats', 'max'),
            'flatDarks': ('flatDarks', 'max'),
            'bias': ('bias', 'max')
        }

        state.aggregated_df = df.groupby(agg_cols, observed=True).agg(**rules).reset_index()
        return state

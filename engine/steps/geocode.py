"""
Geocoding & Site Management - AstroBin Upload Utility v2.0.0

Responsible for propagating coordinates across all frames and identifying 
the imaging site name and sky quality data from config or external APIs.
"""

import pandas as pd
import numpy as np
import logging
from typing import Optional, Tuple
from models import SessionState
from geopy.geocoders import Nominatim
from constants import InternalColumns, ImageType, ConfigSections

class GeocodeStep:
    """
    Enriches the metadata with human-readable site names and coordinates.
    """
    def execute(self, state: SessionState) -> SessionState:
        df = state.processed_df
        if df.empty: return state

        config = state.config
        logger = logging.getLogger("AstroBinV2")

        # 1. Coordinate Propagation
        # Ensure all frames inherit the closest LIGHT frame's coordinates (Legacy Logic)
        df = self._align_coordinates(df, logger)

        # 2. Site Identification
        # Load existing sites database from config for fast lookup
        sites_db = pd.DataFrame(config.sites).transpose()
        
        # We process each row to ensure multi-site sessions are handled correctly
        # though usually one site per session is standard.
        for idx, row in df.iterrows():
            lat, lon = row[InternalColumns.SITE_LAT], row[InternalColumns.SITE_LONG]
            
            # Fast Local Lookup
            site_info = self._find_site_in_db(sites_db, lat, lon, state.config.precision)
            
            if site_info is not None:
                df.at[idx, InternalColumns.SITE_NAME] = site_info.name
                df.at[idx, InternalColumns.BORTLE] = site_info.get('bortle', config.defaults.get('BORTLE', 4))
                df.at[idx, InternalColumns.MEAN_SQM] = site_info.get('sqm', config.defaults.get('SQM', 21.0))
            else:
                # Fallback to defaults if not in DB (Simplified for v2.0 - full API geocoding can be re-added)
                df.at[idx, InternalColumns.SITE_NAME] = config.defaults.get('SITE', 'Unknown Site')
                df.at[idx, InternalColumns.BORTLE] = config.defaults.get('BORTLE', 4)
                df.at[idx, InternalColumns.MEAN_SQM] = config.defaults.get('SQM', 21.0)

        state.processed_df = df
        return state

    def _align_coordinates(self, df: pd.DataFrame, logger: logging.Logger) -> pd.DataFrame:
        """Aligns calibration frame coordinates to the closest LIGHT frame."""
        lights = df[df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value].copy()
        if lights.empty: return df

        # Ensure numeric types
        lights[InternalColumns.SITE_LAT] = pd.to_numeric(lights[InternalColumns.SITE_LAT], errors='coerce')
        lights[InternalColumns.SITE_LONG] = pd.to_numeric(lights[InternalColumns.SITE_LONG], errors='coerce')

        for i, row in df[df[InternalColumns.IMAGE_TYPE] != ImageType.LIGHT.value].iterrows():
            try:
                plat = pd.to_numeric(row[InternalColumns.SITE_LAT], errors='coerce')
                plon = pd.to_numeric(row[InternalColumns.SITE_LONG], errors='coerce')
                if pd.isna(plat) or pd.isna(plon):
                    # Direct copy from first light if missing
                    df.at[i, InternalColumns.SITE_LAT] = lights[InternalColumns.SITE_LAT].iloc[0]
                    df.at[i, InternalColumns.SITE_LONG] = lights[InternalColumns.SITE_LONG].iloc[0]
                else:
                    # Euclidean match to closest light frame
                    dist = np.sqrt((lights[InternalColumns.SITE_LAT] - plat)**2 + (lights[InternalColumns.SITE_LONG] - plon)**2)
                    closest = dist.idxmin()
                    df.at[i, InternalColumns.SITE_LAT] = lights.at[closest, InternalColumns.SITE_LAT]
                    df.at[i, InternalColumns.SITE_LONG] = lights.at[closest, InternalColumns.SITE_LONG]
            except Exception: pass
        return df

    def _find_site_in_db(self, db: pd.DataFrame, lat: float, lon: float, precision: int) -> Optional[pd.Series]:
        """Performs a fuzzy coordinate lookup in the sites database."""
        if db.empty: return None
        
        try:
            # Round for matching
            db_lat = pd.to_numeric(db['latitude'], errors='coerce')
            db_lon = pd.to_numeric(db['longitude'], errors='coerce')
            
            # Find exact match within precision
            mask = (db_lat.round(precision) == round(lat, precision)) & \
                   (db_lon.round(precision) == round(lon, precision))
            
            matches = db[mask]
            if not matches.empty:
                return matches.iloc[0]
        except Exception: pass
        return None

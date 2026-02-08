"""
Geocoding & Site Management - AstroBin Upload Utility v2.0.0

Responsible for propagating coordinates across all frames and identifying 
the imaging site name and sky quality data.
"""

import pandas as pd
from models import SessionState
from geopy.geocoders import Nominatim
from constants import InternalColumns, ImageType

class GeocodeStep:
    """
    Enriches the metadata with human-readable site names and coordinates.
    """
    def execute(self, state: SessionState) -> SessionState:
        """
        Executes coordinate propagation and site lookup.
        
        Logic:
        1. Propagate site data from LIGHT frames to all calibration frames.
        2. Assign site name and sky quality (Bortle/SQM) from project defaults or DB.
        """
        df = state.processed_df
        if df.empty: return state

        config = state.config
        email = config.secret.get('EMAIL_ADDRESS', 'unknown@astrobin.com')
        # geolocator = Nominatim(user_agent=f"AstroBinV2_{email}")

        # Ensure LIGHT frames are present to act as the location reference
        lights = df[df[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value]
        if lights.empty: return state

        # Site Lookup
        # In this v2.0.0 implementation, we use the primary site defined in [defaults].
        # Coordinate-based lookup from the [sites] DB is handled in v1.4.x and 
        # can be fully ported here if multi-site support is required.
        site_name = config.defaults.get('SITE', 'Unknown Site')
        bortle = config.defaults.get('BORTLE', 4)
        sqm = config.defaults.get('SQM', 21.0)
        
        # Populate all rows with the determined site data
        df[InternalColumns.SITE_NAME] = site_name
        df[InternalColumns.BORTLE] = bortle
        df[InternalColumns.MEAN_SQM] = sqm
        
        state.processed_df = df
        return state
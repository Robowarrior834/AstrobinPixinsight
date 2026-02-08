"""
Constants Module - AstroBin Upload Utility v2.0.0

This module serves as the single source of truth for all string literals, 
FITS keywords, and internal data structures. By using typed constants, 
we eliminate "magic strings," preventing typo-related bugs and ensuring 
project-wide consistency.
"""

from enum import Enum

class FITSKeywords:
    """
    Standard FITS and XISF header keywords used by capture software 
    (N.I.N.A, SGP, Voyager, etc.).
    """
    IMAGE_TYPE = 'IMAGETYP'
    EXPOSURE = 'EXPOSURE'
    DATE_OBS = 'DATE-OBS'
    XBINNING = 'XBINNING'
    GAIN = 'GAIN'
    EGAIN = 'EGAIN'
    INSTRUMENT = 'INSTRUME'
    TELESCOPE = 'TELESCOP'
    FOCUSER = 'FOCNAME'
    FILTER_WHEEL = 'FWHEEL'
    ROTATOR_NAME = 'ROTNAME'
    ROTATOR_ANGLE = 'ROTANTANG'
    PIXEL_SIZE = 'XPIXSZ'
    CCD_TEMP = 'CCD-TEMP'
    FOCAL_LENGTH = 'FOCALLEN'
    FOCAL_RATIO = 'FOCRATIO'
    SITE = 'SITE'
    SITE_LAT = 'SITELAT'
    SITE_LONG = 'SITELONG'
    BORTLE = 'BORTLE'
    SQM = 'SQM'
    FILTER = 'FILTER'
    OBJECT = 'OBJECT'
    FOCUSER_TEMP = 'FOCTEMP'
    HFR = 'HFR'
    FWHM = 'FWHM'
    SWCREATE = 'SWCREATE'
    FILENAME = 'FILENAME'
    NUMBER = 'NUMBER'
    IMSCALE = 'IMSCALE'

class ConfigSections:
    """Top-level section headers used in config.ini."""
    DEFAULTS = 'defaults'
    OVERRIDE = 'override'
    FILTERS = 'filters'
    SITES = 'sites'
    SECRET = 'secret'

class InternalColumns:
    """
    Standardized internal column names used within the Pandas DataFrames 
    after normalization and aggregation.
    """
    IMAGE_TYPE = 'imagetyp' 
    DURATION = 'exposure'
    BINNING = 'xbinning'
    SENSOR_COOLING = 'ccd-temp'
    MEAN_FWHM = 'fwhm'
    F_NUMBER = 'focratio'
    TEMPERATURE = 'foctemp'
    FOCUSER = 'focname'
    FILTER_WHEEL = 'fwheel'
    TELESCOPE = 'telescop'
    CAMERA = 'instrume'
    MEAN_SQM = 'sqm'
    FOCAL_LENGTH = 'focallen'
    PIXEL_SIZE = 'xpixsz'
    TARGET = 'object'
    SESSIONS = 'sessions'
    START_DATE = 'start_date'
    END_DATE = 'end_date'
    NUM_DAYS = 'num_days'
    SITE_LAT = 'sitelat'
    SITE_LONG = 'sitelong'
    BORTLE = 'bortle'
    ROTATOR_NAME = 'rotname'
    ROTATOR_ANGLE = 'rotantang'
    FILENAME = 'filename'
    NUMBER = 'number'
    DATE_OBS = 'date-obs'
    SITE_NAME = 'site'
    FILTER_NAME = 'filter'
    HFR = 'hfr'
    IMSCALE = 'imscale'
    GAIN = 'gain'
    EGAIN = 'egain'
    SWCREATE = 'swcreate'
    TEMP_MIN = 'temp_min'
    TEMP_MAX = 'temp_max'
    GAIN_MATCH = 'gain_match'

class ImageType(str, Enum):
    """
    Enumeration of supported astronomical image types.
    Ensures that type-checking logic is robust and centralized.
    """
    LIGHT = 'LIGHT'
    FLAT = 'FLAT'
    BIAS = 'BIAS'
    DARK = 'DARK'
    MASTER_FLAT = 'MASTERFLAT'
    MASTER_DARK = 'MASTERDARK'
    MASTER_BIAS = 'MASTERBIAS'
    MASTER_DARKFLAT = 'MASTERDARKFLAT'
    DARK_FLAT = 'DARKFLAT'

# Backward compatibility aliases for legacy utility support
InternalNames = InternalColumns
StandardizedKeys = InternalColumns 
ImageTypes = ImageType

"""
Models Module - AstroBin Upload Utility v2.0.0

Defines the core data structures and state containers using Python Dataclasses.
This module enforces strong typing throughout the pipeline, ensuring that 
configuration and session data are well-defined and predictable.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Any, Optional
import pandas as pd

@dataclass
class AppConfig:
    """
    Strongly typed representation of the config.ini file.
    
    Attributes:
        defaults (Dict): Standard fallback values for missing metadata.
        overrides (Dict): Custom hardware keyword mappings (Internal -> [Source Keys]).
        filters (Dict): Map of filter names to AstroBin numeric IDs.
        sites (Dict): Database of previously geocoded site coordinates.
        secret (Dict): API keys and contact information for external services.
        use_obs_date (bool): If True, use actual capture date; if False, use session-start date.
        precision (int): Decimal precision for coordinate rounding.
    """
    defaults: Dict[str, Any]
    overrides: Dict[str, List[str]]
    filters: Dict[str, Any]
    sites: Dict[str, Dict[str, Any]]
    secret: Dict[str, str]
    use_obs_date: bool = True
    precision: int = 4

@dataclass
class SessionState:
    """
    State container that flows through the processing pipeline.
    
    Acts as the 'Context' object, carrying data between independent transformation steps.
    
    Attributes:
        config (AppConfig): The active application configuration.
        raw_df (pd.DataFrame): Unfiltered header data exactly as read from disk.
        processed_df (pd.DataFrame): Normalized and cleaned data ready for aggregation.
        aggregated_df (pd.DataFrame): Session-level statistics grouped for export.
        total_images_scanned (int): Counter for the number of files processed.
    """
    config: AppConfig
    raw_df: pd.DataFrame = field(default_factory=pd.DataFrame)
    processed_df: pd.DataFrame = field(default_factory=pd.DataFrame)
    aggregated_df: pd.DataFrame = field(default_factory=pd.DataFrame)
    total_images_scanned: int = 0
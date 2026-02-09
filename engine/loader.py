"""
Configuration Loader Module - AstroBin Upload Utility v2.0.0

Responsible for the ingestion and normalization of the 'config.ini' file. 
This module ensures that the application's runtime settings are correctly 
mapped into the strongly-typed AppConfig model, providing validation and 
standardization for user-provided settings.
"""

import logging
from typing import Dict, Any, List
from configobj import ConfigObj
import os
from models import AppConfig
from constants import ConfigSections

class ConfigLoader:
    """
    Manages the loading and type-mapping of application configuration.
    """
    def __init__(self, logger: logging.Logger):
        """
        Initializes the loader.

        Args:
            logger (logging.Logger): Active application logger.
        """
        self.logger = logger

    def load(self, filepath: str) -> AppConfig:
        """
        Loads the config.ini and maps its hierarchical sections to AppConfig.
        
        This method performs key normalization (e.g., stripping spaces) and 
        type conversion (e.g., strings to booleans).

        Args:
            filepath (str): Path to the config file (e.g., 'config.ini').
            
        Returns:
            AppConfig: A validated configuration object.
            
        Raises:
            FileNotFoundError: If the configuration file cannot be found.
        """
        if not os.path.exists(filepath):
            self.logger.error(f"Configuration file missing: {filepath}")
            raise FileNotFoundError(f"Configuration file {filepath} is required for operation.")

        # Load the raw INI file using ConfigObj for better section management
        config_obj = ConfigObj(filepath, encoding='utf-8')
        
        # Normalize top-level section keys to lowercase for consistent dictionary access
        normalized = {k.lower(): v for k, v in config_obj.items()}
        
        # Extract and validate the 'USEOBSDATE' flag (Default: True)
        defaults_sec = normalized.get(ConfigSections.DEFAULTS, {})
        use_obs_date = str(defaults_sec.get('USEOBSDATE', 'True')).lower() == 'true'
        
        self.logger.info(f"Configuration loaded and normalized from {filepath}")
        
        return AppConfig(
            defaults=self._normalize_defaults(defaults_sec),
            overrides=self._normalize_overrides(normalized.get(ConfigSections.OVERRIDE, {})),
            filters=normalized.get(ConfigSections.FILTERS, {}),
            sites=normalized.get(ConfigSections.SITES, {}),
            secret=normalized.get(ConfigSections.SECRET, {}),
            use_obs_date=use_obs_date
        )

    def _normalize_defaults(self, defaults: Dict[str, Any]) -> Dict[str, Any]:
        """
        Standardizes keys within the [defaults] section.
        
        Converts keys like 'Image Type' or 'exposure' into standardized 
        uppercase FITS-style keys like 'IMAGETYP' and 'EXPOSURE'.
        """
        return {k.upper().replace(' ', ''): v for k, v in defaults.items()}

    def _normalize_overrides(self, overrides: Dict[str, Any]) -> Dict[str, List[str]]:
        """
        Processes the [override] section for keyword remapping.
        
        Handles both single keyword strings and comma-separated lists, 
        ensuring they are all returned as lists of strings.
        """
        result = {}
        for k, v in overrides.items():
            if isinstance(v, str):
                # Split comma-separated values and strip whitespace
                result[k] = [item.strip() for item in v.split(',')]
            else:
                # Handle single non-string values by wrapping them in a list
                result[k] = [str(v).strip()]
        return result

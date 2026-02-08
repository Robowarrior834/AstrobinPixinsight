"""
Configuration Loader - AstroBin Upload Utility v2.0.0

Handles the loading, validation, and normalization of the config.ini file.
Transforms raw INI data into a strongly-typed AppConfig object.
"""

import logging
from typing import Dict, Any, List
from configobj import ConfigObj
import os
from models import AppConfig
from constants import ConfigSections

class ConfigLoader:
    """
    Responsible for reading and normalizing the application configuration.
    """
    def __init__(self, logger: logging.Logger):
        self.logger = logger

    def load(self, filepath: str) -> AppConfig:
        """
        Loads the config.ini and maps it to the AppConfig model.
        
        Args:
            filepath (str): Path to the config file.
            
        Returns:
            AppConfig: The validated configuration object.
            
        Raises:
            FileNotFoundError: If the config file is missing.
        """
        if not os.path.exists(filepath):
            self.logger.error(f"Configuration file missing: {filepath}")
            raise FileNotFoundError(f"Configuration file {filepath} is required.")

        # Load raw INI using configobj
        config_obj = ConfigObj(filepath, encoding='utf-8')
        
        # Normalize top-level section names to lowercase for consistent access
        normalized = {k.lower(): v for k, v in config_obj.items()}
        
        # Extract and validate the USEOBSDATE boolean
        defaults_sec = normalized.get(ConfigSections.DEFAULTS, {})
        use_obs_date = str(defaults_sec.get('USEOBSDATE', 'True')).lower() == 'true'
        
        self.logger.info(f"Configuration loaded from {filepath}")
        
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
        Sanitizes keys in the [defaults] section.
        Converts keys like 'Image Type' to 'IMAGETYP'.
        """
        return {k.upper().replace(' ', ''): v for k, v in defaults.items()}

    def _normalize_overrides(self, overrides: Dict[str, Any]) -> Dict[str, List[str]]:
        """
        Processes the [override] section.
        Supports both single strings and comma-separated lists of hardware keywords.
        """
        result = {}
        for k, v in overrides.items():
            if isinstance(v, str):
                result[k] = [item.strip() for item in v.split(',')]
            else:
                result[k] = [str(v).strip()]
        return result
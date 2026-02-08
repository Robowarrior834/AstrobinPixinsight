"""
General Utility Module - AstroBin Upload Utility v2.0.0

Provides logging initialization and common helper functions for the application.
"""

import logging
import inspect
import os
import pandas as pd
from datetime import datetime
from typing import Tuple, Union, Optional
import numpy as np

# Version tracking for internal consistency
utils_version = '2.0.0'

def initialise_logging(log_filename: str, logger: logging.Logger = None) -> logging.Logger:
    """
    Initializes a robust logging system that captures function names and line numbers.
    
    This logger:
    1. Creates the log directory if it doesn't exist.
    2. Clears previous session logs to keep files concise.
    3. Uses a custom 'FunctionNameFilter' to resolve the true calling function.
    4. Sets up a FileHandler with UTF-8 encoding.

    Args:
        log_filename (str): The absolute path where the log file will be stored.
        logger (logging.Logger, optional): Pre-initialization logger for boot errors.

    Returns:
        logging.Logger: The configured application logger.
    """
    class FunctionNameFilter(logging.Filter):
        def filter(self, record):
            stack = inspect.stack()
            for frame_info in stack:
                if frame_info.filename != __file__ and 'logging' not in frame_info.filename:
                    record.funcname = frame_info.function
                    break
            else:
                record.funcname = 'unknown'
            return True

    try:
        log_dir = os.path.dirname(log_filename)
        if log_dir and not os.path.exists(log_dir):
            os.makedirs(log_dir, exist_ok=True)

        # Truncate existing log to start fresh
        with open(log_filename, 'w', encoding='utf-8') as f:
            f.write('')

        new_logger = logging.getLogger("AstroBinV2")
        new_logger.handlers.clear()
        new_logger.setLevel(logging.INFO)

        handler = logging.FileHandler(log_filename, encoding='utf-8')
        formatter = logging.Formatter(
            '%(asctime)s - %(funcname)s - Line: %(lineno)d - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        handler.setFormatter(formatter)
        handler.addFilter(FunctionNameFilter())
        new_logger.addHandler(handler)

        new_logger.info("Logging initialized successfully.")
        return new_logger

    except Exception as e:
        print(f"CRITICAL: Failed to initialize logging: {e}")
        return logging.getLogger()

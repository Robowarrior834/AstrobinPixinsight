"""
General Utility Module - AstroBin Upload Utility v2.0.0

Provides shared infrastructure for the application, including a robust 
logging system and common helper functions. This module ensures that 
errors and process milestones are captured with sufficient context 
(file names, function names, and line numbers).
"""

import logging
import inspect
import os
import pandas as pd
from datetime import datetime
from typing import Tuple, Union, Optional
import numpy as np

# Version tracking for internal consistency across the utility suite
utils_version = '2.0.0'

def initialise_logging(log_filename: str, logger: logging.Logger = None) -> logging.Logger:
    """
    Initializes a professional logging system with automatic context resolution.
    
    This logger includes:
    1.  **Context Filtering**: Automatically identifies the calling function 
        and line number, even when called through wrappers.
    2.  **Session Isolation**: Truncates the previous log to ensure each 
        run is clear and concise.
    3.  **Directory Handling**: Proactively creates the log directory if missing.

    Args:
        log_filename (str): The absolute path for the log file.
        logger (logging.Logger, optional): A preliminary logger for boot errors.

    Returns:
        logging.Logger: The configured application-level logger.
    """
    class FunctionNameFilter(logging.Filter):
        """
        Custom filter to inject the true calling function name into log records.
        Useful for tracing logic flow across the modular pipeline steps.
        """
        def filter(self, record):
            stack = inspect.stack()
            # Traverse the stack to find the first frame outside this module and logging internals
            for frame_info in stack:
                if frame_info.filename != __file__ and 'logging' not in frame_info.filename:
                    record.funcname = frame_info.function
                    break
            else:
                record.funcname = 'unknown'
            return True

    try:
        # Ensure the destination folder exists
        log_dir = os.path.dirname(log_filename)
        if log_dir and not os.path.exists(log_dir):
            os.makedirs(log_dir, exist_ok=True)

        # Truncate existing log to start fresh for the current session
        with open(log_filename, 'w', encoding='utf-8') as f:
            f.write('')

        # Create or retrieve the named logger
        new_logger = logging.getLogger("AstroBinV2")
        new_logger.handlers.clear() # Prevent duplicate handlers in interactive environments
        new_logger.setLevel(logging.INFO)

        # Configure File Handler with UTF-8 support for astronomical symbols
        handler = logging.FileHandler(log_filename, encoding='utf-8')
        formatter = logging.Formatter(
            '%(asctime)s - %(funcname)s - Line: %(lineno)d - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        handler.setFormatter(formatter)
        handler.addFilter(FunctionNameFilter())
        new_logger.addHandler(handler)

        new_logger.info("Logging system initialized successfully.")
        return new_logger

    except Exception as e:
        # Fallback to a basic console logger if file initialization fails
        print(f"CRITICAL ERROR: Failed to initialize logging: {e}")
        return logging.getLogger()
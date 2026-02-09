"""
Legacy Orchestration Pipeline - AstroBin Upload Utility v2.0.0

This module contains the AstroBinProcessor class, which was the primary 
orchestrator for the v1.x series and early v2.0 prototypes. While the 
application has moved toward a more granular Step-based architecture 
(see engine/processor.py), this class remains as a reference for the 
monolithic orchestration pattern.
"""

import logging
import pandas as pd
from typing import List, Dict, Tuple, Any
from configobj import ConfigObj
from constants import ConfigSections, FITSKeywords, InternalColumns
# Note: These imports refer to legacy functional modules
from headers_functions import process_directories, condition_headers
from sites_functions import get_site_data
from processing_functions import aggregate_parameters, create_astrobin_output
from utils import summarize_session

class AstroBinProcessor:
    """
    Legacy Orchestrator for the AstroBin Upload workflow.
    
    This class encapsulates the state and sequential execution logic 
    required to transform raw FITS headers into a finalized AstroBin 
    acquisition report.
    """

    def __init__(self, config: ConfigObj, logger: logging.Logger):
        """
        Initializes the processor with configuration and logging.

        Args:
            config (ConfigObj): The raw configuration object from config.ini.
            logger (logging.Logger): The active application logger.
        """
        self.config = config
        self.logger = logger
        
        # Internal state containers passed to legacy functional modules
        self.headers_state: Dict[str, Any] = {}
        self.processing_state: Dict[str, Any] = {}
        self.sites_state: Dict[str, Any] = {}
        
        # Data storage for different stages of the pipeline
        self.raw_headers: pd.DataFrame = pd.DataFrame()
        self.conditioned_headers: pd.DataFrame = pd.DataFrame()
        self.aggregated_data: pd.DataFrame = pd.DataFrame()
        self.astrobin_data: pd.DataFrame = pd.DataFrame()
        self.session_summary: str = ""

    def initialize_states(self, precision: int = 4):
        """
        Prepares the state dictionaries required by legacy functions.
        
        This method acts as a bridge, converting the modern ConfigObj into 
        the specific dictionary formats expected by the original 
        functional logic.

        Args:
            precision (int): Decimal precision for coordinate rounding.
        """
        from headers_functions import initialize_headers
        from processing_functions import initialize_processing
        from sites_functions import initialize_sites

        # initialize_headers sets up standard keywords and normalization rules
        self.headers_state = initialize_headers(self.config, self.logger, precision)
        
        # initialize_processing prepares aggregation rules based on headers
        self.processing_state = initialize_processing(self.headers_state, self.logger)
        
        # initialize_sites prepares the local sites database lookup
        self.sites_state = initialize_sites(self.headers_state, self.logger)

    def load_headers(self, directory_paths: List[str], test_file: str = None, output_dir: str = None):
        """
        Loads and conditions metadata from files or diagnostic CSVs.

        Args:
            directory_paths (List[str]): List of paths to scan for FITS/XISF.
            test_file (str, optional): Filename of a CSV to inject instead of scanning.
            output_dir (str, optional): Directory where the test_file is located.
        """
        if test_file:
            import os
            # Build the path to the diagnostic CSV
            test_csv_path = os.path.join(output_dir, test_file)
            self.logger.info(f"Test mode: Loading headers from {test_csv_path}")
            
            # Read and normalize column names to uppercase to match legacy expectations
            self.raw_headers = pd.read_csv(test_csv_path)
            self.raw_headers.columns = [c.upper() for c in self.raw_headers.columns]
            
            # The legacy conditioning function expects a list of dictionaries
            headers_list = self.raw_headers.to_dict('records')
            self.conditioned_headers = condition_headers(headers_list, self.headers_state)
            
            # Validation logging
            if not self.conditioned_headers.empty and FITSKeywords.IMAGE_TYPE in self.conditioned_headers.columns:
                light_count = len(self.conditioned_headers[self.conditioned_headers[FITSKeywords.IMAGE_TYPE] == 'LIGHT'])
                self.logger.info(f"Number of 'LIGHT' frames (post-conditioning): {light_count}")
        else:
            # Recursive scan and extraction from the file system
            self.conditioned_headers, self.raw_headers = process_directories(directory_paths, self.headers_state)

    def process_data(self):
        """
        Executes the data transformation sequence.
        
        Logic Flow: 
        1. Site Identification -> 2. Statistic Aggregation -> 3. Report Generation
        """
        # 1. Site Data: Map coordinates to human-readable site names and Bortle scales
        self.logger.info("Getting site data...")
        site_modified_df = get_site_data(self.conditioned_headers, self.sites_state)
        
        # 2. Aggregation: Group frames by filter/session and calculate mean stats (FWHM, Temp)
        self.logger.info("Aggregating parameters...")
        self.aggregated_data = aggregate_parameters(site_modified_df, self.processing_state)
        
        # 3. Summary Generation: Create the text-based summary for the user
        self.logger.info("Summarizing session...")
        self.session_summary = summarize_session(
            self.aggregated_data, 
            self.logger, 
            self.headers_state.get('number_of_images_processed', 0)
        )
        
        # 4. AstroBin CSV: Format the aggregated data into the specific AstroBin acquisition CSV layout
        self.logger.info("Creating AstroBin output...")
        self.astrobin_data = create_astrobin_output(self.aggregated_data, self.processing_state)

    def export_results(self, output_dir_path: str, session_name: str):
        """
        Writes the processed metadata and summaries to the file system.

        Args:
            output_dir_path (str): The destination directory for reports.
            session_name (str): Prefix used for generated filenames.
        """
        import os
        
        # Append the finalized acquisition table to the bottom of the summary text report
        df_string = self.astrobin_data.to_string(index=False).replace('\n', '\n ')
        output_csv_name = f"{session_name}_acquisition.csv"
        self.session_summary += f"\n{output_csv_name}\n\n{df_string}\n"

        # Save the primary Session Summary (.txt)
        summary_file = os.path.join(output_dir_path, f"{session_name}_session_summary.txt")
        with open(summary_file, 'w', encoding='utf-8') as file:
            file.write(self.session_summary)
        
        # Save the AstroBin Acquisition Data (.csv)
        if not self.astrobin_data.empty:
            output_csv_path = os.path.join(output_dir_path, output_csv_name)
            self.astrobin_data.to_csv(output_csv_path, index=False)
            self.logger.info(f"AstroBin data exported to {output_csv_path}")
        else:
            self.logger.info("No AstroBin data to export.")

        # Output to console for immediate user feedback
        print(self.session_summary)
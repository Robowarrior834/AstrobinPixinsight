
import logging
import pandas as pd
from typing import List, Dict, Tuple, Any
from configobj import ConfigObj
from constants import ConfigKeys, FITSKeywords, InternalNames, StandardizedKeys
from headers_functions import process_directories, condition_headers
from sites_functions import get_site_data
from processing_functions import aggregate_parameters, create_astrobin_output
from utils import summarize_session

class AstroBinProcessor:
    """
    Orchestrates the entire AstroBin Upload workflow.
    Encapsulates state, configuration, and data transformations.
    """

    def __init__(self, config: ConfigObj, logger: logging.Logger):
        self.config = config
        self.logger = logger
        self.headers_state: Dict[str, Any] = {}
        self.processing_state: Dict[str, Any] = {}
        self.sites_state: Dict[str, Any] = {}
        self.raw_headers: pd.DataFrame = pd.DataFrame()
        self.conditioned_headers: pd.DataFrame = pd.DataFrame()
        self.aggregated_data: pd.DataFrame = pd.DataFrame()
        self.astrobin_data: pd.DataFrame = pd.DataFrame()
        self.session_summary: str = ""

    def initialize_states(self, precision: int = 4):
        """Initializes internal state dictionaries required by legacy functions."""
        from headers_functions import initialize_headers
        from processing_functions import initialize_processing
        from sites_functions import initialize_sites

        self.headers_state = initialize_headers(self.config, self.logger, precision)
        self.processing_state = initialize_processing(self.headers_state, self.logger)
        self.sites_state = initialize_sites(self.headers_state, self.logger)

    def load_headers(self, directory_paths: List[str], test_file: str = None, output_dir: str = None):
        """Loads headers from directories or a test CSV."""
        if test_file:
            import os
            test_csv_path = os.path.join(output_dir, test_file)
            self.logger.info(f"Test mode: Loading headers from {test_csv_path}")
            self.raw_headers = pd.read_csv(test_csv_path)
            # Normalize to uppercase matches strict expectation of condition_headers
            self.raw_headers.columns = [c.upper() for c in self.raw_headers.columns]
            
            # Convert to dict list for condition_headers input
            headers_list = self.raw_headers.to_dict('records')
            self.conditioned_headers = condition_headers(headers_list, self.headers_state)
            
            if not self.conditioned_headers.empty and FITSKeywords.IMAGE_TYPE in self.conditioned_headers.columns:
                light_count = len(self.conditioned_headers[self.conditioned_headers[FITSKeywords.IMAGE_TYPE] == 'LIGHT'])
                self.logger.info(f"Number of 'LIGHT' frames (post-conditioning): {light_count}")
        else:
            self.conditioned_headers, self.raw_headers = process_directories(directory_paths, self.headers_state)

    def process_data(self):
        """Executes the data processing pipeline: Sites -> Aggregation -> Output."""
        # 1. Site Data
        self.logger.info("Getting site data...")
        site_modified_df = get_site_data(self.conditioned_headers, self.sites_state)
        
        # 2. Aggregation
        self.logger.info("Aggregating parameters...")
        self.aggregated_data = aggregate_parameters(site_modified_df, self.processing_state)
        
        # 3. Summary Generation
        self.logger.info("Summarizing session...")
        self.session_summary = summarize_session(
            self.aggregated_data, 
            self.logger, 
            self.headers_state.get('number_of_images_processed', 0)
        )
        
        # 4. AstroBin CSV
        self.logger.info("Creating AstroBin output...")
        self.astrobin_data = create_astrobin_output(self.aggregated_data, self.processing_state)

    def export_results(self, output_dir_path: str, session_name: str):
        """Writes the results to disk."""
        import os
        
        # Append AstroBin table to summary text
        df_string = self.astrobin_data.to_string(index=False).replace('\n', '\n ')
        output_csv_name = f"{session_name}_acquisition.csv"
        self.session_summary += f"\n{output_csv_name}\n\n{df_string}\n"

        # Save Summary Text
        summary_file = os.path.join(output_dir_path, f"{session_name}_session_summary.txt")
        with open(summary_file, 'w', encoding='utf-8') as file:
            file.write(self.session_summary)
        
        # Save AstroBin CSV
        if not self.astrobin_data.empty:
            output_csv_path = os.path.join(output_dir_path, output_csv_name)
            self.astrobin_data.to_csv(output_csv_path, index=False)
            self.logger.info(f"AstroBin data exported to {output_csv_path}")
        else:
            self.logger.info("No AstroBin data to export.")

        print(self.session_summary)

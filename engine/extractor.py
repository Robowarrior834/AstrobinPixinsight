"""
Header Extractor - AstroBin Upload Utility v2.0.0

Manages the high-speed, parallelized extraction of metadata from FITS, XISF, 
and CSV files. Optimized for multi-core CPUs and RAID 0 storage.
"""

import os
import logging
import pandas as pd
from typing import List, Optional, Dict, Any
from concurrent.futures import ProcessPoolExecutor, as_completed
from astropy.io import fits
import struct
import xml.etree.ElementTree as ET
from constants import FITSKeywords

class HeaderExtractor:
    """
    Orchestrates the discovery and reading of astronomical metadata.
    """
    def __init__(self, logger: logging.Logger, config: Any):
        self.logger = logger
        self.config = config

    def extract_from_directories(self, paths: List[str]) -> pd.DataFrame:
        """
        Recursively scans directories and reads FITS/XISF headers in parallel.
        
        Args:
            paths (List[str]): List of directory paths to scan.
            
        Returns:
            pd.DataFrame: Raw metadata collected from all valid files.
        """
        file_paths = []
        for path in paths:
            self.logger.info(f"Scanning directory: {path}")
            for root, _, files in os.walk(path, followlinks=True):
                for file in files:
                    if file.lower().endswith(('.fits', '.fit', '.fts', '.xisf')):
                        file_paths.append(os.path.join(root, file))

        total = len(file_paths)
        headers = []
        
        # Parallel Execution: Utilize ProcessPoolExecutor for CPU-bound XML/FITS parsing
        with ProcessPoolExecutor() as executor:
            futures = {executor.submit(self.extract_single_file, fp): fp for fp in file_paths}
            for i, future in enumerate(as_completed(futures), 1):
                res = future.result()
                if res:
                    headers.append(res)
                # Real-time console progress
                print(f"\rScanning files: {i} of {total}...", end="", flush=True)
        
        print("\n") # New line after scan complete
        self.logger.info(f"Extracted {len(headers)} valid headers.")
        return pd.DataFrame(headers)

    def extract_from_csv(self, csv_path: str) -> pd.DataFrame:
        """Loads metadata from a legacy diagnostic CSV."""
        self.logger.info(f"Injecting metadata from CSV: {csv_path}")
        df = pd.read_csv(csv_path)
        # Ensure columns match FITSKeyword standards (uppercase)
        df.columns = [c.upper() for c in df.columns]
        return df

    def extract_single_file(self, filepath: str) -> Optional[Dict[str, Any]]:
        """Worker function: Identifies file type and parses its header."""
        try:
            if filepath.lower().endswith(('.fits', '.fit', '.fts')):
                hdr = self._read_fits(filepath)
            elif filepath.lower().endswith('.xisf'):
                hdr = self._read_xisf(filepath)
            else:
                return None
            
            # Clean string values (strip quotes often found in FITS comments)
            return {k: v.strip("'").strip('"') if isinstance(v, str) else v for k, v in hdr.items()}
        except Exception as e:
            return None

    def _read_fits(self, filepath: str) -> Dict[str, Any]:
        """Reads a FITS file and identifies internal calibration frame counts."""
        with fits.open(filepath) as hdul:
            hdr = dict(hdul[0].header)
            hdr[FITSKeywords.FILENAME] = os.path.basename(filepath)
            hdr[FITSKeywords.NUMBER] = self._get_fit_number(hdr)
            return hdr

    def _read_xisf(self, filepath: str) -> Dict[str, Any]:
        """Parses the XML header of a PixInsight XISF file."""
        with open(filepath, 'rb') as f:
            f.read(8) # signature 'XISF0100'
            length = struct.unpack('<I', f.read(4))[0]
            f.read(4) # reserved
            xml_str = f.read(length).decode('utf-8', errors='ignore')
            
        root = ET.fromstring(xml_str)
        ns = {'xisf': 'http://www.pixinsight.com/xisf'}
        # Collect FITSKeyword elements from XML
        hdr = {kw.get('name'): kw.get('value') for kw in root.findall('.//xisf:FITSKeyword', ns)}
        hdr[FITSKeywords.FILENAME] = os.path.basename(filepath)
        
        # Deep inspection for Master sub-exposure counts
        hdr[FITSKeywords.NUMBER] = 1
        prop = root.find(".//xisf:Property[@id='PixInsight:ProcessingHistory']", ns)
        if prop is not None and prop.text:
            try:
                hist_root = ET.fromstring(prop.text)
                table = hist_root.find(".//table[@id='images']")
                if table is not None:
                    hdr[FITSKeywords.NUMBER] = int(table.get('rows', 1))
            except Exception: pass
            
        return hdr

    def _get_fit_number(self, hdr: Dict[str, Any]) -> int:
        """Searches FITS HISTORY for sub-exposure counts (PixInsight specific)."""
        history = hdr.get('HISTORY', [])
        if isinstance(history, str): history = [history]
        for line in history:
            if 'ImageIntegration.numberOfImages:' in line:
                try: return int(line.split()[-1])
                except Exception: pass
        return 1

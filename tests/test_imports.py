"""
Basic Import and Consistency Tests - AstroBin Upload Utility v2.0.0

Ensures that all core modules can be imported correctly and that 
internal version tracking is synchronized across the application.
"""

import sys
import os
import pytest

# Add the parent directory to sys.path to enable direct module imports during testing
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

def test_imports():
    """
    Verifies that all primary engine modules and entry points are importable.
    This catches syntax errors and missing dependency issues.
    """
    try:
        import AstroBinUpload
        from engine.loader import ConfigLoader
        from engine.extractor import HeaderExtractor
        from engine.processor import PipelineProcessor
        from engine.exporter import Exporter
        import utils
    except ImportError as e:
        pytest.fail(f"Core module import failed: {e}")

def test_version_consistency():
    """
    Ensures that the version string in the main script matches the utility module.
    Crucial for maintaining accurate session logs and user feedback.
    """
    import AstroBinUpload
    import utils
    
    # Standard synchronization check for v2.0.0 release
    assert AstroBinUpload.version == utils.utils_version, 
        f"Version mismatch: AstroBinUpload ({AstroBinUpload.version}) != utils ({utils.utils_version})"

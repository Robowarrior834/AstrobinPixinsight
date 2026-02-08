"""
Reports Module - AstroBin Upload Utility v2.0.0

Restores and refines the high-quality reporting engine from v1.4.x.
Handles the complex string formatting required for human-readable session summaries.
"""

import logging
import pandas as pd
from typing import Tuple, Union, List
from datetime import datetime
from constants import ImageType, InternalColumns

def seconds_to_hms(seconds: Union[int, float], logger: logging.Logger, aligned: bool = False) -> str:
    """Converts a duration in seconds into HH:MM:SS format."""
    try:
        hours = int(seconds // 3600)
        minutes = int((seconds % 3600) // 60)
        secs = float(seconds % 60)
        if aligned:
            return f"{hours:>6} hrs {minutes:>6} mins {secs:>6.2f} secs"
        return f"{hours} hrs {minutes} mins {secs:.2f} secs"
    except Exception:
        return "0 hrs 0 mins 0.00 secs"

def get_target_details(group: pd.DataFrame, logger: logging.Logger) -> str:
    """
    Identifies the target name or identifies a multi-panel mosaic.
    """
    target_format = " Target: {}"
    if group.empty: return target_format.format("No target data")
    
    unique_targets = group[InternalColumns.TARGET].dropna().unique()
    
    # Mosaic Detection: Identify if 'Panel' keyword is present in target names
    panels = [str(t) for t in unique_targets if 'Panel' in str(t)]
    if panels:
        base_name = panels[0].split('Panel')[0].strip()
        return target_format.format(f"{base_name} {len(panels)} Panel Mosaic")
    
    # For standard sessions, return the first identified target
    return target_format.format(unique_targets[0] if len(unique_targets) > 0 else "Unknown")

def get_equipment_used(group: pd.DataFrame, df: pd.DataFrame, logger: logging.Logger) -> str:
    """
    Generates a detailed list of hardware and software used in the session.
    """
    s = ["\nEquipment used:"]
    fmt = "\t{:<20}: {}"
    
    items = {
        'Telescope': InternalColumns.TELESCOPE,
        'Camera': InternalColumns.CAMERA,
        'Filterwheel': InternalColumns.FILTER_WHEEL,
        'Focuser': InternalColumns.FOCUSER,
        'Rotator': InternalColumns.ROTATOR_NAME
    }
    
    # Extract hardware names from the first light frame
    for label, col in items.items():
        if col in group.columns:
            val = group[col].iloc[0]
            if pd.notna(val) and str(val).lower() not in ['none', 'nan', '']:
                s.append(fmt.format(label, val))
            
    # Software Discovery: Check all processed files for capture software metadata
    sw_set = set(group[InternalColumns.SWCREATE].dropna().unique())
    sw_set.update(df[InternalColumns.SWCREATE].dropna().unique())
    
    if sw_set:
        sw_list = sorted(list(sw_set), reverse=True) 
        s.append(fmt.format("Capture software", sw_list.pop(0)))
        for item in sw_list:
            s.append(fmt.format("", item))
            
    return "\n".join(s) + "\n"

def get_observation_period(group: pd.DataFrame, logger: logging.Logger) -> str:
    """
    Summarizes the temporal coverage and ambient conditions of the session.
    """
    s = ["\nObservation period:"]
    fmt = "\t{:<25}: {}"
    
    # Broadcasted session statistics
    start = group[InternalColumns.START_DATE].iloc[0] if InternalColumns.START_DATE in group.columns else "N/A"
    end = group[InternalColumns.END_DATE].iloc[0] if InternalColumns.END_DATE in group.columns else "N/A"
    days = group[InternalColumns.NUM_DAYS].iloc[0] if InternalColumns.NUM_DAYS in group.columns else 0
    sessions = group[InternalColumns.SESSIONS].iloc[0] if InternalColumns.SESSIONS in group.columns else 0
    
    s.append(fmt.format("Start date", start))
    s.append(fmt.format("End date", end))
    s.append(fmt.format("Days", int(days)))
    s.append(fmt.format("Observation sessions", int(sessions)))
    
    # Temperature Statistics
    if InternalColumns.TEMP_MIN in group.columns:
        s.append(fmt.format("Min temperature", f"{group[InternalColumns.TEMP_MIN].min():.1f}\u00B0C"))
        s.append(fmt.format("Max temperature", f"{group[InternalColumns.TEMP_MAX].max():.1f}\u00B0C"))
        s.append(fmt.format("Mean temperature", f"{group[InternalColumns.TEMPERATURE].mean():.1f}\u00B0C"))
    
    return "\n".join(s) + "\n"

def format_image_type_table(group: pd.DataFrame, imagetype: str, logger: logging.Logger) -> Tuple[str, float]:
    """
    Builds the tabular representation of frame counts and exposures for a specific image type.
    """
    lines = []
    total_exposure = 0.0
    
    image_group = group[group[InternalColumns.IMAGE_TYPE] == imagetype].copy()
    if image_group.empty: return "", 0.0

    if imagetype == ImageType.LIGHT.value:
        lines.append(f"\n {imagetype}S:")
        # Lights are categorized by individual Target
        for target, t_group in image_group.groupby(InternalColumns.TARGET, observed=True):
            lines.append(f" Target: {target}\n")
            header = " {:<8} {:<8} {:<8} {:<12} {:<12} {:<12} {:<12} {:<15} {:<15}"
            lines.append(header.format("Filter", "Frames", "Gain", "Egain", "Mean FWHM", "Sensor Temp", "Mean Temp", "Exposure", "Total Exposure"))
            
            t_exposure = 0.0
            for _, row in t_group.iterrows():
                row_exposure = row[InternalColumns.NUMBER] * row[InternalColumns.DURATION]
                t_exposure += row_exposure
                
                gain_str = f"{float(row[InternalColumns.GAIN_MATCH]) * 0.1:.2f} dB"
                egain_str = f"{float(row[InternalColumns.EGAIN]):.2f} e/ADU"
                
                lines.append(header.format(
                    str(row[InternalColumns.FILTER_NAME]), int(row[InternalColumns.NUMBER]), gain_str, egain_str,
                    f"{row[InternalColumns.MEAN_FWHM]:.2f} arcsec", f"{row[InternalColumns.SENSOR_COOLING]:.1f}\u00B0C", f"{row[InternalColumns.TEMPERATURE]:.1f}\u00B0C",
                    f"{row[InternalColumns.DURATION]:.2f} secs", seconds_to_hms(row_exposure, logger, aligned=True)
                ))
            lines.append(f"\n Exposure time for {target}: {seconds_to_hms(t_exposure, logger)}\n")
            total_exposure += t_exposure
    else:
        # Calibration Frames use a simplified table format
        lines.append(f"\n {imagetype}S:\n")
        header = " {:<10} {:<8} {:<10} {:<15} {:<12} {:<15}"
        lines.append(header.format("Filter", "Frames", "Gain", "Egain", "Exposure", "Total Exposure"))
        for _, row in image_group.iterrows():
            row_exposure = row[InternalColumns.NUMBER] * row[InternalColumns.DURATION]
            total_exposure += row_exposure
            gain_str = f"{float(row[InternalColumns.GAIN_MATCH]) * 0.1:.2f} dB"
            egain_str = f"{float(row[InternalColumns.EGAIN]):.2f} e/ADU"
            lines.append(header.format(
                str(row[InternalColumns.FILTER_NAME]), int(row[InternalColumns.NUMBER]), gain_str, egain_str,
                f"{row[InternalColumns.DURATION]:.2f} secs", seconds_to_hms(row_exposure, logger, aligned=True)
            ))
            
    return "\n".join(lines), total_exposure

def generate_full_summary(df: pd.DataFrame, logger: logging.Logger, total_scanned: int) -> str:
    """
    Orchestrates the creation of the full multi-section session summary.
    """
    if df.empty: return "No data available."
    
    report = []
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    report.append(f"Observation session summary\nGenerated {now}")
    
    # Iterate through each Site to generate its specific sub-report
    for site, site_group in df.groupby(InternalColumns.SITE_NAME, observed=True):
        lights = site_group[site_group[InternalColumns.IMAGE_TYPE] == ImageType.LIGHT.value]
        if lights.empty: continue
        
        report.append(get_target_details(lights, logger))
        
        # Site Metadata Section
        report.append(f"\nSite: {site}")
        report.append(f"\tLatitude: {site_group[InternalColumns.SITE_LAT].iloc[0]:.4f}\u00B0")
        report.append(f"\tLongitude: {site_group[InternalColumns.SITE_LONG].iloc[0]:.4f}\u00B0")
        report.append(f"\tBortle scale: {site_group[InternalColumns.BORTLE].iloc[0]:.1f}")
        report.append(f"\tSQM: {site_group[InternalColumns.MEAN_SQM].iloc[0]:.2f} mag/arcsec²")
        
        report.append(get_equipment_used(lights, df, logger))
        report.append(get_observation_period(lights, logger))
        
        # Sequentially process and format each Image Type section
        order = [ImageType.LIGHT.value, ImageType.FLAT.value, ImageType.MASTER_FLAT.value, ImageType.BIAS.value, ImageType.DARK.value]
        for itype in order:
            if itype in site_group[InternalColumns.IMAGE_TYPE].unique():
                table, exp = format_image_type_table(site_group, itype, logger)
                if table:
                    report.append(table)
                    report.append(f"\nTotal {itype} Exposure Time: {seconds_to_hms(exp, logger)}\n")
                    
    report.append(f"\n Total number of images processed: {total_scanned}\n")
    return "\n".join(report)
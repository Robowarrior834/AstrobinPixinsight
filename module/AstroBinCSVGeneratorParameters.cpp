//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.0.0
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorParameters.cpp - Generated 2026-08-12
// ----------------------------------------------------------------------------
// This file is part of the AstroBinCSVGenerator PixInsight module.
//
// Copyright (c) 2026 Jamie Robinson
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ----------------------------------------------------------------------------

#include "AstroBinCSVGeneratorParameters.h"

namespace pcl
{

// ----------------------------------------------------------------------------
// Default filter name-to-AstroBin-ID mapping, mirroring the mapping used by
// the AstroBin CSV Generator PixInsight script (v1.2.5).
// ----------------------------------------------------------------------------

const char* DEFAULT_FILTER_MAP_JSON =
   "{"
      "\"Ha\":4663,"
      "\"SII\":4844,"
      "\"OIII\":4752,"
      "\"Red\":4649,"
      "\"Green\":4643,"
      "\"Blue\":4637,"
      "\"Lum\":2906,"
      "\"L\":2906,"
      "\"CLS\":4632,"
      "\"H-alpha\":4663,"
      "\"Halpha\":4663,"
      "\"Sulfur\":4844,"
      "\"S-II\":4844,"
      "\"O-III\":4752,"
      "\"Clear\":2906,"
      "\"UV\":3056,"
      "\"IR\":3054,"
      "\"Exoplanet\":10022"
   "}";

// ----------------------------------------------------------------------------

InputDirectoryParameter*   TheInputDirectoryParameter   = nullptr;
OutputDirectoryParameter*  TheOutputDirectoryParameter  = nullptr;
OutputFileNameParameter*   TheOutputFileNameParameter   = nullptr;
RecursiveParameter*        TheRecursiveParameter        = nullptr;
SessionGapHoursParameter*  TheSessionGapHoursParameter  = nullptr;
OverrideFilePathParameter* TheOverrideFilePathParameter = nullptr;

SiteNameParameter*         TheSiteNameParameter         = nullptr;
SiteLatitudeParameter*     TheSiteLatitudeParameter     = nullptr;
SiteLongitudeParameter*    TheSiteLongitudeParameter    = nullptr;
SiteElevationParameter*    TheSiteElevationParameter    = nullptr;
BortleParameter*           TheBortleParameter           = nullptr;
SQMParameter*              TheSQMParameter              = nullptr;
FocalLengthParameter*      TheFocalLengthParameter      = nullptr;
PixelSizeParameter*        ThePixelSizeParameter        = nullptr;
FocalRatioParameter*       TheFocalRatioParameter       = nullptr;
ShiftOvernightParameter*   TheShiftOvernightParameter   = nullptr;
UseObservingDateParameter* TheUseObservingDateParameter = nullptr;
DefaultGainParameter*      TheDefaultGainParameter      = nullptr;
DefaultTemperatureParameter* TheDefaultTemperatureParameter = nullptr;
KeywordOverridesParameter* TheKeywordOverridesParameter = nullptr;
DefaultFilterParameter*    TheDefaultFilterParameter    = nullptr;
UseDefaultFilterParameter* TheUseDefaultFilterParameter = nullptr;
FilterMapParameter*        TheFilterMapParameter        = nullptr;
FilterDatabasePathParameter* TheFilterDatabasePathParameter = nullptr;

// ----------------------------------------------------------------------------
// Input / Output parameters
// ----------------------------------------------------------------------------

InputDirectoryParameter::InputDirectoryParameter( MetaProcess* P ) : MetaString( P )
{
   TheInputDirectoryParameter = this;
}

IsoString InputDirectoryParameter::Id() const
{
   return "inputDirectory";
}

String InputDirectoryParameter::Description() const
{
   return "Directory containing the FITS/XISF light frames to process.";
}

// ----------------------------------------------------------------------------

OutputDirectoryParameter::OutputDirectoryParameter( MetaProcess* P ) : MetaString( P )
{
   TheOutputDirectoryParameter = this;
}

IsoString OutputDirectoryParameter::Id() const
{
   return "outputDirectory";
}

String OutputDirectoryParameter::Description() const
{
   return "Directory where the acquisition CSV file will be written. If empty, the "
          "same directory as the input files is used.";
}

// ----------------------------------------------------------------------------

OutputFileNameParameter::OutputFileNameParameter( MetaProcess* P ) : MetaString( P )
{
   TheOutputFileNameParameter = this;
}

IsoString OutputFileNameParameter::Id() const
{
   return "outputFileName";
}

String OutputFileNameParameter::DefaultValue() const
{
   return "acquisition.csv";
}

String OutputFileNameParameter::Description() const
{
   return "File name of the generated AstroBin acquisition CSV file.";
}

// ----------------------------------------------------------------------------

RecursiveParameter::RecursiveParameter( MetaProcess* P ) : MetaBoolean( P )
{
   TheRecursiveParameter = this;
}

IsoString RecursiveParameter::Id() const
{
   return "recursive";
}

bool RecursiveParameter::DefaultValue() const
{
   return true;
}

String RecursiveParameter::Description() const
{
   return "Recursively search subdirectories for FITS/XISF files.";
}

// ----------------------------------------------------------------------------

SessionGapHoursParameter::SessionGapHoursParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheSessionGapHoursParameter = this;
}

IsoString SessionGapHoursParameter::Id() const
{
   return "sessionGapHours";
}

double SessionGapHoursParameter::MinimumValue() const
{
   return 0.1;
}

double SessionGapHoursParameter::DefaultValue() const
{
   return 5;
}

int SessionGapHoursParameter::Precision() const
{
   return 2;
}

String SessionGapHoursParameter::Description() const
{
   return "Minimum time gap (hours) between consecutive frames that defines a "
          "new imaging session.";
}

// ----------------------------------------------------------------------------

OverrideFilePathParameter::OverrideFilePathParameter( MetaProcess* P ) : MetaString( P )
{
   TheOverrideFilePathParameter = this;
}

IsoString OverrideFilePathParameter::Id() const
{
   return "overrideFilePath";
}

String OverrideFilePathParameter::Description() const
{
   return "Optional CSV file with per-file keyword overrides. If empty, no "
          "overrides are applied.";
}

// ----------------------------------------------------------------------------
// Site and equipment parameters
// ----------------------------------------------------------------------------

SiteNameParameter::SiteNameParameter( MetaProcess* P ) : MetaString( P )
{
   TheSiteNameParameter = this;
}

IsoString SiteNameParameter::Id() const
{
   return "siteName";
}

String SiteNameParameter::DefaultValue() const
{
   return "My Site";
}

String SiteNameParameter::Description() const
{
   return "Name of the observing site.";
}

// ----------------------------------------------------------------------------

SiteLatitudeParameter::SiteLatitudeParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheSiteLatitudeParameter = this;
}

IsoString SiteLatitudeParameter::Id() const
{
   return "siteLatitude";
}

double SiteLatitudeParameter::MinimumValue() const
{
   return -90;
}

double SiteLatitudeParameter::MaximumValue() const
{
   return 90;
}

double SiteLatitudeParameter::DefaultValue() const
{
   return 0;
}

int SiteLatitudeParameter::Precision() const
{
   return 6;
}

String SiteLatitudeParameter::Description() const
{
   return "Latitude of the observing site, in degrees (N positive).";
}

// ----------------------------------------------------------------------------

SiteLongitudeParameter::SiteLongitudeParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheSiteLongitudeParameter = this;
}

IsoString SiteLongitudeParameter::Id() const
{
   return "siteLongitude";
}

double SiteLongitudeParameter::MinimumValue() const
{
   return -180;
}

double SiteLongitudeParameter::MaximumValue() const
{
   return 180;
}

double SiteLongitudeParameter::DefaultValue() const
{
   return 0;
}

int SiteLongitudeParameter::Precision() const
{
   return 6;
}

String SiteLongitudeParameter::Description() const
{
   return "Longitude of the observing site, in degrees (E positive).";
}

// ----------------------------------------------------------------------------

SiteElevationParameter::SiteElevationParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheSiteElevationParameter = this;
}

IsoString SiteElevationParameter::Id() const
{
   return "siteElevation";
}

double SiteElevationParameter::MinimumValue() const
{
   return -500;
}

double SiteElevationParameter::DefaultValue() const
{
   return 0;
}

int SiteElevationParameter::Precision() const
{
   return 1;
}

String SiteElevationParameter::Description() const
{
   return "Elevation of the observing site above sea level, in meters.";
}

// ----------------------------------------------------------------------------

BortleParameter::BortleParameter( MetaProcess* P ) : MetaInt32( P )
{
   TheBortleParameter = this;
}

IsoString BortleParameter::Id() const
{
   return "bortle";
}

double BortleParameter::MinimumValue() const
{
   return 1;
}

double BortleParameter::MaximumValue() const
{
   return 9;
}

double BortleParameter::DefaultValue() const
{
   return 4;
}

String BortleParameter::Description() const
{
   return "Bortle dark-sky class of the observing site, from 1 (excellent) to "
          "9 (inner city).";
}

// ----------------------------------------------------------------------------

SQMParameter::SQMParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheSQMParameter = this;
}

IsoString SQMParameter::Id() const
{
   return "sqm";
}

double SQMParameter::MinimumValue() const
{
   return 15;
}

double SQMParameter::DefaultValue() const
{
   return 21;
}

int SQMParameter::Precision() const
{
   return 2;
}

String SQMParameter::Description() const
{
   return "Sky Quality Meter reading of the observing site, in magnitudes per "
          "square arcsecond.";
}

// ----------------------------------------------------------------------------

FocalLengthParameter::FocalLengthParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheFocalLengthParameter = this;
}

IsoString FocalLengthParameter::Id() const
{
   return "focalLength";
}

double FocalLengthParameter::MinimumValue() const
{
   return 1;
}

double FocalLengthParameter::DefaultValue() const
{
   return 540;
}

int FocalLengthParameter::Precision() const
{
   return 1;
}

String FocalLengthParameter::Description() const
{
   return "Focal length of the imaging telescope, in millimeters.";
}

// ----------------------------------------------------------------------------

PixelSizeParameter::PixelSizeParameter( MetaProcess* P ) : MetaDouble( P )
{
   ThePixelSizeParameter = this;
}

IsoString PixelSizeParameter::Id() const
{
   return "pixelSize";
}

double PixelSizeParameter::MinimumValue() const
{
   return 0.1;
}

double PixelSizeParameter::DefaultValue() const
{
   return 3;
}

int PixelSizeParameter::Precision() const
{
   return 2;
}

String PixelSizeParameter::Description() const
{
   return "Pixel size of the imaging camera sensor, in microns.";
}

// ----------------------------------------------------------------------------

FocalRatioParameter::FocalRatioParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheFocalRatioParameter = this;
}

IsoString FocalRatioParameter::Id() const
{
   return "focalRatio";
}

double FocalRatioParameter::MinimumValue() const
{
   return 0.1;
}

double FocalRatioParameter::DefaultValue() const
{
   return 5;
}

int FocalRatioParameter::Precision() const
{
   return 2;
}

String FocalRatioParameter::Description() const
{
   return "Focal ratio of the imaging telescope (f-number).";
}

// ----------------------------------------------------------------------------

ShiftOvernightParameter::ShiftOvernightParameter( MetaProcess* P ) : MetaBoolean( P )
{
   TheShiftOvernightParameter = this;
}

IsoString ShiftOvernightParameter::Id() const
{
   return "shiftOvernight";
}

bool ShiftOvernightParameter::DefaultValue() const
{
   return true;
}

String ShiftOvernightParameter::Description() const
{
   return "Assign frames acquired after midnight to the previous calendar day, "
          "so an overnight session belongs to a single observing night.";
}

// ----------------------------------------------------------------------------

UseObservingDateParameter::UseObservingDateParameter( MetaProcess* P ) : MetaBoolean( P )
{
   TheUseObservingDateParameter = this;
}

IsoString UseObservingDateParameter::Id() const
{
   return "useObservingDate";
}

bool UseObservingDateParameter::DefaultValue() const
{
   return false;
}

String UseObservingDateParameter::Description() const
{
   return "Use the observing date (DATE-OBS) rather than the file modification "
          "date when sorting frames.";
}

// ----------------------------------------------------------------------------

DefaultGainParameter::DefaultGainParameter( MetaProcess* P ) : MetaInt32( P )
{
   TheDefaultGainParameter = this;
}

IsoString DefaultGainParameter::Id() const
{
   return "defaultGain";
}

double DefaultGainParameter::DefaultValue() const
{
   return 0;
}

String DefaultGainParameter::Description() const
{
   return "Default gain (ISO) value used when the GAIN header keyword is "
          "missing.";
}

// ----------------------------------------------------------------------------

DefaultTemperatureParameter::DefaultTemperatureParameter( MetaProcess* P ) : MetaDouble( P )
{
   TheDefaultTemperatureParameter = this;
}

IsoString DefaultTemperatureParameter::Id() const
{
   return "defaultTemperature";
}

double DefaultTemperatureParameter::DefaultValue() const
{
   return -10;
}

int DefaultTemperatureParameter::Precision() const
{
   return 1;
}

String DefaultTemperatureParameter::Description() const
{
   return "Default sensor temperature, in Celsius degrees, used when the "
          "CCD-TEMP header keyword is missing.";
}

// ----------------------------------------------------------------------------

KeywordOverridesParameter::KeywordOverridesParameter( MetaProcess* P ) : MetaString( P )
{
   TheKeywordOverridesParameter = this;
}

IsoString KeywordOverridesParameter::Id() const
{
   return "keywordOverrides";
}

String KeywordOverridesParameter::Description() const
{
   return "JSON object with FITS keyword overrides applied to all frames, "
          "e.g. {\"FOCALLEN\":540,\"XPIXSZ\":3.0}.";
}

// ----------------------------------------------------------------------------

DefaultFilterParameter::DefaultFilterParameter( MetaProcess* P ) : MetaString( P )
{
   TheDefaultFilterParameter = this;
}

IsoString DefaultFilterParameter::Id() const
{
   return "defaultFilter";
}

String DefaultFilterParameter::Description() const
{
   return "Filter name used as a fallback when the FILTER keyword is missing "
          "or cannot be mapped to an AstroBin filter ID.";
}

// ----------------------------------------------------------------------------

UseDefaultFilterParameter::UseDefaultFilterParameter( MetaProcess* P ) : MetaBoolean( P )
{
   TheUseDefaultFilterParameter = this;
}

IsoString UseDefaultFilterParameter::Id() const
{
   return "useDefaultFilter";
}

bool UseDefaultFilterParameter::DefaultValue() const
{
   return false;
}

String UseDefaultFilterParameter::Description() const
{
   return "Enable use of the default filter name as a fallback.";
}

// ----------------------------------------------------------------------------

FilterMapParameter::FilterMapParameter( MetaProcess* P ) : MetaString( P )
{
   TheFilterMapParameter = this;
}

IsoString FilterMapParameter::Id() const
{
   return "filterMap";
}

String FilterMapParameter::DefaultValue() const
{
   return DEFAULT_FILTER_MAP_JSON;
}

String FilterMapParameter::Description() const
{
   return "JSON object mapping filter names to AstroBin filter IDs, used as a "
          "fallback when the downloaded filter database is unavailable.";
}

// ----------------------------------------------------------------------------

FilterDatabasePathParameter::FilterDatabasePathParameter( MetaProcess* P ) : MetaString( P )
{
   TheFilterDatabasePathParameter = this;
}

IsoString FilterDatabasePathParameter::Id() const
{
   return "filterDatabasePath";
}

String FilterDatabasePathParameter::Description() const
{
   return "Path to the AstroBin filter database JSON cache file. If empty, the "
          "default path PixInsight/AstroBinFilters.json in the user's home "
          "directory is used.";
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorParameters.cpp

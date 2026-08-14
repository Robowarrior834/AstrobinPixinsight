//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorInstance.h - Generated 2026-08-12
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

#ifndef __AstroBinCSVGeneratorInstance_h
#define __AstroBinCSVGeneratorInstance_h

#include <pcl/ProcessImplementation.h>
#include <pcl/MetaParameter.h> // pcl_bool
#include <pcl/String.h>

namespace pcl
{

// ----------------------------------------------------------------------------

class AstroBinCSVGeneratorInstance : public ProcessImplementation
{
public:

   AstroBinCSVGeneratorInstance( const MetaProcess* );
   AstroBinCSVGeneratorInstance( const AstroBinCSVGeneratorInstance& );

   void Assign( const ProcessImplementation& ) override;
   bool CanExecuteOn( const View&, pcl::String& whyNot ) const override;
   bool CanExecuteGlobal( String& whyNot ) const override;
   bool ExecuteGlobal() override;
   void* LockParameter( const MetaParameter*, size_type /*tableRow*/ ) override;

private:

   // Input / output
   String   p_inputDirectory;    // directory with FITS/XISF light frames
   String   p_outputDirectory;   // directory for the CSV file (empty = same as input)
   String   p_outputFileName;    // name of the generated CSV file
   pcl_bool p_recursive;         // search subdirectories recursively
   double   p_sessionGapHours;   // minimum gap (hours) to define a new session
   String   p_overrideFilePath;  // per-file keyword overrides CSV file

   // Site and equipment
   String   p_siteName;          // observing site name
   double   p_siteLatitude;      // site latitude, degrees
   double   p_siteLongitude;     // site longitude, degrees
   double   p_siteElevation;     // site elevation, meters
   int32    p_bortle;            // Bortle class
   double   p_sqm;               // SQM reading
   double   p_focalLength;       // focal length, mm
   double   p_pixelSize;         // pixel size, microns
   double   p_focalRatio;        // focal ratio
   pcl_bool p_shiftOvernight;    // shift frames after midnight to previous day
   pcl_bool p_useObservingDate;  // use DATE-OBS instead of file mtime
   int32    p_defaultGain;       // fallback GAIN
   double   p_defaultTemperature;// fallback CCD-TEMP
   String   p_keywordOverrides;  // JSON object of keyword overrides
   String   p_defaultFilter;     // fallback filter name
   pcl_bool p_useDefaultFilter;  // enable default filter fallback
   String   p_filterMap;         // JSON object mapping filter names to AstroBin IDs
   String   p_filterDatabasePath;// path to the filter database JSON cache
   String   p_fileList;          // JSON array of {path, filterId, filterLabel}; when
                                 // non-empty, these files are processed instead of
                                 // scanning p_inputDirectory

   friend class AstroBinCSVGeneratorProcess;
   friend class AstroBinCSVGeneratorInterface;
   friend class ABCGSettingsDialog;
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __AstroBinCSVGeneratorInstance_h

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorInstance.h

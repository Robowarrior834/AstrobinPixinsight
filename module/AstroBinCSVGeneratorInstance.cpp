//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.0.0
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorInstance.cpp - Generated 2026-08-12
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

#include "AstroBinCSVGeneratorInstance.h"
#include "AstroBinCSVGeneratorParameters.h"
#include "AstroBinCSVGeneratorProcess.h"

#include <pcl/Console.h>
#include <pcl/Exception.h>

namespace pcl
{

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInstance::AstroBinCSVGeneratorInstance( const MetaProcess* m )
   : ProcessImplementation( m )
   , p_inputDirectory( TheInputDirectoryParameter->DefaultValue() )
   , p_outputDirectory( TheOutputDirectoryParameter->DefaultValue() )
   , p_outputFileName( TheOutputFileNameParameter->DefaultValue() )
   , p_recursive( TheRecursiveParameter->DefaultValue() )
   , p_sessionGapHours( TheSessionGapHoursParameter->DefaultValue() )
   , p_overrideFilePath( TheOverrideFilePathParameter->DefaultValue() )
   , p_siteName( TheSiteNameParameter->DefaultValue() )
   , p_siteLatitude( TheSiteLatitudeParameter->DefaultValue() )
   , p_siteLongitude( TheSiteLongitudeParameter->DefaultValue() )
   , p_siteElevation( TheSiteElevationParameter->DefaultValue() )
   , p_bortle( TheBortleParameter->DefaultValue() )
   , p_sqm( TheSQMParameter->DefaultValue() )
   , p_focalLength( TheFocalLengthParameter->DefaultValue() )
   , p_pixelSize( ThePixelSizeParameter->DefaultValue() )
   , p_focalRatio( TheFocalRatioParameter->DefaultValue() )
   , p_shiftOvernight( TheShiftOvernightParameter->DefaultValue() )
   , p_useObservingDate( TheUseObservingDateParameter->DefaultValue() )
   , p_defaultGain( TheDefaultGainParameter->DefaultValue() )
   , p_defaultTemperature( TheDefaultTemperatureParameter->DefaultValue() )
   , p_keywordOverrides( TheKeywordOverridesParameter->DefaultValue() )
   , p_defaultFilter( TheDefaultFilterParameter->DefaultValue() )
   , p_useDefaultFilter( TheUseDefaultFilterParameter->DefaultValue() )
   , p_filterMap( TheFilterMapParameter->DefaultValue() )
   , p_filterDatabasePath( TheFilterDatabasePathParameter->DefaultValue() )
{
}

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInstance::AstroBinCSVGeneratorInstance( const AstroBinCSVGeneratorInstance& x )
   : ProcessImplementation( x )
{
   Assign( x );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInstance::Assign( const ProcessImplementation& p )
{
   const AstroBinCSVGeneratorInstance* x = dynamic_cast<const AstroBinCSVGeneratorInstance*>( &p );
   if ( x != nullptr )
   {
      p_inputDirectory     = x->p_inputDirectory;
      p_outputDirectory    = x->p_outputDirectory;
      p_outputFileName     = x->p_outputFileName;
      p_recursive          = x->p_recursive;
      p_sessionGapHours    = x->p_sessionGapHours;
      p_overrideFilePath   = x->p_overrideFilePath;

      p_siteName           = x->p_siteName;
      p_siteLatitude       = x->p_siteLatitude;
      p_siteLongitude      = x->p_siteLongitude;
      p_siteElevation      = x->p_siteElevation;
      p_bortle             = x->p_bortle;
      p_sqm                = x->p_sqm;
      p_focalLength        = x->p_focalLength;
      p_pixelSize          = x->p_pixelSize;
      p_focalRatio         = x->p_focalRatio;
      p_shiftOvernight     = x->p_shiftOvernight;
      p_useObservingDate   = x->p_useObservingDate;
      p_defaultGain        = x->p_defaultGain;
      p_defaultTemperature = x->p_defaultTemperature;
      p_keywordOverrides   = x->p_keywordOverrides;
      p_defaultFilter      = x->p_defaultFilter;
      p_useDefaultFilter   = x->p_useDefaultFilter;
      p_filterMap          = x->p_filterMap;
      p_filterDatabasePath = x->p_filterDatabasePath;
   }
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInstance::CanExecuteOn( const View& view, pcl::String& whyNot ) const
{
   whyNot = "AstroBinCSVGenerator is a global process; it cannot be executed on views.";
   return false;
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInstance::CanExecuteGlobal( String& whyNot ) const
{
   return true;
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInstance::ExecuteGlobal()
{
   Console c;
   c.WriteLn( "<end><cbr>AstroBin CSV Generator: CSV generation engine not "
              "implemented yet." );
   return true;
}

// ----------------------------------------------------------------------------

void* AstroBinCSVGeneratorInstance::LockParameter( const MetaParameter* p, size_type /*tableRow*/ )
{
   if ( p == TheInputDirectoryParameter )
      return &p_inputDirectory;
   if ( p == TheOutputDirectoryParameter )
      return &p_outputDirectory;
   if ( p == TheOutputFileNameParameter )
      return &p_outputFileName;
   if ( p == TheRecursiveParameter )
      return &p_recursive;
   if ( p == TheSessionGapHoursParameter )
      return &p_sessionGapHours;
   if ( p == TheOverrideFilePathParameter )
      return &p_overrideFilePath;

   if ( p == TheSiteNameParameter )
      return &p_siteName;
   if ( p == TheSiteLatitudeParameter )
      return &p_siteLatitude;
   if ( p == TheSiteLongitudeParameter )
      return &p_siteLongitude;
   if ( p == TheSiteElevationParameter )
      return &p_siteElevation;
   if ( p == TheBortleParameter )
      return &p_bortle;
   if ( p == TheSQMParameter )
      return &p_sqm;
   if ( p == TheFocalLengthParameter )
      return &p_focalLength;
   if ( p == ThePixelSizeParameter )
      return &p_pixelSize;
   if ( p == TheFocalRatioParameter )
      return &p_focalRatio;
   if ( p == TheShiftOvernightParameter )
      return &p_shiftOvernight;
   if ( p == TheUseObservingDateParameter )
      return &p_useObservingDate;
   if ( p == TheDefaultGainParameter )
      return &p_defaultGain;
   if ( p == TheDefaultTemperatureParameter )
      return &p_defaultTemperature;
   if ( p == TheKeywordOverridesParameter )
      return &p_keywordOverrides;
   if ( p == TheDefaultFilterParameter )
      return &p_defaultFilter;
   if ( p == TheUseDefaultFilterParameter )
      return &p_useDefaultFilter;
   if ( p == TheFilterMapParameter )
      return &p_filterMap;
   if ( p == TheFilterDatabasePathParameter )
      return &p_filterDatabasePath;

   return nullptr;
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorInstance.cpp

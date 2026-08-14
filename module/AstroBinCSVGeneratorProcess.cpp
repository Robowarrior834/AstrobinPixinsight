//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorProcess.cpp - Generated 2026-08-12
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

#include "AstroBinCSVGeneratorProcess.h"
#include "AstroBinCSVGeneratorParameters.h"
#include "AstroBinCSVGeneratorInstance.h"
#include "AstroBinCSVGeneratorInterface.h"

namespace pcl
{

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorProcess* TheAstroBinCSVGeneratorProcess = nullptr;

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorProcess::AstroBinCSVGeneratorProcess()
{
   TheAstroBinCSVGeneratorProcess = this;

   // Instantiate process parameters
   new InputDirectoryParameter( this );
   new OutputDirectoryParameter( this );
   new OutputFileNameParameter( this );
   new RecursiveParameter( this );
   new SessionGapHoursParameter( this );
   new OverrideFilePathParameter( this );

   new SiteNameParameter( this );
   new SiteLatitudeParameter( this );
   new SiteLongitudeParameter( this );
   new SiteElevationParameter( this );
   new BortleParameter( this );
   new SQMParameter( this );
   new FocalLengthParameter( this );
   new PixelSizeParameter( this );
   new FocalRatioParameter( this );
   new ShiftOvernightParameter( this );
   new UseObservingDateParameter( this );
   new DefaultGainParameter( this );
   new DefaultTemperatureParameter( this );
   new KeywordOverridesParameter( this );
   new DefaultFilterParameter( this );
   new UseDefaultFilterParameter( this );
   new FilterMapParameter( this );
   new FilterDatabasePathParameter( this );
   new FileListJSONParameter( this );
}

// ----------------------------------------------------------------------------

IsoString AstroBinCSVGeneratorProcess::Id() const
{
   return "AstroBinCSVGenerator";
}

// ----------------------------------------------------------------------------

IsoString AstroBinCSVGeneratorProcess::Category() const
{
   return "Utility";
}

// ----------------------------------------------------------------------------

uint32 AstroBinCSVGeneratorProcess::Version() const
{
   return 0x100;
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorProcess::IconImageSVGFile() const
{
   return "@module_icons_dir/AstroBinCSVGenerator.svg";
}

// ----------------------------------------------------------------------------

ProcessInterface* AstroBinCSVGeneratorProcess::DefaultInterface() const
{
   return TheAstroBinCSVGeneratorInterface;
}

// ----------------------------------------------------------------------------

ProcessImplementation* AstroBinCSVGeneratorProcess::Create() const
{
   return new AstroBinCSVGeneratorInstance( this );
}

// ----------------------------------------------------------------------------

ProcessImplementation* AstroBinCSVGeneratorProcess::Clone( const ProcessImplementation& p ) const
{
   const AstroBinCSVGeneratorInstance* instPtr = dynamic_cast<const AstroBinCSVGeneratorInstance*>( &p );
   return (instPtr != nullptr) ? new AstroBinCSVGeneratorInstance( *instPtr ) : nullptr;
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorProcess::IsAssignable() const
{
   return true;
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorProcess.cpp

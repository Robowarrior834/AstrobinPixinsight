//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorModule.cpp - Generated 2026-08-12
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

#define MODULE_VERSION_MAJOR     1
#define MODULE_VERSION_MINOR     2
#define MODULE_VERSION_REVISION  5
#define MODULE_VERSION_BUILD     0
#define MODULE_VERSION_LANGUAGE  eng

#define MODULE_RELEASE_YEAR      2026
#define MODULE_RELEASE_MONTH     8
#define MODULE_RELEASE_DAY       12

#include "AstroBinCSVGeneratorModule.h"
#include "AstroBinCSVGeneratorProcess.h"
#include "AstroBinCSVGeneratorInterface.h"

namespace pcl
{

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorModule::AstroBinCSVGeneratorModule()
{
}

// ----------------------------------------------------------------------------

const char* AstroBinCSVGeneratorModule::Version() const
{
   return PCL_MODULE_VERSION( MODULE_VERSION_MAJOR,
                              MODULE_VERSION_MINOR,
                              MODULE_VERSION_REVISION,
                              MODULE_VERSION_BUILD,
                              MODULE_VERSION_LANGUAGE );
}

// ----------------------------------------------------------------------------

IsoString AstroBinCSVGeneratorModule::Name() const
{
   return "AstroBinCSVGenerator";
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorModule::Description() const
{
   return "PixInsight AstroBin CSV Generator Process Module";
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorModule::Company() const
{
   return "Jamie Robinson";
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorModule::Author() const
{
   return "Jamie Robinson";
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorModule::Copyright() const
{
   return "Copyright (c) 2026 Jamie Robinson";
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorModule::TradeMarks() const
{
   return "PixInsight";
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorModule::OriginalFileName() const
{
#ifdef __PCL_FREEBSD
   return "AstroBinCSVGenerator-pxm.so";
#endif
#ifdef __PCL_LINUX
   return "AstroBinCSVGenerator-pxm.so";
#endif
#ifdef __PCL_MACOSX
   return "AstroBinCSVGenerator-pxm.dylib";
#endif
#ifdef __PCL_WINDOWS
   return "AstroBinCSVGenerator-pxm.dll";
#endif
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorModule::GetReleaseDate( int& year, int& month, int& day ) const
{
   year  = MODULE_RELEASE_YEAR;
   month = MODULE_RELEASE_MONTH;
   day   = MODULE_RELEASE_DAY;
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------

PCL_MODULE_EXPORT int InstallPixInsightModule( int mode )
{
   new pcl::AstroBinCSVGeneratorModule;

   if ( mode == pcl::InstallMode::FullInstall )
   {
      new pcl::AstroBinCSVGeneratorProcess;
      new pcl::AstroBinCSVGeneratorInterface;
   }

   return 0;
}

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorModule.cpp

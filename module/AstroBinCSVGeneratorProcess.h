//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorProcess.h - Generated 2026-08-12
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

#ifndef __AstroBinCSVGeneratorProcess_h
#define __AstroBinCSVGeneratorProcess_h

#include <pcl/MetaProcess.h>

namespace pcl
{

// ----------------------------------------------------------------------------

class AstroBinCSVGeneratorProcess : public MetaProcess
{
public:

   AstroBinCSVGeneratorProcess();

   IsoString Id() const override;
   IsoString Category() const override;
   uint32 Version() const override;
   String IconImageSVGFile() const override;
   ProcessInterface* DefaultInterface() const override;
   ProcessImplementation* Create() const override;
   ProcessImplementation* Clone( const ProcessImplementation& ) const override;
   bool IsAssignable() const override;
};

// ----------------------------------------------------------------------------

PCL_BEGIN_LOCAL
extern AstroBinCSVGeneratorProcess* TheAstroBinCSVGeneratorProcess;
PCL_END_LOCAL

// ----------------------------------------------------------------------------

} // pcl

#endif   // __AstroBinCSVGeneratorProcess_h

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorProcess.h

//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorParameters.h - Generated 2026-08-12
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

#ifndef __AstroBinCSVGeneratorParameters_h
#define __AstroBinCSVGeneratorParameters_h

#include <pcl/MetaParameter.h>

namespace pcl
{

PCL_BEGIN_LOCAL

// ----------------------------------------------------------------------------
// Input / Output parameters
// ----------------------------------------------------------------------------

class InputDirectoryParameter : public MetaString
{
public:

   InputDirectoryParameter( MetaProcess* );

   IsoString Id() const override;
   String Description() const override;
};

extern InputDirectoryParameter* TheInputDirectoryParameter;

// ----------------------------------------------------------------------------

class OutputDirectoryParameter : public MetaString
{
public:

   OutputDirectoryParameter( MetaProcess* );

   IsoString Id() const override;
   String Description() const override;
};

extern OutputDirectoryParameter* TheOutputDirectoryParameter;

// ----------------------------------------------------------------------------

class OutputFileNameParameter : public MetaString
{
public:

   OutputFileNameParameter( MetaProcess* );

   IsoString Id() const override;
   String DefaultValue() const override;
   String Description() const override;
};

extern OutputFileNameParameter* TheOutputFileNameParameter;

// ----------------------------------------------------------------------------

class RecursiveParameter : public MetaBoolean
{
public:

   RecursiveParameter( MetaProcess* );

   IsoString Id() const override;
   bool DefaultValue() const override;
   String Description() const override;
};

extern RecursiveParameter* TheRecursiveParameter;

// ----------------------------------------------------------------------------

class SessionGapHoursParameter : public MetaDouble
{
public:

   SessionGapHoursParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern SessionGapHoursParameter* TheSessionGapHoursParameter;

// ----------------------------------------------------------------------------

class OverrideFilePathParameter : public MetaString
{
public:

   OverrideFilePathParameter( MetaProcess* );

   IsoString Id() const override;
   String Description() const override;
};

extern OverrideFilePathParameter* TheOverrideFilePathParameter;

// ----------------------------------------------------------------------------
// Site and equipment parameters
// ----------------------------------------------------------------------------

class SiteNameParameter : public MetaString
{
public:

   SiteNameParameter( MetaProcess* );

   IsoString Id() const override;
   String DefaultValue() const override;
   String Description() const override;
};

extern SiteNameParameter* TheSiteNameParameter;

// ----------------------------------------------------------------------------

class SiteLatitudeParameter : public MetaDouble
{
public:

   SiteLatitudeParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double MaximumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern SiteLatitudeParameter* TheSiteLatitudeParameter;

// ----------------------------------------------------------------------------

class SiteLongitudeParameter : public MetaDouble
{
public:

   SiteLongitudeParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double MaximumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern SiteLongitudeParameter* TheSiteLongitudeParameter;

// ----------------------------------------------------------------------------

class SiteElevationParameter : public MetaDouble
{
public:

   SiteElevationParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern SiteElevationParameter* TheSiteElevationParameter;

// ----------------------------------------------------------------------------

class BortleParameter : public MetaInt32
{
public:

   BortleParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double MaximumValue() const override;
   double DefaultValue() const override;
   String Description() const override;
};

extern BortleParameter* TheBortleParameter;

// ----------------------------------------------------------------------------

class SQMParameter : public MetaDouble
{
public:

   SQMParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern SQMParameter* TheSQMParameter;

// ----------------------------------------------------------------------------

class FocalLengthParameter : public MetaDouble
{
public:

   FocalLengthParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern FocalLengthParameter* TheFocalLengthParameter;

// ----------------------------------------------------------------------------

class PixelSizeParameter : public MetaDouble
{
public:

   PixelSizeParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern PixelSizeParameter* ThePixelSizeParameter;

// ----------------------------------------------------------------------------

class FocalRatioParameter : public MetaDouble
{
public:

   FocalRatioParameter( MetaProcess* );

   IsoString Id() const override;
   double MinimumValue() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern FocalRatioParameter* TheFocalRatioParameter;

// ----------------------------------------------------------------------------

class ShiftOvernightParameter : public MetaBoolean
{
public:

   ShiftOvernightParameter( MetaProcess* );

   IsoString Id() const override;
   bool DefaultValue() const override;
   String Description() const override;
};

extern ShiftOvernightParameter* TheShiftOvernightParameter;

// ----------------------------------------------------------------------------

class UseObservingDateParameter : public MetaBoolean
{
public:

   UseObservingDateParameter( MetaProcess* );

   IsoString Id() const override;
   bool DefaultValue() const override;
   String Description() const override;
};

extern UseObservingDateParameter* TheUseObservingDateParameter;

// ----------------------------------------------------------------------------

class DefaultGainParameter : public MetaInt32
{
public:

   DefaultGainParameter( MetaProcess* );

   IsoString Id() const override;
   double DefaultValue() const override;
   String Description() const override;
};

extern DefaultGainParameter* TheDefaultGainParameter;

// ----------------------------------------------------------------------------

class DefaultTemperatureParameter : public MetaDouble
{
public:

   DefaultTemperatureParameter( MetaProcess* );

   IsoString Id() const override;
   double DefaultValue() const override;
   int Precision() const override;
   String Description() const override;
};

extern DefaultTemperatureParameter* TheDefaultTemperatureParameter;

// ----------------------------------------------------------------------------

class KeywordOverridesParameter : public MetaString
{
public:

   KeywordOverridesParameter( MetaProcess* );

   IsoString Id() const override;
   String Description() const override;
};

extern KeywordOverridesParameter* TheKeywordOverridesParameter;

// ----------------------------------------------------------------------------

class DefaultFilterParameter : public MetaString
{
public:

   DefaultFilterParameter( MetaProcess* );

   IsoString Id() const override;
   String Description() const override;
};

extern DefaultFilterParameter* TheDefaultFilterParameter;

// ----------------------------------------------------------------------------

class UseDefaultFilterParameter : public MetaBoolean
{
public:

   UseDefaultFilterParameter( MetaProcess* );

   IsoString Id() const override;
   bool DefaultValue() const override;
   String Description() const override;
};

extern UseDefaultFilterParameter* TheUseDefaultFilterParameter;

// ----------------------------------------------------------------------------

class FilterMapParameter : public MetaString
{
public:

   FilterMapParameter( MetaProcess* );

   IsoString Id() const override;
   String DefaultValue() const override;
   String Description() const override;
};

extern FilterMapParameter* TheFilterMapParameter;

// ----------------------------------------------------------------------------

class FilterDatabasePathParameter : public MetaString
{
public:

   FilterDatabasePathParameter( MetaProcess* );

   IsoString Id() const override;
   String Description() const override;
};

extern FilterDatabasePathParameter* TheFilterDatabasePathParameter;

// ----------------------------------------------------------------------------

PCL_END_LOCAL

} // pcl

#endif   // __AstroBinCSVGeneratorParameters_h

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorParameters.h

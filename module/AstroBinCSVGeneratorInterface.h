//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.0.0
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorInterface.h - Generated 2026-08-12
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

#ifndef __AstroBinCSVGeneratorInterface_h
#define __AstroBinCSVGeneratorInterface_h

#include <pcl/CheckBox.h>
#include <pcl/Edit.h>
#include <pcl/Label.h>
#include <pcl/NumericControl.h>
#include <pcl/ProcessInterface.h>
#include <pcl/PushButton.h>
#include <pcl/SectionBar.h>
#include <pcl/Sizer.h>

#include "AstroBinCSVGeneratorInstance.h"

namespace pcl
{

// ----------------------------------------------------------------------------

class AstroBinCSVGeneratorInterface : public ProcessInterface
{
public:

   AstroBinCSVGeneratorInterface();
   virtual ~AstroBinCSVGeneratorInterface();

   IsoString Id() const override;
   MetaProcess* Process() const override;
   String IconImageSVGFile() const override;
   InterfaceFeatures Features() const override;
   void ResetInstance() override;
   bool Launch( const MetaProcess&, const ProcessImplementation*, bool& dynamic, unsigned& /*flags*/ ) override;
   ProcessImplementation* NewProcess() const override;
   bool ValidateProcess( const ProcessImplementation&, pcl::String& whyNot ) const override;
   bool RequiresInstanceValidation() const override;
   bool ImportProcess( const ProcessImplementation& ) override;

private:

   AstroBinCSVGeneratorInstance m_instance;

   struct GUIData
   {
      GUIData( AstroBinCSVGeneratorInterface& );

      VerticalSizer Global_Sizer;

         SectionBar    InputFiles_SectionBar;
         Control       InputFiles_Section;
         VerticalSizer InputFiles_Section_Sizer;
            HorizontalSizer InputDirectory_Sizer;
               Label            InputDirectory_Label;
               Edit             InputDirectory_Edit;
               PushButton       InputDirectory_Browse_PushButton;
            CheckBox          Recursive_CheckBox;
            HorizontalSizer OutputDirectory_Sizer;
               Label            OutputDirectory_Label;
               Edit             OutputDirectory_Edit;
               PushButton       OutputDirectory_Browse_PushButton;
            HorizontalSizer OutputFileName_Sizer;
               Label            OutputFileName_Label;
               Edit             OutputFileName_Edit;
            HorizontalSizer OverrideFile_Sizer;
               Label            OverrideFile_Label;
               Edit             OverrideFile_Edit;
               PushButton       OverrideFile_Browse_PushButton;

         SectionBar    Site_SectionBar;
         Control       Site_Section;
         VerticalSizer Site_Section_Sizer;
            HorizontalSizer SiteName_Sizer;
               Label            SiteName_Label;
               Edit             SiteName_Edit;
            HorizontalSizer SiteCoordinates_Sizer;
               NumericControl    SiteLatitude_NumericControl;
               NumericControl    SiteLongitude_NumericControl;
            HorizontalSizer SiteElevationSky_Sizer;
               NumericControl    SiteElevation_NumericControl;
               NumericControl    Bortle_NumericControl;
            HorizontalSizer SiteSky_Sizer;
               NumericControl    SQM_NumericControl;

         SectionBar    Equipment_SectionBar;
         Control       Equipment_Section;
         VerticalSizer Equipment_Section_Sizer;
            HorizontalSizer Optics_Sizer;
               NumericControl    FocalLength_NumericControl;
               NumericControl    PixelSize_NumericControl;
            HorizontalSizer OpticsMisc_Sizer;
               NumericControl    FocalRatio_NumericControl;
               NumericControl    DefaultGain_NumericControl;
            HorizontalSizer Camera_Sizer;
               NumericControl    DefaultTemperature_NumericControl;

         SectionBar    Sessions_SectionBar;
         Control       Sessions_Section;
         VerticalSizer Sessions_Section_Sizer;
            NumericControl SessionGapHours_NumericControl;
            CheckBox       ShiftOvernight_CheckBox;
            CheckBox       UseObservingDate_CheckBox;

         SectionBar    Filters_SectionBar;
         Control       Filters_Section;
         VerticalSizer Filters_Section_Sizer;
            HorizontalSizer FilterDatabase_Sizer;
               Label            FilterDatabase_Label;
               Edit             FilterDatabase_Edit;
               PushButton       FilterDatabase_Browse_PushButton;
               PushButton       DownloadFilters_PushButton;
            HorizontalSizer DefaultFilter_Sizer;
               Label            DefaultFilter_Label;
               Edit             DefaultFilter_Edit;
               CheckBox         UseDefaultFilter_CheckBox;
            Label           FilterMap_Label;
            Edit            FilterMap_Edit;

         SectionBar    Overrides_SectionBar;
         Control       Overrides_Section;
         VerticalSizer Overrides_Section_Sizer;
            Label   KeywordOverrides_Label;
            Edit    KeywordOverrides_Edit;
   };

   GUIData* GUI = nullptr;

   void UpdateControls();
   void LoadSettings();
   void SaveSettings() const;

   void e_EditCompleted( Edit& sender );
   void e_CheckBox_Click( Button& sender, bool checked );
   void e_ValueUpdated( NumericEdit& sender, double value );
   void e_Browse_Click( Button& sender, bool checked );
   void e_Download_Click( Button& sender, bool checked );

   friend struct GUIData;
};

// ----------------------------------------------------------------------------

PCL_BEGIN_LOCAL
extern AstroBinCSVGeneratorInterface* TheAstroBinCSVGeneratorInterface;
PCL_END_LOCAL

// ----------------------------------------------------------------------------

} // pcl

#endif   // __AstroBinCSVGeneratorInterface_h

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorInterface.h

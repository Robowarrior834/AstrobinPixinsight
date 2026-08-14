//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
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
#include <pcl/TreeBox.h>

#include "AstroBinCSVGeneratorEngine.h"
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
            TreeBox           FileList_TreeBox;
            HorizontalSizer FileListButtons_Sizer;
               PushButton AddFiles_PushButton;
               PushButton AddDirectory_PushButton;
               PushButton RemoveSelected_PushButton;
               PushButton ClearAll_PushButton;
               PushButton SetFilter_PushButton;
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
            HorizontalSizer Settings_Sizer;
               PushButton Settings_PushButton;
   };

   GUIData* GUI = nullptr;

   // Loaded files shown in the file list. Each entry stores the extracted
   // frame metadata plus any per-file filter override set by the user.
   std::vector<AstroBinCSVGeneratorEngine::FrameData> m_files;

   void UpdateControls();
   void LoadSettings();
   void SaveSettings() const;

   void RebuildFileTree();
   void SyncFileListToInstance();
   void SyncFileListFromInstance();
   void ProcessFiles( const StringList& paths );
   void SetFilterForSelected();
   void OpenSettings();
   AstroBinCSVGeneratorEngine CreateEngine() const;

   void e_EditCompleted( Edit& sender );
   void e_Browse_Click( Button& sender, bool checked );
   void e_Settings_Click( Button& sender, bool checked );
   void e_AddFiles_Click( Button& sender, bool checked );
   void e_AddDirectory_Click( Button& sender, bool checked );
   void e_RemoveSelected_Click( Button& sender, bool checked );
   void e_ClearAll_Click( Button& sender, bool checked );
   void e_SetFilter_Click( Button& sender, bool checked );

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

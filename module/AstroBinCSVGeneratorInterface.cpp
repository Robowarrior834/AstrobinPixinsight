//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.0.0
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorInterface.cpp - Generated 2026-08-12
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

#include "AstroBinCSVGeneratorInterface.h"
#include "AstroBinCSVGeneratorParameters.h"
#include "AstroBinCSVGeneratorProcess.h"

#include <pcl/Console.h>
#include <pcl/FileDialog.h>
#include <pcl/Settings.h>

namespace pcl
{

// ----------------------------------------------------------------------------
// Settings namespace prefix, shared with the AstroBin CSV Generator script
// (v1.2.5) so existing user settings carry over to this module.
// ----------------------------------------------------------------------------

#define SETTINGS_NS "AstroBin CSV Generator.1.2.5_"

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInterface* TheAstroBinCSVGeneratorInterface = nullptr;

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInterface::AstroBinCSVGeneratorInterface()
   : m_instance( TheAstroBinCSVGeneratorProcess )
{
   TheAstroBinCSVGeneratorInterface = this;
}

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInterface::~AstroBinCSVGeneratorInterface()
{
   if ( GUI != nullptr )
      delete GUI, GUI = nullptr;
}

// ----------------------------------------------------------------------------

IsoString AstroBinCSVGeneratorInterface::Id() const
{
   return "AstroBinCSVGenerator";
}

// ----------------------------------------------------------------------------

MetaProcess* AstroBinCSVGeneratorInterface::Process() const
{
   return TheAstroBinCSVGeneratorProcess;
}

// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorInterface::IconImageSVGFile() const
{
   return "@module_icons_dir/AstroBinCSVGenerator.svg";
}

// ----------------------------------------------------------------------------

InterfaceFeatures AstroBinCSVGeneratorInterface::Features() const
{
   return InterfaceFeature::DefaultGlobal;
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::ResetInstance()
{
   AstroBinCSVGeneratorInstance defaultInstance( TheAstroBinCSVGeneratorProcess );
   ImportProcess( defaultInstance );
   LoadSettings();
   UpdateControls();
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInterface::Launch( const MetaProcess& P, const ProcessImplementation*, bool& dynamic, unsigned& /*flags*/ )
{
   if ( GUI == nullptr )
   {
      GUI = new GUIData( *this );
      SetWindowTitle( "AstroBin CSV Generator" );
      LoadSettings();
      UpdateControls();
   }

   dynamic = false;
   return &P == TheAstroBinCSVGeneratorProcess;
}

// ----------------------------------------------------------------------------

ProcessImplementation* AstroBinCSVGeneratorInterface::NewProcess() const
{
   return new AstroBinCSVGeneratorInstance( m_instance );
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInterface::ValidateProcess( const ProcessImplementation& p, String& whyNot ) const
{
   if ( dynamic_cast<const AstroBinCSVGeneratorInstance*>( &p ) != nullptr )
      return true;
   whyNot = "Not an AstroBinCSVGenerator instance.";
   return false;
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInterface::RequiresInstanceValidation() const
{
   return true;
}

// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorInterface::ImportProcess( const ProcessImplementation& p )
{
   m_instance.Assign( p );
   UpdateControls();
   return true;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::UpdateControls()
{
   GUI->InputDirectory_Edit.SetText( m_instance.p_inputDirectory );
   GUI->Recursive_CheckBox.SetChecked( m_instance.p_recursive );
   GUI->OutputDirectory_Edit.SetText( m_instance.p_outputDirectory );
   GUI->OutputFileName_Edit.SetText( m_instance.p_outputFileName );
   GUI->OverrideFile_Edit.SetText( m_instance.p_overrideFilePath );

   GUI->SiteName_Edit.SetText( m_instance.p_siteName );
   GUI->SiteLatitude_NumericControl.SetValue( m_instance.p_siteLatitude );
   GUI->SiteLongitude_NumericControl.SetValue( m_instance.p_siteLongitude );
   GUI->SiteElevation_NumericControl.SetValue( m_instance.p_siteElevation );
   GUI->Bortle_NumericControl.SetValue( m_instance.p_bortle );
   GUI->SQM_NumericControl.SetValue( m_instance.p_sqm );

   GUI->FocalLength_NumericControl.SetValue( m_instance.p_focalLength );
   GUI->PixelSize_NumericControl.SetValue( m_instance.p_pixelSize );
   GUI->FocalRatio_NumericControl.SetValue( m_instance.p_focalRatio );
   GUI->DefaultGain_NumericControl.SetValue( m_instance.p_defaultGain );
   GUI->DefaultTemperature_NumericControl.SetValue( m_instance.p_defaultTemperature );

   GUI->SessionGapHours_NumericControl.SetValue( m_instance.p_sessionGapHours );
   GUI->ShiftOvernight_CheckBox.SetChecked( m_instance.p_shiftOvernight );
   GUI->UseObservingDate_CheckBox.SetChecked( m_instance.p_useObservingDate );

   GUI->FilterDatabase_Edit.SetText( m_instance.p_filterDatabasePath );
   GUI->DefaultFilter_Edit.SetText( m_instance.p_defaultFilter );
   GUI->UseDefaultFilter_CheckBox.SetChecked( m_instance.p_useDefaultFilter );
   GUI->FilterMap_Edit.SetText( m_instance.p_filterMap );

   GUI->KeywordOverrides_Edit.SetText( m_instance.p_keywordOverrides );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::LoadSettings()
{
   String s;
   bool b;
   int i;
   double d;

   if ( Settings::Read( SETTINGS_NS "siteName", s ) )
      m_instance.p_siteName = s;
   if ( Settings::Read( SETTINGS_NS "siteLat", d ) )
      m_instance.p_siteLatitude = d;
   if ( Settings::Read( SETTINGS_NS "siteLon", d ) )
      m_instance.p_siteLongitude = d;
   if ( Settings::Read( SETTINGS_NS "siteElev", d ) )
      m_instance.p_siteElevation = d;
   if ( Settings::Read( SETTINGS_NS "bortle", i ) )
      m_instance.p_bortle = i;
   if ( Settings::Read( SETTINGS_NS "sqm", d ) )
      m_instance.p_sqm = d;
   if ( Settings::Read( SETTINGS_NS "focalLength", d ) )
      m_instance.p_focalLength = d;
   if ( Settings::Read( SETTINGS_NS "pixelSize", d ) )
      m_instance.p_pixelSize = d;
   if ( Settings::Read( SETTINGS_NS "focalRatio", d ) )
      m_instance.p_focalRatio = d;
   if ( Settings::Read( SETTINGS_NS "shiftOvernight", b ) )
      m_instance.p_shiftOvernight = b;
   if ( Settings::Read( SETTINGS_NS "useObsDate", b ) )
      m_instance.p_useObservingDate = b;
   if ( Settings::Read( SETTINGS_NS "defaultGain", i ) )
      m_instance.p_defaultGain = i;
   if ( Settings::Read( SETTINGS_NS "defaultTemp", d ) )
      m_instance.p_defaultTemperature = d;
   if ( Settings::Read( SETTINGS_NS "keywordOverrides", s ) )
      if ( !s.IsEmpty() )
         m_instance.p_keywordOverrides = s;
   if ( Settings::Read( SETTINGS_NS "defaultFilter", s ) )
      m_instance.p_defaultFilter = s;
   if ( Settings::Read( SETTINGS_NS "useDefaultFilter", b ) )
      m_instance.p_useDefaultFilter = b;
   if ( Settings::Read( SETTINGS_NS "filterMap", s ) )
      if ( !s.IsEmpty() )
         m_instance.p_filterMap = s;
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::SaveSettings() const
{
   Settings::Write( SETTINGS_NS "siteName", m_instance.p_siteName );
   Settings::Write( SETTINGS_NS "siteLat", m_instance.p_siteLatitude );
   Settings::Write( SETTINGS_NS "siteLon", m_instance.p_siteLongitude );
   Settings::Write( SETTINGS_NS "siteElev", m_instance.p_siteElevation );
   Settings::Write( SETTINGS_NS "bortle", int( m_instance.p_bortle ) );
   Settings::Write( SETTINGS_NS "sqm", m_instance.p_sqm );
   Settings::Write( SETTINGS_NS "focalLength", m_instance.p_focalLength );
   Settings::Write( SETTINGS_NS "pixelSize", m_instance.p_pixelSize );
   Settings::Write( SETTINGS_NS "focalRatio", m_instance.p_focalRatio );
   Settings::Write( SETTINGS_NS "shiftOvernight", bool( m_instance.p_shiftOvernight ) );
   Settings::Write( SETTINGS_NS "useObsDate", bool( m_instance.p_useObservingDate ) );
   Settings::Write( SETTINGS_NS "defaultGain", int( m_instance.p_defaultGain ) );
   Settings::Write( SETTINGS_NS "defaultTemp", m_instance.p_defaultTemperature );
   Settings::Write( SETTINGS_NS "keywordOverrides", m_instance.p_keywordOverrides );
   Settings::Write( SETTINGS_NS "defaultFilter", m_instance.p_defaultFilter );
   Settings::Write( SETTINGS_NS "useDefaultFilter", bool( m_instance.p_useDefaultFilter ) );
   Settings::Write( SETTINGS_NS "filterMap", m_instance.p_filterMap );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_EditCompleted( Edit& sender )
{
   String text = sender.Text().Trimmed();

   if ( sender == GUI->InputDirectory_Edit )
      m_instance.p_inputDirectory = text;
   else if ( sender == GUI->OutputDirectory_Edit )
      m_instance.p_outputDirectory = text;
   else if ( sender == GUI->OutputFileName_Edit )
      m_instance.p_outputFileName = text;
   else if ( sender == GUI->OverrideFile_Edit )
      m_instance.p_overrideFilePath = text;
   else if ( sender == GUI->SiteName_Edit )
   {
      m_instance.p_siteName = text;
      SaveSettings();
   }
   else if ( sender == GUI->FilterDatabase_Edit )
      m_instance.p_filterDatabasePath = text;
   else if ( sender == GUI->DefaultFilter_Edit )
   {
      m_instance.p_defaultFilter = text;
      SaveSettings();
   }
   else if ( sender == GUI->FilterMap_Edit )
   {
      m_instance.p_filterMap = text;
      SaveSettings();
   }
   else if ( sender == GUI->KeywordOverrides_Edit )
   {
      m_instance.p_keywordOverrides = text;
      SaveSettings();
   }

   sender.SetText( text );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_CheckBox_Click( Button& sender, bool /*checked*/ )
{
   if ( sender == GUI->Recursive_CheckBox )
      m_instance.p_recursive = GUI->Recursive_CheckBox.IsChecked();
   else if ( sender == GUI->ShiftOvernight_CheckBox )
   {
      m_instance.p_shiftOvernight = GUI->ShiftOvernight_CheckBox.IsChecked();
      SaveSettings();
   }
   else if ( sender == GUI->UseObservingDate_CheckBox )
   {
      m_instance.p_useObservingDate = GUI->UseObservingDate_CheckBox.IsChecked();
      SaveSettings();
   }
   else if ( sender == GUI->UseDefaultFilter_CheckBox )
   {
      m_instance.p_useDefaultFilter = GUI->UseDefaultFilter_CheckBox.IsChecked();
      SaveSettings();
   }
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_ValueUpdated( NumericEdit& sender, double value )
{
   if ( sender == GUI->SiteLatitude_NumericControl )
      m_instance.p_siteLatitude = value;
   else if ( sender == GUI->SiteLongitude_NumericControl )
      m_instance.p_siteLongitude = value;
   else if ( sender == GUI->SiteElevation_NumericControl )
      m_instance.p_siteElevation = value;
   else if ( sender == GUI->Bortle_NumericControl )
      m_instance.p_bortle = int( value );
   else if ( sender == GUI->SQM_NumericControl )
      m_instance.p_sqm = value;
   else if ( sender == GUI->FocalLength_NumericControl )
      m_instance.p_focalLength = value;
   else if ( sender == GUI->PixelSize_NumericControl )
      m_instance.p_pixelSize = value;
   else if ( sender == GUI->FocalRatio_NumericControl )
      m_instance.p_focalRatio = value;
   else if ( sender == GUI->DefaultGain_NumericControl )
      m_instance.p_defaultGain = int( value );
   else if ( sender == GUI->DefaultTemperature_NumericControl )
      m_instance.p_defaultTemperature = value;
   else if ( sender == GUI->SessionGapHours_NumericControl )
      m_instance.p_sessionGapHours = value;

   SaveSettings();
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_Browse_Click( Button& sender, bool /*checked*/ )
{
   if ( sender == GUI->InputDirectory_Browse_PushButton )
   {
      GetDirectoryDialog dlg;
      dlg.SetCaption( "Select Directory with Light Frames" );
      if ( dlg.Execute() )
      {
         m_instance.p_inputDirectory = dlg.Directory();
         GUI->InputDirectory_Edit.SetText( m_instance.p_inputDirectory );
      }
   }
   else if ( sender == GUI->OutputDirectory_Browse_PushButton )
   {
      GetDirectoryDialog dlg;
      dlg.SetCaption( "Select Output Directory" );
      if ( dlg.Execute() )
      {
         m_instance.p_outputDirectory = dlg.Directory();
         GUI->OutputDirectory_Edit.SetText( m_instance.p_outputDirectory );
      }
   }
   else if ( sender == GUI->OverrideFile_Browse_PushButton )
   {
      OpenFileDialog dlg;
      dlg.SetCaption( "Select Keyword Overrides CSV File" );
      dlg.SetFilter( FileFilter( "CSV files (*.csv)", "csv" ) );
      if ( dlg.Execute() )
      {
         m_instance.p_overrideFilePath = dlg.FileName();
         GUI->OverrideFile_Edit.SetText( m_instance.p_overrideFilePath );
      }
   }
   else if ( sender == GUI->FilterDatabase_Browse_PushButton )
   {
      OpenFileDialog dlg;
      dlg.SetCaption( "Select Filter Database JSON File" );
      dlg.SetFilter( FileFilter( "JSON files (*.json)", "json" ) );
      if ( dlg.Execute() )
      {
         m_instance.p_filterDatabasePath = dlg.FileName();
         GUI->FilterDatabase_Edit.SetText( m_instance.p_filterDatabasePath );
      }
   }
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_Download_Click( Button& /*sender*/, bool /*checked*/ )
{
   Console c;
   c.WriteLn( "<end><cbr>AstroBin CSV Generator: filter database download not "
              "implemented yet." );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInterface::GUIData::GUIData( AstroBinCSVGeneratorInterface& w )
{
   pcl::Font fnt = w.Font();
   int pathLabelWidth = fnt.Width( String( "Filter database:" ) + 'M' );
   int labelWidth = fnt.Width( String( "Output file name:" ) + 'M' );
   int editWidth = 30 * fnt.Width( 'M' );
   int editWidth2 = 20 * fnt.Width( 'M' );

   //
   // Input Files section
   //

   InputDirectory_Label.SetText( "Input directory:" );
   InputDirectory_Label.SetMinWidth( labelWidth );
   InputDirectory_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   InputDirectory_Edit.SetMinWidth( editWidth );
   InputDirectory_Edit.SetToolTip( "<p>Directory containing the FITS/XISF light frames to process.</p>" );
   InputDirectory_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   InputDirectory_Browse_PushButton.SetText( "Browse" );
   InputDirectory_Browse_PushButton.SetToolTip( "<p>Select a directory with light frames.</p>" );
   InputDirectory_Browse_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_Browse_Click, w );

   InputDirectory_Sizer.SetSpacing( 4 );
   InputDirectory_Sizer.Add( InputDirectory_Label );
   InputDirectory_Sizer.Add( InputDirectory_Edit, 100 );
   InputDirectory_Sizer.Add( InputDirectory_Browse_PushButton );

   Recursive_CheckBox.SetText( "Recursive search" );
   Recursive_CheckBox.SetToolTip( "<p>Recursively search subdirectories for FITS/XISF files.</p>" );
   Recursive_CheckBox.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_CheckBox_Click, w );

   OutputDirectory_Label.SetText( "Output directory:" );
   OutputDirectory_Label.SetMinWidth( labelWidth );
   OutputDirectory_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   OutputDirectory_Edit.SetMinWidth( editWidth );
   OutputDirectory_Edit.SetToolTip( "<p>Directory where the acquisition CSV file will be written. Leave empty to use the input directory.</p>" );
   OutputDirectory_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   OutputDirectory_Browse_PushButton.SetText( "Browse" );
   OutputDirectory_Browse_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_Browse_Click, w );

   OutputDirectory_Sizer.SetSpacing( 4 );
   OutputDirectory_Sizer.Add( OutputDirectory_Label );
   OutputDirectory_Sizer.Add( OutputDirectory_Edit, 100 );
   OutputDirectory_Sizer.Add( OutputDirectory_Browse_PushButton );

   OutputFileName_Label.SetText( "Output file name:" );
   OutputFileName_Label.SetMinWidth( labelWidth );
   OutputFileName_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   OutputFileName_Edit.SetMinWidth( editWidth2 );
   OutputFileName_Edit.SetToolTip( "<p>File name of the generated AstroBin acquisition CSV file.</p>" );
   OutputFileName_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   OutputFileName_Sizer.SetSpacing( 4 );
   OutputFileName_Sizer.Add( OutputFileName_Label );
   OutputFileName_Sizer.Add( OutputFileName_Edit );
   OutputFileName_Sizer.AddStretch();

   OverrideFile_Label.SetText( "Override file:" );
   OverrideFile_Label.SetMinWidth( labelWidth );
   OverrideFile_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   OverrideFile_Edit.SetMinWidth( editWidth );
   OverrideFile_Edit.SetToolTip( "<p>Optional CSV file with per-file keyword overrides.</p>" );
   OverrideFile_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   OverrideFile_Browse_PushButton.SetText( "Browse" );
   OverrideFile_Browse_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_Browse_Click, w );

   OverrideFile_Sizer.SetSpacing( 4 );
   OverrideFile_Sizer.Add( OverrideFile_Label );
   OverrideFile_Sizer.Add( OverrideFile_Edit, 100 );
   OverrideFile_Sizer.Add( OverrideFile_Browse_PushButton );

   InputFiles_Section_Sizer.SetMargin( 6 );
   InputFiles_Section_Sizer.SetSpacing( 4 );
   InputFiles_Section_Sizer.Add( InputDirectory_Sizer );
   InputFiles_Section_Sizer.Add( Recursive_CheckBox );
   InputFiles_Section_Sizer.Add( OutputDirectory_Sizer );
   InputFiles_Section_Sizer.Add( OutputFileName_Sizer );
   InputFiles_Section_Sizer.Add( OverrideFile_Sizer );

   InputFiles_Section.SetSizer( InputFiles_Section_Sizer );
   InputFiles_SectionBar.SetTitle( "Input Files" );
   InputFiles_SectionBar.SetSection( InputFiles_Section );

   //
   // Site section
   //

   int siteLabelWidth = fnt.Width( String( "Elevation (m):" ) + 'M' );

   SiteName_Label.SetText( "Site name:" );
   SiteName_Label.SetMinWidth( siteLabelWidth );
   SiteName_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   SiteName_Edit.SetMinWidth( editWidth2 );
   SiteName_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   SiteName_Sizer.SetSpacing( 4 );
   SiteName_Sizer.Add( SiteName_Label );
   SiteName_Sizer.Add( SiteName_Edit );
   SiteName_Sizer.AddStretch();

   SiteLatitude_NumericControl.label.SetText( "Latitude:" );
   SiteLatitude_NumericControl.label.SetMinWidth( siteLabelWidth );
   SiteLatitude_NumericControl.SetReal();
   SiteLatitude_NumericControl.SetRange( TheSiteLatitudeParameter->MinimumValue(), TheSiteLatitudeParameter->MaximumValue() );
   SiteLatitude_NumericControl.SetPrecision( TheSiteLatitudeParameter->Precision() );
   SiteLatitude_NumericControl.SetToolTip( "<p>Latitude of the observing site, in degrees (N positive).</p>" );
   SiteLatitude_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   SiteLongitude_NumericControl.label.SetText( "Longitude:" );
   SiteLongitude_NumericControl.label.SetMinWidth( siteLabelWidth );
   SiteLongitude_NumericControl.SetReal();
   SiteLongitude_NumericControl.SetRange( TheSiteLongitudeParameter->MinimumValue(), TheSiteLongitudeParameter->MaximumValue() );
   SiteLongitude_NumericControl.SetPrecision( TheSiteLongitudeParameter->Precision() );
   SiteLongitude_NumericControl.SetToolTip( "<p>Longitude of the observing site, in degrees (E positive).</p>" );
   SiteLongitude_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   SiteElevation_NumericControl.label.SetText( "Elevation (m):" );
   SiteElevation_NumericControl.label.SetMinWidth( siteLabelWidth );
   SiteElevation_NumericControl.SetReal();
   SiteElevation_NumericControl.SetRange( TheSiteElevationParameter->MinimumValue(), TheSiteElevationParameter->MaximumValue() );
   SiteElevation_NumericControl.SetPrecision( TheSiteElevationParameter->Precision() );
   SiteElevation_NumericControl.SetToolTip( "<p>Elevation of the observing site above sea level, in meters.</p>" );
   SiteElevation_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   SiteCoordinates_Sizer.SetSpacing( 8 );
   SiteCoordinates_Sizer.Add( SiteLatitude_NumericControl, 100 );
   SiteCoordinates_Sizer.Add( SiteLongitude_NumericControl, 100 );

   SiteElevationSky_Sizer.SetSpacing( 8 );
   SiteElevationSky_Sizer.Add( SiteElevation_NumericControl, 100 );
   SiteElevationSky_Sizer.Add( Bortle_NumericControl, 100 );

   Bortle_NumericControl.label.SetText( "Bortle:" );
   Bortle_NumericControl.label.SetMinWidth( siteLabelWidth );
   Bortle_NumericControl.SetInteger();
   Bortle_NumericControl.SetRange( TheBortleParameter->MinimumValue(), TheBortleParameter->MaximumValue() );
   Bortle_NumericControl.SetToolTip( "<p>Bortle dark-sky class, from 1 (excellent) to 9 (inner city).</p>" );
   Bortle_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   SQM_NumericControl.label.SetText( "SQM:" );
   SQM_NumericControl.label.SetMinWidth( siteLabelWidth );
   SQM_NumericControl.SetReal();
   SQM_NumericControl.SetRange( TheSQMParameter->MinimumValue(), TheSQMParameter->MaximumValue() );
   SQM_NumericControl.SetPrecision( TheSQMParameter->Precision() );
   SQM_NumericControl.SetToolTip( "<p>Sky Quality Meter reading, in magnitudes per square arcsecond.</p>" );
   SQM_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   SiteSky_Sizer.SetSpacing( 8 );
   SiteSky_Sizer.Add( SQM_NumericControl, 100 );

   Site_Section_Sizer.SetMargin( 6 );
   Site_Section_Sizer.SetSpacing( 4 );
   Site_Section_Sizer.Add( SiteName_Sizer );
   Site_Section_Sizer.Add( SiteCoordinates_Sizer );
   Site_Section_Sizer.Add( SiteElevationSky_Sizer );
   Site_Section_Sizer.Add( SiteSky_Sizer );

   Site_Section.SetSizer( Site_Section_Sizer );
   Site_SectionBar.SetTitle( "Site" );
   Site_SectionBar.SetSection( Site_Section );

   //
   // Equipment section
   //

   int opticsLabelWidth = fnt.Width( String( "Focal length (mm):" ) + 'M' );

   FocalLength_NumericControl.label.SetText( "Focal length (mm):" );
   FocalLength_NumericControl.label.SetMinWidth( opticsLabelWidth );
   FocalLength_NumericControl.SetReal();
   FocalLength_NumericControl.SetRange( TheFocalLengthParameter->MinimumValue(), TheFocalLengthParameter->MaximumValue() );
   FocalLength_NumericControl.SetPrecision( TheFocalLengthParameter->Precision() );
   FocalLength_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   PixelSize_NumericControl.label.SetText( "Pixel size (um):" );
   PixelSize_NumericControl.label.SetMinWidth( opticsLabelWidth );
   PixelSize_NumericControl.SetReal();
   PixelSize_NumericControl.SetRange( ThePixelSizeParameter->MinimumValue(), ThePixelSizeParameter->MaximumValue() );
   PixelSize_NumericControl.SetPrecision( ThePixelSizeParameter->Precision() );
   PixelSize_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   FocalRatio_NumericControl.label.SetText( "Focal ratio:" );
   FocalRatio_NumericControl.label.SetMinWidth( opticsLabelWidth );
   FocalRatio_NumericControl.SetReal();
   FocalRatio_NumericControl.SetRange( TheFocalRatioParameter->MinimumValue(), TheFocalRatioParameter->MaximumValue() );
   FocalRatio_NumericControl.SetPrecision( TheFocalRatioParameter->Precision() );
   FocalRatio_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   Optics_Sizer.SetSpacing( 8 );
   Optics_Sizer.Add( FocalLength_NumericControl, 100 );
   Optics_Sizer.Add( PixelSize_NumericControl, 100 );

   OpticsMisc_Sizer.SetSpacing( 8 );
   OpticsMisc_Sizer.Add( FocalRatio_NumericControl, 100 );
   OpticsMisc_Sizer.Add( DefaultGain_NumericControl, 100 );

   DefaultTemperature_NumericControl.label.SetText( "Default temp (C):" );
   DefaultTemperature_NumericControl.label.SetMinWidth( opticsLabelWidth );
   DefaultTemperature_NumericControl.SetReal();
   DefaultTemperature_NumericControl.SetPrecision( TheDefaultTemperatureParameter->Precision() );
   DefaultTemperature_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   Camera_Sizer.SetSpacing( 8 );
   Camera_Sizer.Add( DefaultTemperature_NumericControl, 100 );

   Equipment_Section_Sizer.SetMargin( 6 );
   Equipment_Section_Sizer.SetSpacing( 4 );
   Equipment_Section_Sizer.Add( Optics_Sizer );
   Equipment_Section_Sizer.Add( OpticsMisc_Sizer );
   Equipment_Section_Sizer.Add( Camera_Sizer );

   Equipment_Section.SetSizer( Equipment_Section_Sizer );
   Equipment_SectionBar.SetTitle( "Equipment" );
   Equipment_SectionBar.SetSection( Equipment_Section );

   //
   // Sessions section
   //

   SessionGapHours_NumericControl.label.SetText( "Session gap (hours):" );
   SessionGapHours_NumericControl.SetReal();
   SessionGapHours_NumericControl.SetRange( TheSessionGapHoursParameter->MinimumValue(), TheSessionGapHoursParameter->MaximumValue() );
   SessionGapHours_NumericControl.SetPrecision( TheSessionGapHoursParameter->Precision() );
   SessionGapHours_NumericControl.SetToolTip( "<p>Minimum time gap (hours) between consecutive frames that defines a new imaging session.</p>" );
   SessionGapHours_NumericControl.OnValueUpdated( (NumericEdit::value_event_handler)&AstroBinCSVGeneratorInterface::e_ValueUpdated, w );

   ShiftOvernight_CheckBox.SetText( "Shift overnight sessions to the previous day" );
   ShiftOvernight_CheckBox.SetToolTip( "<p>Assign frames acquired after midnight to the previous calendar day.</p>" );
   ShiftOvernight_CheckBox.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_CheckBox_Click, w );

   UseObservingDate_CheckBox.SetText( "Use observing date (DATE-OBS) for sorting" );
   UseObservingDate_CheckBox.SetToolTip( "<p>Use DATE-OBS instead of the file modification date when sorting frames.</p>" );
   UseObservingDate_CheckBox.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_CheckBox_Click, w );

   Sessions_Section_Sizer.SetMargin( 6 );
   Sessions_Section_Sizer.SetSpacing( 4 );
   Sessions_Section_Sizer.Add( SessionGapHours_NumericControl );
   Sessions_Section_Sizer.Add( ShiftOvernight_CheckBox );
   Sessions_Section_Sizer.Add( UseObservingDate_CheckBox );

   Sessions_Section.SetSizer( Sessions_Section_Sizer );
   Sessions_SectionBar.SetTitle( "Sessions" );
   Sessions_SectionBar.SetSection( Sessions_Section );

   //
   // Filters section
   //

   FilterDatabase_Label.SetText( "Filter database:" );
   FilterDatabase_Label.SetMinWidth( pathLabelWidth );
   FilterDatabase_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   FilterDatabase_Edit.SetMinWidth( editWidth );
   FilterDatabase_Edit.SetToolTip( "<p>Path to the AstroBin filter database JSON cache file. Leave empty for the default location.</p>" );
   FilterDatabase_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   FilterDatabase_Browse_PushButton.SetText( "Browse" );
   FilterDatabase_Browse_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_Browse_Click, w );

   DownloadFilters_PushButton.SetText( "Download" );
   DownloadFilters_PushButton.SetToolTip( "<p>Download the AstroBin filter database from the AstroBin REST API.</p>" );
   DownloadFilters_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_Download_Click, w );

   FilterDatabase_Sizer.SetSpacing( 4 );
   FilterDatabase_Sizer.Add( FilterDatabase_Label );
   FilterDatabase_Sizer.Add( FilterDatabase_Edit, 100 );
   FilterDatabase_Sizer.Add( FilterDatabase_Browse_PushButton );
   FilterDatabase_Sizer.Add( DownloadFilters_PushButton );

   DefaultFilter_Label.SetText( "Default filter:" );
   DefaultFilter_Label.SetMinWidth( pathLabelWidth );
   DefaultFilter_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

   DefaultFilter_Edit.SetMinWidth( editWidth2 );
   DefaultFilter_Edit.SetToolTip( "<p>Filter name used as a fallback when the FILTER keyword is missing or cannot be mapped.</p>" );
   DefaultFilter_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   UseDefaultFilter_CheckBox.SetText( "Use default filter" );
   UseDefaultFilter_CheckBox.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_CheckBox_Click, w );

   DefaultFilter_Sizer.SetSpacing( 4 );
   DefaultFilter_Sizer.Add( DefaultFilter_Label );
   DefaultFilter_Sizer.Add( DefaultFilter_Edit );
   DefaultFilter_Sizer.Add( UseDefaultFilter_CheckBox );
   DefaultFilter_Sizer.AddStretch();

   FilterMap_Label.SetText( "Filter map (JSON):" );
   FilterMap_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );

   FilterMap_Edit.SetScaledMinSize( 30, 1 );
   FilterMap_Edit.SetToolTip( "<p>JSON object mapping filter names to AstroBin filter IDs. Used as a fallback when the database is unavailable.</p>" );
   FilterMap_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   Filters_Section_Sizer.SetMargin( 6 );
   Filters_Section_Sizer.SetSpacing( 4 );
   Filters_Section_Sizer.Add( FilterDatabase_Sizer );
   Filters_Section_Sizer.Add( DefaultFilter_Sizer );
   Filters_Section_Sizer.Add( FilterMap_Label );
   Filters_Section_Sizer.Add( FilterMap_Edit, 100 );

   Filters_Section.SetSizer( Filters_Section_Sizer );
   Filters_SectionBar.SetTitle( "Filters" );
   Filters_SectionBar.SetSection( Filters_Section );

   //
   // Keyword overrides section
   //

   KeywordOverrides_Label.SetText( "Keyword overrides (JSON):" );
   KeywordOverrides_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );

   KeywordOverrides_Edit.SetScaledMinSize( 30, 1 );
   KeywordOverrides_Edit.SetToolTip( "<p>JSON object with FITS keyword overrides applied to all frames, e.g. {\\\"FOCALLEN\\\":540,\\\"XPIXSZ\\\":3.0}.</p>" );
   KeywordOverrides_Edit.OnEditCompleted( (Edit::edit_event_handler)&AstroBinCSVGeneratorInterface::e_EditCompleted, w );

   Overrides_Section_Sizer.SetMargin( 6 );
   Overrides_Section_Sizer.SetSpacing( 4 );
   Overrides_Section_Sizer.Add( KeywordOverrides_Label );
   Overrides_Section_Sizer.Add( KeywordOverrides_Edit, 100 );

   Overrides_Section.SetSizer( Overrides_Section_Sizer );
   Overrides_SectionBar.SetTitle( "Keyword Overrides" );
   Overrides_SectionBar.SetSection( Overrides_Section );

   //
   // Global layout
   //

   Global_Sizer.SetMargin( 8 );
   Global_Sizer.SetSpacing( 4 );

   Global_Sizer.Add( InputFiles_SectionBar );
   Global_Sizer.Add( InputFiles_Section );
   Global_Sizer.Add( Site_SectionBar );
   Global_Sizer.Add( Site_Section );
   Global_Sizer.Add( Equipment_SectionBar );
   Global_Sizer.Add( Equipment_Section );
   Global_Sizer.Add( Sessions_SectionBar );
   Global_Sizer.Add( Sessions_Section );
   Global_Sizer.Add( Filters_SectionBar );
   Global_Sizer.Add( Filters_Section );
   Global_Sizer.Add( Overrides_SectionBar );
   Global_Sizer.Add( Overrides_Section );

   w.SetSizer( Global_Sizer );

   Site_Section.Hide();
   Equipment_Section.Hide();
   Sessions_Section.Hide();
   Filters_Section.Hide();
   Overrides_Section.Hide();

   w.EnsureLayoutUpdated();
   w.AdjustToContents();
   w.SetFixedSize();
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorInterface.cpp

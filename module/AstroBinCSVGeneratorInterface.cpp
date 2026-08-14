//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
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
#include "AstroBinCSVGeneratorEngine.h"

#include <pcl/Console.h>
#include <pcl/Dialog.h>
#include <pcl/File.h>
#include <pcl/FileDialog.h>
#include <pcl/MessageBox.h>
#include <pcl/MetaModule.h>
#include <pcl/Settings.h>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <utility>
#include <vector>

namespace pcl
{

// ----------------------------------------------------------------------------
// TEMPORARY DIAGNOSTICS (remove after the settings-dialog crash is fixed).
// Breadcrumbs + exception logging to C:\PCL\module-diagnostics.log. The crash
// after "Import from Image" -> OK terminates PixInsight via abort() (WER:
// BEX64 c0000409, ucrtbase.dll, FAST_FAIL_FATAL_APP_EXIT), so we log every
// step of the close path to see where execution stops.
// ----------------------------------------------------------------------------

static void DbgLog( const String& s )
{
   try
   {
      std::ofstream f( "C:/PCL/module-diagnostics.log", std::ios::app );
      if ( f.is_open() )
      {
         time_t t = time( nullptr );
         struct tm tmv;
         localtime_s( &tmv, &t );
         char ts[32];
         strftime( ts, sizeof( ts ), "%H:%M:%S", &tmv );
         f << "[" << ts << "] " << s.ToUTF8().c_str() << "\n";
      }
   }
   catch ( ... )
   {
   }
}

static void DbgLogException( const char* where )
{
   try { throw; }
   catch ( const Exception& e )
   {
      DbgLog( String( where ) + ": PCL exception: " + e.Message() );
   }
   catch ( const std::exception& e )
   {
      DbgLog( String( where ) + ": std exception: " + e.what() );
   }
   catch ( ... )
   {
      DbgLog( String( where ) + ": unknown exception" );
   }
}

// ----------------------------------------------------------------------------
// Settings namespace prefix, shared with the Astro Bin CSV Generator script
// (v1.2.5) so existing user settings carry over to this module.
// ----------------------------------------------------------------------------

#define SETTINGS_NS "AstroBin CSV Generator.1.2.5_"

// ----------------------------------------------------------------------------
// Modal filter selection dialog.
//
// Mirrors the FilterPickerDialog of the JS script: a searchable list of all
// filters from the local AstroBin filter database. Returns the selected
// filter's AstroBin numeric ID and a display label.
// ----------------------------------------------------------------------------

class ABCGFilterPickerDialog : public Dialog
{
public:

   ABCGFilterPickerDialog( const std::vector<AstroBinCSVGeneratorEngine::FilterEntry>& filters )
      : m_filters( filters )
   {
      SetWindowTitle( "Select AstroBin Filter" );
      EnableUserResizing();

      int labelWidth = Font().Width( String( "Search:" ) + 'M' );

      Search_Label.SetText( "Search:" );
      Search_Label.SetMinWidth( labelWidth );
      Search_Label.SetTextAlignment( TextAlign::Right | TextAlign::VertCenter );

      Search_Edit.SetFixedWidth( 280 );
      Search_Edit.OnTextUpdated( (Edit::text_event_handler)&ABCGFilterPickerDialog::e_Search, *this );

      Search_Sizer.SetSpacing( 4 );
      Search_Sizer.Add( Search_Label );
      Search_Sizer.Add( Search_Edit );
      Search_Sizer.AddStretch();

      FilterList_TreeBox.SetNumberOfColumns( 1 );
      FilterList_TreeBox.SetHeaderText( 0, "Filter" );
      FilterList_TreeBox.SetMinHeight( 300 );
      FilterList_TreeBox.DisableRootDecoration();
      FilterList_TreeBox.EnableMultipleSelections( false );

      Status_Label.SetTextAlignment( TextAlign::Left | TextAlign::VertCenter );

      OK_PushButton.SetText( "OK" );
      OK_PushButton.SetDefault();
      OK_PushButton.OnClick( (Button::click_event_handler)&ABCGFilterPickerDialog::e_OK, *this );

      Cancel_PushButton.SetText( "Cancel" );
      Cancel_PushButton.OnClick( (Button::click_event_handler)&ABCGFilterPickerDialog::e_Cancel, *this );

      Button_Sizer.SetSpacing( 8 );
      Button_Sizer.AddStretch();
      Button_Sizer.Add( OK_PushButton );
      Button_Sizer.Add( Cancel_PushButton );

      Global_Sizer.SetSpacing( 8 );
      Global_Sizer.SetMargin( 12 );
      Global_Sizer.Add( Search_Sizer );
      Global_Sizer.Add( FilterList_TreeBox );
      Global_Sizer.Add( Status_Label );
      Global_Sizer.Add( Button_Sizer );

      SetSizer( Global_Sizer );
      AdjustToContents();
      SetFixedSize();

      PopulateList( String() );
   }

   String ChosenId() const { return m_chosenId; }
   String ChosenLabel() const { return m_chosenLabel; }

private:

   const std::vector<AstroBinCSVGeneratorEngine::FilterEntry>& m_filters;

   VerticalSizer   Global_Sizer;
   HorizontalSizer Search_Sizer;
      Label         Search_Label;
      Edit          Search_Edit;
   TreeBox         FilterList_TreeBox;
   Label           Status_Label;
   HorizontalSizer Button_Sizer;
      PushButton   OK_PushButton;
      PushButton   Cancel_PushButton;

   String m_chosenId;
   String m_chosenLabel;

   // Filters currently displayed, in tree order (used to resolve selections).
   std::vector<size_type> m_displayed;

   void PopulateList( const String& query )
   {
      FilterList_TreeBox.Clear();
      m_displayed.clear();

      IsoString q = query.Trimmed().ToIsoString();
      q.ToLowercase();

      // Sort by brand then name (index sort, String-copy safe).
      std::vector<size_type> idx( m_filters.size() );
      for ( size_type i = 0; i < idx.size(); i++ )
         idx[i] = i;
      std::sort( idx.begin(), idx.end(),
         [this]( size_type a, size_type b )
         {
            int cmp = m_filters[a].brandName.CompareIC( m_filters[b].brandName );
            if ( cmp != 0 )
               return cmp < 0;
            return m_filters[a].name.CompareIC( m_filters[b].name ) < 0;
         } );

      for ( size_type i : idx )
      {
         const AstroBinCSVGeneratorEngine::FilterEntry& f = m_filters[i];
         String label = f.brandName.IsEmpty()
                        ? f.name + " (" + f.id + ")"
                        : f.brandName + " " + f.name + " (" + f.id + ")";
         if ( !q.IsEmpty() )
         {
            IsoString l = label.ToIsoString();
            l.ToLowercase();
            if ( !l.Contains( q ) )
               continue;
         }

         // Heap-allocated; the tree owns and deletes the node.
         TreeBox::Node* node = new TreeBox::Node( FilterList_TreeBox );
         node->SetText( 0, label );
         m_displayed.push_back( i );
      }

      Status_Label.SetText( String( m_displayed.size() ) + " of " + String( m_filters.size() ) + " filters" );
   }

   void e_Search( Edit& /*sender*/ )
   {
      PopulateList( Search_Edit.Text() );
   }

   void e_OK( Button& /*sender*/ )
   {
      TreeBox::Node* current = FilterList_TreeBox.CurrentNode();
      int resolved = -1;

      if ( current != nullptr )
      {
         // Pass 1: node identity (tree order == m_displayed order)
         for ( int i = 0; i < FilterList_TreeBox.NumberOfChildren(); i++ )
            if ( FilterList_TreeBox.Child( i ) == current )
            {
               resolved = i;
               break;
            }
         // Pass 2: match by text
         if ( resolved < 0 )
         {
            String curText = current->Text( 0 );
            for ( size_type i = 0; i < m_displayed.size(); i++ )
            {
               const AstroBinCSVGeneratorEngine::FilterEntry& f = m_filters[m_displayed[i]];
               String label = f.brandName.IsEmpty()
                              ? f.name + " (" + f.id + ")"
                              : f.brandName + " " + f.name + " (" + f.id + ")";
               if ( label == curText )
               {
                  resolved = int( i );
                  break;
               }
            }
         }
      }

      if ( resolved < 0 || resolved >= int( m_displayed.size() ) )
      {
         (new MessageBox(
            "Please select a filter from the list.",
            "Select AstroBin Filter", StdIcon::Warning, StdButton::Ok ))->Execute();
         return;
      }

      const AstroBinCSVGeneratorEngine::FilterEntry& f = m_filters[m_displayed[resolved]];
      m_chosenId = f.id;
      m_chosenLabel = f.brandName.IsEmpty() ? f.name + " (" + f.id + ")"
                                            : f.brandName + " " + f.name + " (" + f.id + ")";
      Ok();
   }

   void e_Cancel( Button& /*sender*/ )
   {
      Cancel();
   }
};

// ----------------------------------------------------------------------------
// Modal settings dialog.
//
// Holds the Site, Equipment, Sessions, Filters and Keyword Overrides options
// in their own resizable window so the main interface stays compact. Values
// are edited on local copies of the controls and committed to the process
// instance only when OK is pressed.
// ----------------------------------------------------------------------------

// Mirrors the JS importFromImage() keyword lookups: returns the first
// non-empty value found among the given header keyword names.
static bool GetKeywordNumber( const std::map<std::string,ABCGJSON::Value>& kw,
                              double& out,
                              std::initializer_list<const char*> keys )
{
   for ( const char* key : keys )
   {
      auto it = kw.find( key );
      if ( it == kw.end() )
         continue;
      const ABCGJSON::Value& v = it->second;
      if ( v.type == ABCGJSON::NumberType )
      {
         out = v.num;
         return true;
      }
      if ( v.type == ABCGJSON::StringType && !v.str.empty() &&
           AstroBinCSVGeneratorEngine::JsNumber( v.str, out ) )
         return true;
      if ( v.type == ABCGJSON::BoolType )
      {
         out = v.b ? 1.0 : 0.0;
         return true;
      }
   }
   return false;
}

static bool GetKeywordString( const std::map<std::string,ABCGJSON::Value>& kw,
                              String& out,
                              std::initializer_list<const char*> keys )
{
   for ( const char* key : keys )
   {
      auto it = kw.find( key );
      if ( it == kw.end() )
         continue;
      const ABCGJSON::Value& v = it->second;
      if ( v.type == ABCGJSON::StringType && !v.str.empty() )
      {
         out = String( v.str.c_str() );
         return true;
      }
      if ( v.type == ABCGJSON::NumberType )
      {
         out = String().Format( "%g", v.num );
         return true;
      }
   }
   return false;
}

class ABCGSettingsDialog : public Dialog
{
public:

   ABCGSettingsDialog( AstroBinCSVGeneratorInstance& instance )
      : m_instance( instance )
   {
      SetWindowTitle( "AstroBin CSV Generator - Settings" );
      EnableUserResizing();

      pcl::Font fnt = Font();
      int siteLabelWidth   = fnt.Width( String( "Elevation (m):" ) + 'M' );
      int opticsLabelWidth = fnt.Width( String( "Focal length (mm):" ) + 'M' );
      int pathLabelWidth   = fnt.Width( String( "Filter database:" ) + 'M' );
      int editWidth        = 30 * fnt.Width( 'M' );
      int editWidth2       = 20 * fnt.Width( 'M' );
      int numericEditWidth = 14 * fnt.Width( 'M' );

      //
      // Site section
      //

      Import_PushButton.SetText( "Import Site/Equipment from Image..." );
      Import_PushButton.SetToolTip( "<p>Read FITS/XISF headers to populate site and equipment fields.</p>" );
      Import_PushButton.OnClick( (Button::click_event_handler)&ABCGSettingsDialog::e_ImportFromImage, *this );

      Import_Sizer.SetSpacing( 6 );
      Import_Sizer.Add( Import_PushButton );
      Import_Sizer.AddStretch();

      SiteName_Label.SetText( "Site name:" );
      SiteName_Label.SetMinWidth( siteLabelWidth );
      SiteName_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

      SiteName_Edit.SetMinWidth( editWidth2 );

      SiteName_Sizer.SetSpacing( 4 );
      SiteName_Sizer.Add( SiteName_Label );
      SiteName_Sizer.Add( SiteName_Edit );
      SiteName_Sizer.AddStretch();

      SiteLatitude_NumericControl.DisableAutoAdjustEditWidth();
      SiteLatitude_NumericControl.edit.SetMinWidth( numericEditWidth );
      SiteLatitude_NumericControl.label.SetText( "Latitude:" );
      SiteLatitude_NumericControl.label.SetMinWidth( siteLabelWidth );
      SiteLatitude_NumericControl.SetReal();
      SiteLatitude_NumericControl.SetRange( TheSiteLatitudeParameter->MinimumValue(), TheSiteLatitudeParameter->MaximumValue() );
      SiteLatitude_NumericControl.SetPrecision( TheSiteLatitudeParameter->Precision() );
      SiteLatitude_NumericControl.SetToolTip( "<p>Latitude of the observing site, in degrees (N positive).</p>" );

      SiteLongitude_NumericControl.DisableAutoAdjustEditWidth();
      SiteLongitude_NumericControl.edit.SetMinWidth( numericEditWidth );
      SiteLongitude_NumericControl.label.SetText( "Longitude:" );
      SiteLongitude_NumericControl.label.SetMinWidth( siteLabelWidth );
      SiteLongitude_NumericControl.SetReal();
      SiteLongitude_NumericControl.SetRange( TheSiteLongitudeParameter->MinimumValue(), TheSiteLongitudeParameter->MaximumValue() );
      SiteLongitude_NumericControl.SetPrecision( TheSiteLongitudeParameter->Precision() );
      SiteLongitude_NumericControl.SetToolTip( "<p>Longitude of the observing site, in degrees (E positive).</p>" );

      SiteElevation_NumericControl.DisableAutoAdjustEditWidth();
      SiteElevation_NumericControl.edit.SetMinWidth( numericEditWidth );
      SiteElevation_NumericControl.label.SetText( "Elevation (m):" );
      SiteElevation_NumericControl.label.SetMinWidth( siteLabelWidth );
      SiteElevation_NumericControl.SetReal();
      SiteElevation_NumericControl.SetRange( TheSiteElevationParameter->MinimumValue(), TheSiteElevationParameter->MaximumValue() );
      SiteElevation_NumericControl.SetPrecision( TheSiteElevationParameter->Precision() );
      SiteElevation_NumericControl.SetToolTip( "<p>Elevation of the observing site above sea level, in meters.</p>" );

      SiteCoordinates_Sizer.SetSpacing( 8 );
      SiteCoordinates_Sizer.Add( SiteLatitude_NumericControl, 100 );
      SiteCoordinates_Sizer.Add( SiteLongitude_NumericControl, 100 );

      SiteElevationSky_Sizer.SetSpacing( 8 );
      SiteElevationSky_Sizer.Add( SiteElevation_NumericControl, 100 );
      SiteElevationSky_Sizer.Add( Bortle_NumericControl, 100 );

      Bortle_NumericControl.DisableAutoAdjustEditWidth();
      Bortle_NumericControl.edit.SetMinWidth( numericEditWidth );
      Bortle_NumericControl.label.SetText( "Bortle:" );
      Bortle_NumericControl.label.SetMinWidth( siteLabelWidth );
      Bortle_NumericControl.SetInteger();
      Bortle_NumericControl.SetRange( TheBortleParameter->MinimumValue(), TheBortleParameter->MaximumValue() );
      Bortle_NumericControl.SetToolTip( "<p>Bortle dark-sky class, from 1 (excellent) to 9 (inner city).</p>" );

      SQM_NumericControl.DisableAutoAdjustEditWidth();
      SQM_NumericControl.edit.SetMinWidth( numericEditWidth );
      SQM_NumericControl.label.SetText( "SQM:" );
      SQM_NumericControl.label.SetMinWidth( siteLabelWidth );
      SQM_NumericControl.SetReal();
      SQM_NumericControl.SetRange( TheSQMParameter->MinimumValue(), TheSQMParameter->MaximumValue() );
      SQM_NumericControl.SetPrecision( TheSQMParameter->Precision() );
      SQM_NumericControl.SetToolTip( "<p>Sky Quality Meter reading, in magnitudes per square arcsecond.</p>" );

      SiteSky_Sizer.SetSpacing( 8 );
      SiteSky_Sizer.Add( SQM_NumericControl, 100 );

      Site_Section_Sizer.SetMargin( 6 );
      Site_Section_Sizer.SetSpacing( 4 );
      Site_Section_Sizer.Add( Import_Sizer );
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

      FocalLength_NumericControl.DisableAutoAdjustEditWidth();
      FocalLength_NumericControl.edit.SetMinWidth( numericEditWidth );
      FocalLength_NumericControl.label.SetText( "Focal length (mm):" );
      FocalLength_NumericControl.label.SetMinWidth( opticsLabelWidth );
      FocalLength_NumericControl.SetReal();
      FocalLength_NumericControl.SetRange( TheFocalLengthParameter->MinimumValue(), TheFocalLengthParameter->MaximumValue() );
      FocalLength_NumericControl.SetPrecision( TheFocalLengthParameter->Precision() );
      FocalLength_NumericControl.SetToolTip( "<p>Focal length of the telescope, in millimeters.</p>" );

      PixelSize_NumericControl.DisableAutoAdjustEditWidth();
      PixelSize_NumericControl.edit.SetMinWidth( numericEditWidth );
      PixelSize_NumericControl.label.SetText( "Pixel size (um):" );
      PixelSize_NumericControl.label.SetMinWidth( opticsLabelWidth );
      PixelSize_NumericControl.SetReal();
      PixelSize_NumericControl.SetRange( ThePixelSizeParameter->MinimumValue(), ThePixelSizeParameter->MaximumValue() );
      PixelSize_NumericControl.SetPrecision( ThePixelSizeParameter->Precision() );
      PixelSize_NumericControl.SetToolTip( "<p>Camera pixel size, in micrometers.</p>" );

      FocalRatio_NumericControl.DisableAutoAdjustEditWidth();
      FocalRatio_NumericControl.edit.SetMinWidth( numericEditWidth );
      FocalRatio_NumericControl.label.SetText( "Focal ratio:" );
      FocalRatio_NumericControl.label.SetMinWidth( opticsLabelWidth );
      FocalRatio_NumericControl.SetReal();
      FocalRatio_NumericControl.SetRange( TheFocalRatioParameter->MinimumValue(), TheFocalRatioParameter->MaximumValue() );
      FocalRatio_NumericControl.SetPrecision( TheFocalRatioParameter->Precision() );
      FocalRatio_NumericControl.SetToolTip( "<p>Focal ratio of the telescope.</p>" );

      DefaultGain_NumericControl.DisableAutoAdjustEditWidth();
      DefaultGain_NumericControl.edit.SetMinWidth( numericEditWidth );
      DefaultGain_NumericControl.label.SetText( "Default gain:" );
      DefaultGain_NumericControl.label.SetMinWidth( opticsLabelWidth );
      DefaultGain_NumericControl.SetInteger();
      DefaultGain_NumericControl.SetRange( TheDefaultGainParameter->MinimumValue(), TheDefaultGainParameter->MaximumValue() );
      DefaultGain_NumericControl.SetToolTip( "<p>Default gain (ISO) value used when the GAIN header keyword is missing.</p>" );

      DefaultTemperature_NumericControl.DisableAutoAdjustEditWidth();
      DefaultTemperature_NumericControl.edit.SetMinWidth( numericEditWidth );
      DefaultTemperature_NumericControl.label.SetText( "Default temp (C):" );
      DefaultTemperature_NumericControl.label.SetMinWidth( opticsLabelWidth );
      DefaultTemperature_NumericControl.SetReal();
      DefaultTemperature_NumericControl.SetRange( TheDefaultTemperatureParameter->MinimumValue(), TheDefaultTemperatureParameter->MaximumValue() );
      DefaultTemperature_NumericControl.SetPrecision( TheDefaultTemperatureParameter->Precision() );
      DefaultTemperature_NumericControl.SetToolTip( "<p>Default sensor temperature, in Celsius, used when the CCD-TEMP header keyword is missing.</p>" );

      Optics_Sizer.SetSpacing( 8 );
      Optics_Sizer.Add( FocalLength_NumericControl, 100 );
      Optics_Sizer.Add( PixelSize_NumericControl, 100 );

      OpticsMisc_Sizer.SetSpacing( 8 );
      OpticsMisc_Sizer.Add( FocalRatio_NumericControl, 100 );
      OpticsMisc_Sizer.Add( DefaultGain_NumericControl, 100 );

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

      SessionGapHours_NumericControl.DisableAutoAdjustEditWidth();
      SessionGapHours_NumericControl.edit.SetMinWidth( numericEditWidth );
      SessionGapHours_NumericControl.label.SetText( "Session gap (hours):" );
      SessionGapHours_NumericControl.SetReal();
      SessionGapHours_NumericControl.SetRange( TheSessionGapHoursParameter->MinimumValue(), TheSessionGapHoursParameter->MaximumValue() );
      SessionGapHours_NumericControl.SetPrecision( TheSessionGapHoursParameter->Precision() );
      SessionGapHours_NumericControl.SetToolTip( "<p>Minimum time gap (hours) between consecutive frames that defines a new imaging session.</p>" );

      ShiftOvernight_CheckBox.SetText( "Shift overnight sessions to the previous day" );
      ShiftOvernight_CheckBox.SetToolTip( "<p>Assign frames acquired after midnight to the previous calendar day.</p>" );

      UseObservingDate_CheckBox.SetText( "Use observing date (DATE-OBS) for sorting" );
      UseObservingDate_CheckBox.SetToolTip( "<p>Use DATE-OBS instead of the file modification date when sorting frames.</p>" );

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
      FilterDatabase_Edit.OnEditCompleted( (Edit::edit_event_handler)&ABCGSettingsDialog::e_FilterDatabase, *this );

      FilterDatabase_Browse_PushButton.SetText( "Browse" );
      FilterDatabase_Browse_PushButton.OnClick( (Button::click_event_handler)&ABCGSettingsDialog::e_FilterDatabaseBrowse, *this );

      DownloadFilters_PushButton.SetText( "Download" );
      DownloadFilters_PushButton.SetToolTip( "<p>Download the AstroBin filter database from the AstroBin REST API.</p>" );
      DownloadFilters_PushButton.OnClick( (Button::click_event_handler)&ABCGSettingsDialog::e_Download, *this );

      FilterDatabase_Status_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );
      FilterDatabase_Status_Label.SetToolTip( "<p>Status of the local AstroBin filter database cache.</p>" );

      FilterDatabase_Sizer.SetSpacing( 4 );
      FilterDatabase_Sizer.Add( FilterDatabase_Label );
      FilterDatabase_Sizer.Add( FilterDatabase_Edit, 100 );
      FilterDatabase_Sizer.Add( FilterDatabase_Browse_PushButton );
      FilterDatabase_Sizer.Add( DownloadFilters_PushButton );

      FilterDatabase_Status_Sizer.SetSpacing( 4 );
      FilterDatabase_Status_Sizer.Add( FilterDatabase_Status_Label );

      DefaultFilter_Label.SetText( "Default filter:" );
      DefaultFilter_Label.SetMinWidth( pathLabelWidth );
      DefaultFilter_Label.SetTextAlignment( TextAlign::Right|TextAlign::VertCenter );

      DefaultFilter_Edit.SetMinWidth( editWidth2 );
      DefaultFilter_Edit.SetToolTip( "<p>Filter name used as a fallback when the FILTER keyword is missing or cannot be mapped.</p>" );

      UseDefaultFilter_CheckBox.SetText( "Use default filter" );

      DefaultFilter_Sizer.SetSpacing( 4 );
      DefaultFilter_Sizer.Add( DefaultFilter_Label );
      DefaultFilter_Sizer.Add( DefaultFilter_Edit );
      DefaultFilter_Sizer.Add( UseDefaultFilter_CheckBox );
      DefaultFilter_Sizer.AddStretch();

      FilterMap_Label.SetText( "Filter map (JSON):" );
      FilterMap_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );

      FilterMap_Edit.SetScaledMinSize( 30, 1 );
      FilterMap_Edit.SetToolTip( "<p>JSON object mapping filter names to AstroBin filter IDs. Used as a fallback when the database is unavailable.</p>" );

      Filters_Section_Sizer.SetMargin( 6 );
      Filters_Section_Sizer.SetSpacing( 4 );
      Filters_Section_Sizer.Add( FilterDatabase_Sizer );
      Filters_Section_Sizer.Add( FilterDatabase_Status_Sizer );
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

      Overrides_Section_Sizer.SetMargin( 6 );
      Overrides_Section_Sizer.SetSpacing( 4 );
      Overrides_Section_Sizer.Add( KeywordOverrides_Label );
      Overrides_Section_Sizer.Add( KeywordOverrides_Edit, 100 );

      Overrides_Section.SetSizer( Overrides_Section_Sizer );
      Overrides_SectionBar.SetTitle( "Keyword Overrides" );
      Overrides_SectionBar.SetSection( Overrides_Section );

      //
      // Dialog buttons
      //

      OK_PushButton.SetText( "OK" );
      OK_PushButton.SetDefault();
      OK_PushButton.OnClick( (Button::click_event_handler)&ABCGSettingsDialog::e_OK, *this );

      Cancel_PushButton.SetText( "Cancel" );
      Cancel_PushButton.OnClick( (Button::click_event_handler)&ABCGSettingsDialog::e_Cancel, *this );

      Button_Sizer.SetSpacing( 8 );
      Button_Sizer.AddStretch();
      Button_Sizer.Add( OK_PushButton );
      Button_Sizer.Add( Cancel_PushButton );

      //
      // Global layout
      //

      Global_Sizer.SetMargin( 12 );
      Global_Sizer.SetSpacing( 8 );
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
      Global_Sizer.Add( Button_Sizer );

      SetSizer( Global_Sizer );

      // Sections start collapsed so the dialog opens compact. Clicking a
      // section bar expands it and the window resizes to fit.
      Site_Section.Hide();
      Equipment_Section.Hide();
      Sessions_Section.Hide();
      Filters_Section.Hide();
      Overrides_Section.Hide();

      EnsureLayoutUpdated();
      AdjustToContents();

      // Populate the controls from the instance.
      SiteName_Edit.SetText( m_instance.p_siteName );
      SiteLatitude_NumericControl.SetValue( m_instance.p_siteLatitude );
      SiteLongitude_NumericControl.SetValue( m_instance.p_siteLongitude );
      SiteElevation_NumericControl.SetValue( m_instance.p_siteElevation );
      Bortle_NumericControl.SetValue( m_instance.p_bortle );
      SQM_NumericControl.SetValue( m_instance.p_sqm );
      FocalLength_NumericControl.SetValue( m_instance.p_focalLength );
      PixelSize_NumericControl.SetValue( m_instance.p_pixelSize );
      FocalRatio_NumericControl.SetValue( m_instance.p_focalRatio );
      DefaultGain_NumericControl.SetValue( m_instance.p_defaultGain );
      DefaultTemperature_NumericControl.SetValue( m_instance.p_defaultTemperature );
      SessionGapHours_NumericControl.SetValue( m_instance.p_sessionGapHours );
      ShiftOvernight_CheckBox.SetChecked( m_instance.p_shiftOvernight );
      UseObservingDate_CheckBox.SetChecked( m_instance.p_useObservingDate );
      FilterDatabase_Edit.SetText( m_instance.p_filterDatabasePath );
      DefaultFilter_Edit.SetText( m_instance.p_defaultFilter );
      UseDefaultFilter_CheckBox.SetChecked( m_instance.p_useDefaultFilter );
      FilterMap_Edit.SetText( m_instance.p_filterMap );
      KeywordOverrides_Edit.SetText( m_instance.p_keywordOverrides );

      UpdateFilterDatabaseStatus();
   }

private:

   AstroBinCSVGeneratorInstance& m_instance;

   VerticalSizer Global_Sizer;

   Control       Site_Section;
   SectionBar    Site_SectionBar;
   VerticalSizer Site_Section_Sizer;
         HorizontalSizer Import_Sizer;
            PushButton       Import_PushButton;
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

      Control       Equipment_Section;
      SectionBar    Equipment_SectionBar;
      VerticalSizer Equipment_Section_Sizer;
         HorizontalSizer Optics_Sizer;
            NumericControl    FocalLength_NumericControl;
            NumericControl    PixelSize_NumericControl;
         HorizontalSizer OpticsMisc_Sizer;
            NumericControl    FocalRatio_NumericControl;
            NumericControl    DefaultGain_NumericControl;
         HorizontalSizer Camera_Sizer;
            NumericControl    DefaultTemperature_NumericControl;

      Control       Sessions_Section;
      SectionBar    Sessions_SectionBar;
      VerticalSizer Sessions_Section_Sizer;
         NumericControl SessionGapHours_NumericControl;
         CheckBox       ShiftOvernight_CheckBox;
         CheckBox       UseObservingDate_CheckBox;

      Control       Filters_Section;
      SectionBar    Filters_SectionBar;
      VerticalSizer Filters_Section_Sizer;
      HorizontalSizer FilterDatabase_Sizer;
         Label            FilterDatabase_Label;
         Edit             FilterDatabase_Edit;
         PushButton       FilterDatabase_Browse_PushButton;
         PushButton       DownloadFilters_PushButton;
      HorizontalSizer FilterDatabase_Status_Sizer;
         Label            FilterDatabase_Status_Label;
      HorizontalSizer DefaultFilter_Sizer;
         Label            DefaultFilter_Label;
         Edit             DefaultFilter_Edit;
         CheckBox         UseDefaultFilter_CheckBox;
      Label           FilterMap_Label;
      Edit            FilterMap_Edit;

      Control       Overrides_Section;
      SectionBar    Overrides_SectionBar;
      VerticalSizer Overrides_Section_Sizer;
         Label   KeywordOverrides_Label;
         Edit            KeywordOverrides_Edit;

      HorizontalSizer Button_Sizer;
      PushButton OK_PushButton;
      PushButton Cancel_PushButton;

   void UpdateFilterDatabaseStatus();
   void Commit();
   void ImportFromImage();

   void e_ImportFromImage( Button& sender, bool checked );
   void e_FilterDatabase( Edit& sender );
   void e_FilterDatabaseBrowse( Button& sender, bool checked );
   void e_Download( Button& sender, bool checked );
   void e_OK( Button& sender, bool checked );
   void e_Cancel( Button& sender, bool checked );
};

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::UpdateFilterDatabaseStatus()
{
   String dbPath = FilterDatabase_Edit.Text().Trimmed();
   if ( dbPath.IsEmpty() )
      dbPath = File::HomeDirectory() + "/PixInsight/AstroBinFilters.json";

   if ( File::Exists( dbPath ) )
   {
      try
      {
         IsoString content = File::ReadTextFile( dbPath );
         ABCGJSON::Value root;
         if ( ABCGJSON::Parse( content.c_str(), root ) && root.IsObject() )
         {
            const ABCGJSON::Value* filters = root.Find( "filters" );
            if ( filters != nullptr && filters->IsArray() )
            {
               const ABCGJSON::Value* updated = root.Find( "lastUpdated" );
               String when = ( updated != nullptr && updated->type == ABCGJSON::StringType )
                             ? String( updated->str.c_str() ) : String( "unknown" );
               FilterDatabase_Status_Label.SetText(
                  "Database: " + String( filters->arr.size() ) + " filters (updated: " + when + ")" );
               return;
            }
         }
      }
      catch ( ... )
      {
      }
   }

   FilterDatabase_Status_Label.SetText( "Database: not found (click Download)" );
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::Commit()
{
   m_instance.p_siteName = SiteName_Edit.Text().Trimmed();
   m_instance.p_siteLatitude = SiteLatitude_NumericControl.Value();
   m_instance.p_siteLongitude = SiteLongitude_NumericControl.Value();
   m_instance.p_siteElevation = SiteElevation_NumericControl.Value();
   m_instance.p_bortle = int( Bortle_NumericControl.Value() );
   m_instance.p_sqm = SQM_NumericControl.Value();
   m_instance.p_focalLength = FocalLength_NumericControl.Value();
   m_instance.p_pixelSize = PixelSize_NumericControl.Value();
   m_instance.p_focalRatio = FocalRatio_NumericControl.Value();
   m_instance.p_defaultGain = int( DefaultGain_NumericControl.Value() );
   m_instance.p_defaultTemperature = DefaultTemperature_NumericControl.Value();
   m_instance.p_sessionGapHours = SessionGapHours_NumericControl.Value();
   m_instance.p_shiftOvernight = ShiftOvernight_CheckBox.IsChecked();
   m_instance.p_useObservingDate = UseObservingDate_CheckBox.IsChecked();
   m_instance.p_filterDatabasePath = FilterDatabase_Edit.Text().Trimmed();
   m_instance.p_defaultFilter = DefaultFilter_Edit.Text().Trimmed();
   m_instance.p_useDefaultFilter = UseDefaultFilter_CheckBox.IsChecked();
   m_instance.p_filterMap = FilterMap_Edit.Text().Trimmed();
   m_instance.p_keywordOverrides = KeywordOverrides_Edit.Text().Trimmed();
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::e_FilterDatabase( Edit& sender )
{
   try
   {
      sender.SetText( sender.Text().Trimmed() );
      UpdateFilterDatabaseStatus();
   }
   catch ( ... )
   {
      DbgLogException( "e_FilterDatabase" );
   }
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::e_FilterDatabaseBrowse( Button& /*sender*/, bool /*checked*/ )
{
   try
   {
      OpenFileDialog dlg;
      dlg.SetCaption( "Select Filter Database JSON File" );
      dlg.SetFilter( FileFilter( "JSON files (*.json)", "json" ) );
      if ( dlg.Execute() )
      {
         FilterDatabase_Edit.SetText( dlg.FileName() );
         UpdateFilterDatabaseStatus();
      }
   }
   catch ( ... )
   {
      DbgLogException( "e_FilterDatabaseBrowse" );
   }
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::e_Download( Button& /*sender*/, bool /*checked*/ )
{
   try
   {
   // Confirm before starting (matches the JS script's behavior).
   MessageBox confirm(
      "This will download the entire AstroBin filter database "
      "(approximately 2500 filters).\n"
      "This may take a minute. Continue?",
      "AstroBin CSV Generator - Download Filter Database",
      StdIcon::Question, StdButton::Yes, StdButton::No );
   if ( confirm.Execute() != StdButton::Yes )
      return;

   String dbPath = FilterDatabase_Edit.Text().Trimmed();
   if ( dbPath.IsEmpty() )
      dbPath = File::HomeDirectory() + "/PixInsight/AstroBinFilters.json";

   // Disable the dialog buttons while the download runs so the dialog cannot
   // be dismissed mid-download. The download runs synchronously on the main
   // thread (the engine pumps events between pages); without this the user
   // could press OK/Cancel, which would close the dialog and destroy its
   // controls while this handler is still running (re-entrant Return() ->
   // use-after-free -> PixInsight freeze).
   OK_PushButton.Disable();
   Cancel_PushButton.Disable();
   DownloadFilters_PushButton.Disable();

   FilterDatabase_Status_Label.SetText( "Downloading filter database..." );
   FilterDatabase_Status_Label.SetTextAlignment( TextAlign::Left|TextAlign::VertCenter );

   AstroBinCSVGeneratorEngine engine;
   engine.FilterDatabasePath = dbPath;
   bool ok = engine.DownloadFilterDatabase();

   if ( ok && IsVisible() )
   {
      FilterDatabase_Status_Label.SetText(
         "Database: " + String( engine.FilterCount() ) + " filters (saved to " + dbPath + ")" );
      (new MessageBox(
         "Filter database downloaded successfully!\n\n" +
         String( engine.FilterCount() ) + " filters loaded.\n" +
         "Saved to: " + dbPath,
         "AstroBin CSV Generator", StdIcon::Information, StdButton::Ok ))->Execute();
   }
   else if ( IsVisible() )
   {
      FilterDatabase_Status_Label.SetText( "Database: download failed. See the console." );
      (new MessageBox(
         "Failed to download filter database.\n"
         "Check the console for error details.",
         "AstroBin CSV Generator", StdIcon::Error, StdButton::Ok ))->Execute();
   }

   OK_PushButton.Enable();
   Cancel_PushButton.Enable();
   DownloadFilters_PushButton.Enable();
   }
   catch ( ... )
   {
      DbgLogException( "e_Download" );
   }
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::e_OK( Button& /*sender*/, bool /*checked*/ )
{
   DbgLog( "e_OK enter" );
   try
   {
      Commit();
      DbgLog( "e_OK Commit done" );
      Ok();
      DbgLog( "e_OK Ok done" );
   }
   catch ( ... )
   {
      DbgLogException( "e_OK" );
   }
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::e_Cancel( Button& /*sender*/, bool /*checked*/ )
{
   try
   {
      Cancel();
   }
   catch ( ... )
   {
      DbgLogException( "e_Cancel" );
   }
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::e_ImportFromImage( Button& /*sender*/, bool /*checked*/ )
{
   try
   {
      ImportFromImage();
   }
   catch ( ... )
   {
      DbgLogException( "e_ImportFromImage" );
   }
}

// ----------------------------------------------------------------------------

void ABCGSettingsDialog::ImportFromImage()
{
   OpenFileDialog dlg;
   dlg.SetCaption( "Select an image to import site/equipment from" );
   dlg.SetFilter( FileFilter( "All supported files (*.fit *.fits *.fts *.xisf)",
      StringList{ "fit", "fits", "fts", "xisf" } ) );
   if ( !dlg.Execute() )
      return;

   DbgLog( "ImportFromImage: file selected" );

   String filePath = dlg.FileName();
   String ext = File::ExtractExtension( filePath );
   ext.ToLowercase();

   std::map<std::string,ABCGJSON::Value> keywords;
   bool ok = false;
   try
   {
      if ( ext == ".xisf" )
         ok = AstroBinCSVGeneratorEngine::ReadXISFHeaders( filePath, keywords );
      else
         ok = AstroBinCSVGeneratorEngine::ReadFITSHeaders( filePath, keywords );
   }
   catch ( ... )
   {
      ok = false;
   }

   if ( !ok || keywords.empty() )
   {
      DbgLog( "ImportFromImage: no keywords" );
      (new MessageBox( "No keywords found in " + File::ExtractNameAndExtension( filePath ),
         "Import Site/Equipment from Image", StdIcon::Warning, StdButton::Ok ))->Execute();
      return;
   }

   DbgLog( "ImportFromImage: keywords read (" + String( keywords.size() ) + ")" );

   StringList imported;
   String strVal;
   double numVal;

   // Site info
   if ( GetKeywordString( keywords, strVal, { "SITE", "SITENAME", "OBSERVAT" } ) )
   {
      SiteName_Edit.SetText( strVal );
      imported.Append( "Site: " + strVal );
   }
   if ( GetKeywordNumber( keywords, numVal, { "SITELAT", "OBSGEO-B", "LAT-OBS", "LATITUDE" } ) )
   {
      SiteLatitude_NumericControl.SetValue( numVal );
      imported.Append( "Latitude: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "SITELONG", "OBSGEO-L", "LONG-OBS", "LONGITUDE" } ) )
   {
      SiteLongitude_NumericControl.SetValue( numVal );
      imported.Append( "Longitude: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "SITEELEV", "ELEVATION" } ) )
   {
      SiteElevation_NumericControl.SetValue( numVal );
      imported.Append( "Elevation: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "BORTLE" } ) )
   {
      Bortle_NumericControl.SetValue( numVal );
      imported.Append( "Bortle: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "SQM", "SKYQUAL" } ) )
   {
      SQM_NumericControl.SetValue( numVal );
      imported.Append( "SQM: " + String().Format( "%g", numVal ) );
   }

   // Equipment defaults
   if ( GetKeywordNumber( keywords, numVal, { "FOCALLEN", "FOC-LEN", "FOCLENGTH", "EFL" } ) )
   {
      FocalLength_NumericControl.SetValue( numVal );
      imported.Append( "Focal Length: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "XPIXSZ", "YPIXSZ", "PIXSIZE" } ) )
   {
      PixelSize_NumericControl.SetValue( numVal );
      imported.Append( "Pixel Size: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "FOCRATIO", "FOCUS" } ) )
   {
      FocalRatio_NumericControl.SetValue( numVal );
      imported.Append( "F-Ratio: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "GAIN" } ) )
   {
      DefaultGain_NumericControl.SetValue( numVal );
      imported.Append( "Gain: " + String().Format( "%g", numVal ) );
   }
   if ( GetKeywordNumber( keywords, numVal, { "CCD-TEMP", "CCDTEMP", "SET-TEMP" } ) )
   {
      DefaultTemperature_NumericControl.SetValue( numVal );
      imported.Append( "Temp: " + String().Format( "%g", numVal ) );
   }

   if ( imported.Length() > 0 )
   {
      String msg = "Imported from " + File::ExtractNameAndExtension( filePath ) + ":\n\n";
      for ( int i = 0; i < imported.Length(); ++i )
      {
         if ( i > 0 )
            msg += '\n';
         msg += imported[i];
      }
      DbgLog( "ImportFromImage: showing result message box" );
      (new MessageBox( msg, "Import Site/Equipment from Image",
         StdIcon::Information, StdButton::Ok ))->Execute();
      DbgLog( "ImportFromImage: result message box returned" );
   }
   else
   {
      (new MessageBox(
         "No site or equipment keywords found in " + File::ExtractNameAndExtension( filePath ),
         "Import Site/Equipment from Image", StdIcon::Warning, StdButton::Ok ))->Execute();
   }
}

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
      DbgLog( "Launch: creating GUI" );
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
   SyncFileListFromInstance();

   GUI->OutputDirectory_Edit.SetText( m_instance.p_outputDirectory );
   GUI->OutputFileName_Edit.SetText( m_instance.p_outputFileName );
   GUI->OverrideFile_Edit.SetText( m_instance.p_overrideFilePath );
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
    if ( Settings::Read( SETTINGS_NS "filterDatabasePath", s ) )
       if ( !s.IsEmpty() )
          m_instance.p_filterDatabasePath = s;
    if ( Settings::Read( SETTINGS_NS "fileList", s ) )
       if ( !s.IsEmpty() )
          m_instance.p_fileList = s;
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
   Settings::Write( SETTINGS_NS "filterDatabasePath", m_instance.p_filterDatabasePath );
   Settings::Write( SETTINGS_NS "fileList", m_instance.p_fileList );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

AstroBinCSVGeneratorEngine AstroBinCSVGeneratorInterface::CreateEngine() const
{
   AstroBinCSVGeneratorEngine engine;
   engine.SessionGapHours      = m_instance.p_sessionGapHours;
   engine.ShiftOvernight       = m_instance.p_shiftOvernight;
   engine.UseObservingDate     = m_instance.p_useObservingDate;
   engine.DefaultGain          = m_instance.p_defaultGain;
   engine.DefaultTemperature   = m_instance.p_defaultTemperature;
   engine.DefaultFilter        = m_instance.p_defaultFilter;
   engine.UseDefaultFilter     = m_instance.p_useDefaultFilter;
   engine.SiteName             = m_instance.p_siteName;
   engine.SiteLatitude         = m_instance.p_siteLatitude;
   engine.SiteLongitude        = m_instance.p_siteLongitude;
   engine.SiteElevation        = m_instance.p_siteElevation;
   engine.Bortle               = m_instance.p_bortle;
   engine.SQM                  = m_instance.p_sqm;
   engine.FocalLength          = m_instance.p_focalLength;
   engine.PixelSize            = m_instance.p_pixelSize;
   engine.FocalRatio           = m_instance.p_focalRatio;
   engine.FilterMapJSON        = m_instance.p_filterMap;
   engine.KeywordOverridesJSON = m_instance.p_keywordOverrides;
   engine.FilterDatabasePath   = m_instance.p_filterDatabasePath;
   return engine;
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::RebuildFileTree()
{
   GUI->FileList_TreeBox.Clear();

   for ( const AstroBinCSVGeneratorEngine::FrameData& f : m_files )
   {
      String type;
      if ( f.isMaster )
         type = "Master";
      else
      {
         type = f.imagetyp;
         type.ToUppercase();
      }

      String filter = f.hasFilterOverride ? f.filterOverrideLabel : f.filter;
      if ( filter.IsEmpty() )
         filter = "?";

      // Nodes are heap-allocated; the tree owns and deletes them. A stack
      // node would be removed from the tree when it goes out of scope.
      TreeBox::Node* node = new TreeBox::Node( GUI->FileList_TreeBox );
      node->SetText( 0, File::ExtractNameAndExtension( f.filePath ) );
      node->SetText( 1, filter );
      node->SetText( 2, String().Format( "%.2f", f.exposure ) );
      node->SetText( 3, String().Format( "%.0f", f.gain ) );
      node->SetText( 4, String().Format( "%.1f", f.ccdTemp ) );
      node->SetText( 5, f.dateObs );
      node->SetText( 6, type );
   }
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::SyncFileListToInstance()
{
   ABCGJSON::Value root;
   root.type = ABCGJSON::ArrayType;

   for ( const AstroBinCSVGeneratorEngine::FrameData& f : m_files )
   {
      ABCGJSON::Value el;
      el.type = ABCGJSON::ObjectType;

      ABCGJSON::Value path;
      path.type = ABCGJSON::StringType;
      path.str = f.filePath.ToIsoString().c_str();
      el.obj.push_back( { "path", std::move( path ) } );

      if ( f.hasFilterOverride )
      {
         ABCGJSON::Value id;
         id.type = ABCGJSON::StringType;
         id.str = f.filterOverrideId.ToIsoString().c_str();
         el.obj.push_back( { "filterId", std::move( id ) } );

         ABCGJSON::Value label;
         label.type = ABCGJSON::StringType;
         label.str = f.filterOverrideLabel.ToIsoString().c_str();
         el.obj.push_back( { "filterLabel", std::move( label ) } );
      }

      root.arr.push_back( el );
   }

   m_instance.p_fileList = String( ABCGJSON::Serialize( root ).c_str() );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::SyncFileListFromInstance()
{
   m_files.clear();

   ABCGJSON::Value root;
   std::string text = m_instance.p_fileList.ToIsoString().c_str();
   if ( ABCGJSON::Parse( text, root ) && root.IsArray() )
   {
      for ( const ABCGJSON::Value& el : root.arr )
      {
         if ( !el.IsObject() )
            continue;

         const ABCGJSON::Value* path = el.Find( "path" );
         if ( path == nullptr || path->type != ABCGJSON::StringType || path->str.empty() )
            continue;

         AstroBinCSVGeneratorEngine::FrameData f;
         f.filePath = String( path->str.c_str() );
         f.fileName = File::ExtractNameAndExtension( f.filePath );

         const ABCGJSON::Value* id = el.Find( "filterId" );
         if ( id != nullptr && id->type == ABCGJSON::StringType && !id->str.empty() )
         {
            f.hasFilterOverride = true;
            f.filterOverrideId = String( id->str.c_str() );

            const ABCGJSON::Value* label = el.Find( "filterLabel" );
            f.filterOverrideLabel = ( label != nullptr && label->type == ABCGJSON::StringType )
                                    ? String( label->str.c_str() ) : f.filterOverrideId;
         }

         m_files.push_back( f );
      }
   }

   RebuildFileTree();
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::ProcessFiles( const StringList& paths )
{
   if ( paths.IsEmpty() )
      return;

   // Skip paths already present in the list.
   StringList pending;
   for ( const String& path : paths )
   {
      bool found = false;
      for ( const AstroBinCSVGeneratorEngine::FrameData& f : m_files )
         if ( f.filePath == path )
         {
            found = true;
            break;
         }
      if ( !found )
         pending.Add( path );
   }

   if ( pending.IsEmpty() )
      return;

   AstroBinCSVGeneratorEngine engine = CreateEngine();
   engine.LoadFilterDatabase();

   Console().Show();
   for ( const String& path : pending )
   {
      Console().WriteLn( "Reading headers: " + path );
      m_files.push_back( engine.ExtractFrame( path ) );
   }

   // Sort by file path. std::sort must not be applied directly to containers of
   // pcl::String-bearing structs (PCL move-assign null-deref), so sort an index
   // vector and copy-construct the result (copy ctor is refcount-safe).
   std::vector<size_type> idx( m_files.size() );
   for ( size_type i = 0; i < idx.size(); i++ )
      idx[i] = i;
   std::sort( idx.begin(), idx.end(),
      [this]( size_type a, size_type b )
      {
         return m_files[a].filePath.CompareIC( m_files[b].filePath ) < 0;
      } );

   std::vector<AstroBinCSVGeneratorEngine::FrameData> sorted;
   sorted.reserve( m_files.size() );
   for ( size_type i : idx )
      sorted.push_back( m_files[i] );
   m_files = std::move( sorted );

   RebuildFileTree();
   SyncFileListToInstance();
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::SetFilterForSelected()
{
   IndirectArray<TreeBox::Node> nodes = GUI->FileList_TreeBox.SelectedNodes();
   if ( nodes.IsEmpty() )
      return;

   AstroBinCSVGeneratorEngine engine = CreateEngine();
   engine.LoadFilterDatabase();

   const std::vector<AstroBinCSVGeneratorEngine::FilterEntry>& filters = engine.Filters();
   if ( filters.empty() )
   {
      (new MessageBox(
         "The AstroBin filter database is empty. Please download it in the "
         "Filters section of the Settings dialog first.",
         "No Filters Available", StdIcon::Warning, StdButton::Ok ))->Execute();
      return;
   }

   ABCGFilterPickerDialog dlg( filters );
   if ( dlg.Execute() != StdDialogCode::Ok )
      return;

   String id = dlg.ChosenId();
   String label = dlg.ChosenLabel();

   for ( TreeBox::Node* n : nodes )
   {
      int idx = -1;
      for ( int i = 0; i < GUI->FileList_TreeBox.NumberOfChildren(); i++ )
         if ( GUI->FileList_TreeBox.Child( i ) == n )
         {
            idx = i;
            break;
         }
      if ( idx < 0 || idx >= int( m_files.size() ) )
         continue;

      m_files[idx].hasFilterOverride = true;
      m_files[idx].filterOverrideId = id;
      m_files[idx].filterOverrideLabel = label;
   }

   RebuildFileTree();
   SyncFileListToInstance();
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_AddFiles_Click( Button& /*sender*/, bool /*checked*/ )
{
   OpenFileDialog dlg;
   dlg.SetCaption( "Select FITS/XISF Files" );
   dlg.SetFilter( FileFilter( "FITS/XISF files (*.fit *.fits *.fts *.xisf)",
      StringList{ "fit", "fits", "fts", "xisf" } ) );
   dlg.EnableMultipleSelections( true );
   if ( dlg.Execute() )
      ProcessFiles( dlg.FileNames() );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_AddDirectory_Click( Button& /*sender*/, bool /*checked*/ )
{
   GetDirectoryDialog dlg;
   dlg.SetCaption( "Select Folder with Light Frames" );
   if ( !dlg.Execute() )
      return;

   AstroBinCSVGeneratorEngine engine = CreateEngine();
   std::vector<String> files;
   engine.CollectFiles( dlg.Directory(), true, files );

   StringList paths;
   for ( const String& p : files )
      paths.Add( p );
   ProcessFiles( paths );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_RemoveSelected_Click( Button& /*sender*/, bool /*checked*/ )
{
   IndirectArray<TreeBox::Node> nodes = GUI->FileList_TreeBox.SelectedNodes();
   if ( nodes.IsEmpty() )
      return;

   // Collect the node indices in the current tree order.
   std::vector<int> indices;
   for ( TreeBox::Node* n : nodes )
      for ( int i = 0; i < GUI->FileList_TreeBox.NumberOfChildren(); i++ )
         if ( GUI->FileList_TreeBox.Child( i ) == n )
         {
            indices.push_back( i );
            break;
         }

   // Rebuild the list without the removed rows. vector::erase() is unsafe here
   // because it move-assigns pcl::String-bearing elements (PCL null-deref bug).
   std::vector<bool> remove( m_files.size(), false );
   for ( int idx : indices )
      if ( idx >= 0 && idx < int( m_files.size() ) )
         remove[idx] = true;

   std::vector<AstroBinCSVGeneratorEngine::FrameData> kept;
   kept.reserve( m_files.size() );
   for ( size_type i = 0; i < m_files.size(); i++ )
      if ( !remove[i] )
         kept.push_back( m_files[i] );
   m_files = std::move( kept );

   RebuildFileTree();
   SyncFileListToInstance();
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_ClearAll_Click( Button& /*sender*/, bool /*checked*/ )
{
   if ( m_files.empty() )
      return;

   m_files.clear();
   RebuildFileTree();
   SyncFileListToInstance();
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_SetFilter_Click( Button& /*sender*/, bool /*checked*/ )
{
   SetFilterForSelected();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_EditCompleted( Edit& sender )
{
   String text = sender.Text().Trimmed();

   if ( sender == GUI->OutputDirectory_Edit )
      m_instance.p_outputDirectory = text;
   else if ( sender == GUI->OutputFileName_Edit )
      m_instance.p_outputFileName = text;
   else if ( sender == GUI->OverrideFile_Edit )
      m_instance.p_overrideFilePath = text;

   sender.SetText( text );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_Browse_Click( Button& sender, bool /*checked*/ )
{
   if ( sender == GUI->OutputDirectory_Browse_PushButton )
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
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::OpenSettings()
{
   DbgLog( "OpenSettings enter" );
   try
   {
      ABCGSettingsDialog dlg( m_instance );
      DbgLog( "OpenSettings dialog constructed" );
      if ( dlg.Execute() == StdDialogCode::Ok )
      {
         DbgLog( "OpenSettings dialog returned Ok" );
         SaveSettings();
         DbgLog( "OpenSettings SaveSettings done" );
      }
      else
         DbgLog( "OpenSettings dialog returned not-Ok" );
   }
   catch ( ... )
   {
      DbgLogException( "OpenSettings" );
   }
   DbgLog( "OpenSettings exit" );
}

// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorInterface::e_Settings_Click( Button& /*sender*/, bool /*checked*/ )
{
   OpenSettings();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

AstroBinCSVGeneratorInterface::GUIData::GUIData( AstroBinCSVGeneratorInterface& w )
{
   pcl::Font fnt = w.Font();
   int labelWidth = fnt.Width( String( "Output file name:" ) + 'M' );
   int editWidth = 30 * fnt.Width( 'M' );
   int editWidth2 = 20 * fnt.Width( 'M' );

   //
   // Input Files section
   //

   FileList_TreeBox.SetNumberOfColumns( 7 );
   FileList_TreeBox.SetHeaderText( 0, "File Name" );
   FileList_TreeBox.SetHeaderText( 1, "Filter" );
   FileList_TreeBox.SetHeaderText( 2, "Exposure (s)" );
   FileList_TreeBox.SetHeaderText( 3, "Gain" );
   FileList_TreeBox.SetHeaderText( 4, "Temp (C)" );
   FileList_TreeBox.SetHeaderText( 5, "Date" );
   FileList_TreeBox.SetHeaderText( 6, "Type" );
   FileList_TreeBox.DisableRootDecoration();
   FileList_TreeBox.EnableAlternateRowColor();
   FileList_TreeBox.EnableMultipleSelections( true );
   FileList_TreeBox.SetScaledMinSize( 64, 12 );
   FileList_TreeBox.SetToolTip( "<p>Files to process. Add files or a folder below, then use "
      "Set Filter... to assign an AstroBin filter to the selected rows. When the list is "
      "non-empty it takes precedence over the input directory.</p>" );

   AddFiles_PushButton.SetText( "Add Files..." );
   AddFiles_PushButton.SetToolTip( "<p>Add individual FITS/XISF files to the list.</p>" );
   AddFiles_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_AddFiles_Click, w );

   AddDirectory_PushButton.SetText( "Add Folder..." );
   AddDirectory_PushButton.SetToolTip( "<p>Add all FITS/XISF files in a folder to the list.</p>" );
   AddDirectory_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_AddDirectory_Click, w );

   RemoveSelected_PushButton.SetText( "Remove Selected" );
   RemoveSelected_PushButton.SetToolTip( "<p>Remove the selected rows from the list.</p>" );
   RemoveSelected_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_RemoveSelected_Click, w );

   ClearAll_PushButton.SetText( "Clear All" );
   ClearAll_PushButton.SetToolTip( "<p>Clear the entire file list.</p>" );
   ClearAll_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_ClearAll_Click, w );

   SetFilter_PushButton.SetText( "Set Filter..." );
   SetFilter_PushButton.SetToolTip( "<p>Assign an AstroBin filter to the selected rows. "
      "Requires the AstroBin filter database (see the Filters section in Settings...).</p>" );
   SetFilter_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_SetFilter_Click, w );

   FileListButtons_Sizer.SetSpacing( 4 );
   FileListButtons_Sizer.Add( AddFiles_PushButton );
   FileListButtons_Sizer.Add( AddDirectory_PushButton );
   FileListButtons_Sizer.Add( RemoveSelected_PushButton );
   FileListButtons_Sizer.Add( ClearAll_PushButton );
   FileListButtons_Sizer.Add( SetFilter_PushButton );
   FileListButtons_Sizer.AddStretch();

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

   Settings_PushButton.SetText( "Settings..." );
   Settings_PushButton.SetToolTip( "<p>Open the settings dialog with the Site, Equipment, "
      "Sessions, Filters and Keyword Overrides options.</p>" );
   Settings_PushButton.OnClick( (Button::click_event_handler)&AstroBinCSVGeneratorInterface::e_Settings_Click, w );

   Settings_Sizer.SetSpacing( 4 );
   Settings_Sizer.AddStretch();
   Settings_Sizer.Add( Settings_PushButton );

   InputFiles_Section_Sizer.SetMargin( 6 );
   InputFiles_Section_Sizer.SetSpacing( 4 );
   InputFiles_Section_Sizer.Add( FileList_TreeBox );
   InputFiles_Section_Sizer.Add( FileListButtons_Sizer );
   InputFiles_Section_Sizer.Add( OutputDirectory_Sizer );
   InputFiles_Section_Sizer.Add( OutputFileName_Sizer );
   InputFiles_Section_Sizer.Add( OverrideFile_Sizer );
   InputFiles_Section_Sizer.Add( Settings_Sizer );

   InputFiles_Section.SetSizer( InputFiles_Section_Sizer );
   InputFiles_SectionBar.SetTitle( "Input Files" );
   InputFiles_SectionBar.SetSection( InputFiles_Section );

   //
   // Global layout
   //

   Global_Sizer.SetMargin( 8 );
   Global_Sizer.SetSpacing( 4 );

   Global_Sizer.Add( InputFiles_SectionBar );
   Global_Sizer.Add( InputFiles_Section );

   w.SetSizer( Global_Sizer );

   w.EnsureLayoutUpdated();
   w.AdjustToContents();
   w.SetFixedSize();
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorInterface.cpp

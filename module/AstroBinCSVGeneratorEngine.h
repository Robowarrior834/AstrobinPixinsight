//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorEngine.h - Core CSV generation engine
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

#ifndef __AstroBinCSVGeneratorEngine_h
#define __AstroBinCSVGeneratorEngine_h

#include <pcl/String.h>

#include <string>
#include <vector>
#include <map>
#include <limits>
#include <cmath>

namespace pcl
{

// ----------------------------------------------------------------------------
// Constants inherited from the AstroBin CSV Generator script (v1.2.5).
// ----------------------------------------------------------------------------

constexpr int    ABCG_FITS_CARD_SIZE      = 80;    // FITS header card size (bytes)
constexpr double ABCG_FWHM_TO_HFR_FACTOR  = 2.0;   // HFR -> FWHM approximation factor
constexpr double ABCG_DEFAULT_GAP_HOURS   = 5.0;   // default session gap threshold
const char* const ABCG_ASTROBIN_API_BASE  = "https://app.astrobin.com/api/v2/equipment/filter/";

// ----------------------------------------------------------------------------
// JSON value + parser.
//
// PCL 2.10.4 has no JSON support, so a minimal JSON parser/serializer is
// implemented here. Numbers are stored as IEEE 754 doubles, mirroring the
// JavaScript V8 runtime used by the reference script.
// ----------------------------------------------------------------------------

class ABCGJSON
{
public:

   enum Type { NullType, BoolType, NumberType, StringType, ArrayType, ObjectType };

   struct Value
   {
      Type   type = NullType;
      bool   b = false;
      double num = 0;
      std::string str;

      std::vector<Value>                arr;
      std::vector< std::pair<std::string,Value> > obj;

      Value() = default;
      Value( const Value& ) = default;
      Value& operator =( const Value& ) = default;

      bool IsNull() const { return type == NullType; }
      bool IsObject() const { return type == ObjectType; }
      bool IsArray() const { return type == ArrayType; }

      const Value* Find( const char* key ) const;
      const Value* Find( const std::string& key ) const;
   };

   // Parses a JSON document. Returns false on malformed input.
   static bool Parse( const std::string& s, Value& out );

   // Serializes a value with the same rules as JSON.stringify(): compact,
   // no whitespace, object keys in insertion order, numbers shortest form.
   static std::string Serialize( const Value& v );

private:

   struct ParserState
   {
      std::string s;
      size_t pos = 0;
      bool ok = true;
   };

   static void   SkipWs( ParserState& st );
   static bool   ParseValue( ParserState& st, Value& out );
   static bool   ParseObject( ParserState& st, Value& out );
   static bool   ParseArray( ParserState& st, Value& out );
   static bool   ParseString( ParserState& st, std::string& out );
   static bool   ParseNumber( ParserState& st, double& out );
   static bool   ParseLiteral( ParserState& st, const char* literal, bool& out );

   static void   SerializeValue( std::string& s, const Value& v );
   static void   SerializeString( std::string& s, const std::string& str );
};

// ----------------------------------------------------------------------------
// AstroBin CSV Generator engine.
//
// Faithful port of the acquisition.csv generation logic from the
// AstroBinCSVGenerator.js script (v1.2.5), rewritten in original C++.
// ----------------------------------------------------------------------------

class AstroBinCSVGeneratorEngine
{
public:

   // -------------------------------------------------------------------------
   // Structured frame data (equivalent to the JS extractFrameData() result).
   // -------------------------------------------------------------------------

   struct FrameData
   {
      String   filePath;
      String   fileName;

      String   imagetyp;
      bool     isMaster = false;

      double   exposure = 0;
      String   dateObs;
      bool     hasDate = false;
      double   jd = 0;                    // instant used for session detection
      int      lYear = 0, lMonth = 0, lDay = 0, lHour = 0; // JS local date components

      double   xbinning = 1;
      double   ybinning = 1;
      double   gain = 0;
      double   egain = 0;
      double   ccdTemp = 0;

      double   focalLength = 0;
      double   pixelSize = 0;
      double   focalRatio = 0;
      double   imscale = 0;
      double   fwhm = 0;
      double   hfr = 0;

      String   filter;
      bool     hasFilterOverride = false;
      String   filterOverrideId;
      String   filterOverrideLabel;

      String   object;
      String   ra;
      String   dec;
      String   telescope;
      String   camera;

      String   site;
      double   siteLat = 0;
      double   siteLon = 0;
      double   bortle = 0;
      double   sqm = 0;
      double   foctemp = 0;
      String   swcreate;

      int      sessionId = -1;
      String   sessionDate;
   };

   // -------------------------------------------------------------------------
   // Aggregated CSV row (equivalent to the JS aggregateFrames() result).
   // -------------------------------------------------------------------------

   struct AggregateRow
   {
      String   sessionDate;
      String   filter;       // display label
      String   filterCode;   // AstroBin numeric ID, or raw name when unmapped
      double   gain = 0;
      double   xbinning = 1;
      double   exposure = 0;
      String   object;
      int      number = 0;
      double   sensorCooling = 0;
      double   meanFwhm = 0;
      double   meanSqm = 0;
      double   temperature = 0;
      double   fNumber = 0;
      double   bortle = 0;
   };

   // -------------------------------------------------------------------------
   // Processing options (mirror the module MetaParameters and the JS settings).
   // -------------------------------------------------------------------------

   double SessionGapHours    = ABCG_DEFAULT_GAP_HOURS;
   bool   ShiftOvernight     = true;
   bool   UseObservingDate   = false;

   int    DefaultGain        = 0;
   double DefaultTemperature = -10.0;
   String DefaultFilter;
   bool   UseDefaultFilter   = false;

   String SiteName           = "My Site";
   double SiteLatitude       = 0;
   double SiteLongitude      = 0;
   double SiteElevation      = 0;
   int    Bortle             = 4;
   double SQM                = 21.0;
   double FocalLength        = 540.0;
   double PixelSize          = 3.0;
   double FocalRatio         = 5.0;

   // JSON documents, as stored in the module parameters.
   String FilterMapJSON;       // object: filter name -> AstroBin ID
   String KeywordOverridesJSON;// object: canonical keyword -> source keyword
   String FilterDatabasePath;  // empty = default ~/PixInsight/AstroBinFilters.json

   // -------------------------------------------------------------------------
   // Filter database
   // -------------------------------------------------------------------------

   size_type LoadFilterDatabase();      // returns number of filters loaded
   bool DownloadFilterDatabase();       // fetch from AstroBin API + save cache
   bool SaveFilterDatabase();           // persist m_filters + lastUpdated to cache
   size_type FilterCount() const { return m_filters.size(); }
   bool HasFilterDatabase() const { return !m_filters.empty(); }
   String FilterLastUpdated() const { return m_lastUpdated; }
   String ResolveDatabasePath() const;

   // Returns the current UTC time as an ISO 8601 string with milliseconds,
   // equivalent to JavaScript's new Date().toISOString().
   static String NowIsoString();
   // -------------------------------------------------------------------------
   // Main entry point: scans input directory, extracts frames, aggregates and
   // writes the acquisition CSV. Returns true on success.
   // -------------------------------------------------------------------------

   bool Generate( const String& inputDirectory,
                  const String& outputDirectory,
                  const String& outputFileName,
                  bool recursive,
                  const String& overrideFilePath );

   // Parses the configured JSON documents and populates internal tables.
   void Configure();

   // Returns the aggregated rows produced by the last Generate() call. This
   // allows the caller to print the CSV content to the PixInsight console.
   const std::vector<AggregateRow>& Results() const
   {
      return m_results;
   }

private:

   // -------------------------------------------------------------------------
   // Filter database entry.
   // -------------------------------------------------------------------------

   struct FilterEntry
   {
      String          id;               // AstroBin numeric ID (as string)
      String          name;             // display name
      String          searchFriendlyName;// searchable name
      String          brandName;        // manufacturer
      ABCGJSON::Value raw;              // full JSON object as fetched/saved
   };

   std::vector<FilterEntry> m_filters;
   String m_lastUpdated;
   String m_dbPath;

   // Aggregated rows produced by the last Generate() call.
   std::vector<AggregateRow> m_results;

   // Configured filter map: ordered (name -> ID string) pairs from FilterMapJSON.
   std::vector< std::pair<String,String> > m_filterMap;
   // Configured keyword overrides: ordered (canonical -> source) pairs.
   std::vector< std::pair<String,String> > m_keywordOverrides;

   // -------------------------------------------------------------------------
   // File/header scanning
   // -------------------------------------------------------------------------

   bool CollectFiles( const String& dir, bool recursive,
                      std::vector<String>& files ) const;

   static bool HasSupportedExtension( const String& path );

   static bool ReadFITSHeaders( const String& filePath,
                                std::map<std::string,ABCGJSON::Value>& keywords );
   static bool ReadXISFHeaders( const String& filePath,
                                std::map<std::string,ABCGJSON::Value>& keywords );

   // Performs a synchronous HTTP GET and returns the response body, or an
   // empty string on failure. Mirrors the JS httpGet() helper.
   static String HttpGet( const String& url );

   // -------------------------------------------------------------------------
   // Frame extraction
   // -------------------------------------------------------------------------

   FrameData ExtractFrameData( const std::map<std::string,ABCGJSON::Value>& rawKeywords,
                               const String& filePath ) const;

   // -------------------------------------------------------------------------
   // Session detection + aggregation
   // -------------------------------------------------------------------------

   static void DetectSessions( std::vector<FrameData>& frames,
                               double gapHours, bool shiftSessions );
   std::vector<AggregateRow> AggregateFrames( const std::vector<FrameData>& frames ) const;

   // -------------------------------------------------------------------------
   // Filter mapping
   // -------------------------------------------------------------------------

   String MapFilter( const String& name ) const;
   String SearchFilterDatabase( const String& name ) const;

   // -------------------------------------------------------------------------
   // Override file parsing (CSV: filename,filter_name,filter_id)
   // -------------------------------------------------------------------------

   static bool LoadOverrideFile( const String& overrideFilePath,
                                 std::vector< std::pair<String, std::pair<String,String> > >& overrides );

   // -------------------------------------------------------------------------
   // CSV generation
   // -------------------------------------------------------------------------

   static bool WriteCSV( const std::vector<AggregateRow>& rows, const String& outputPath );

public:

   static double JsRound( double v ) { return ( std::isnan( v ) || std::isinf( v ) ) ? v : std::floor( v + 0.5 ); }       // JS Math.round()
   static double JsRound2( double v ) { return ( std::isnan( v ) || std::isinf( v ) ) ? v : std::floor( v*100.0 + 0.5 )/100.0; } // Math.round(x*100)/100

   // -------------------------------------------------------------------------
   // Value conversion helpers (mirror JavaScript Number()/String() semantics)
   // -------------------------------------------------------------------------

   static bool     JsNumber( const std::string& s, double& out ); // JS Number(string)
   static double   JsNumber( const ABCGJSON::Value& v );          // JS Number(value)

private:

   static std::string JsString( double v );                      // JS String(number)
   static bool     JsTruthy( const ABCGJSON::Value& v );          // JS || semantics

   static const ABCGJSON::Value* FirstTruthy( const std::map<std::string,ABCGJSON::Value>& kw,
                                              std::initializer_list<const char*> keys );

   // -------------------------------------------------------------------------
   // Date/time parsing (ISO 8601, JS Date() compatible enough for FITS/XISF)
   // -------------------------------------------------------------------------

   static bool ParseDateTime( const String& s, bool& hasTz, int& tzMinutes,
                              int& year, int& month, int& day,
                              int& hour, int& minute, double& seconds );

   static double JulianDate( int year, int month, int day,
                             int hour, int minute, double seconds );
   static void   CalendarDate( double jd, int& year, int& month, int& day );
   static void   AddDays( int& year, int& month, int& day, int days );
   static String FormatDate( int year, int month, int day );

};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __AstroBinCSVGeneratorEngine_h

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorEngine.h

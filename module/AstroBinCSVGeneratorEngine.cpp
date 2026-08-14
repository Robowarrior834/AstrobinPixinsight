//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.10.4
// ----------------------------------------------------------------------------
// AstroBin CSV Generator Process Module Version 1.2.5
// ----------------------------------------------------------------------------
// AstroBinCSVGeneratorEngine.cpp - Core CSV generation engine
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

#include "AstroBinCSVGeneratorEngine.h"

#include <pcl/Console.h>
#include <pcl/Control.h>
#include <pcl/File.h>
#include <pcl/MetaModule.h>
#include <pcl/NetworkTransfer.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <charconv>
#include <chrono>

namespace pcl
{

// ----------------------------------------------------------------------------
// Anonymous-namespace helpers
// ----------------------------------------------------------------------------

namespace
{

// ----------------------------------------------------------------------------
// Trims ASCII whitespace (matches JavaScript String.prototype.trim()).
// ----------------------------------------------------------------------------

std::string Trim( std::string s )
{
   size_t b = s.find_first_not_of( " \t\r\n\f\v" );
   if ( b == std::string::npos )
      return std::string();
   size_t e = s.find_last_not_of( " \t\r\n\f\v" );
   return s.substr( b, e - b + 1 );
}

// ----------------------------------------------------------------------------
// Returns the shortest round-trip decimal representation of a double, using
// the same digit generation rules as JavaScript's Number::toString(). This is
// required to reproduce the JS script output byte for byte.
// ----------------------------------------------------------------------------

std::string shortest_double_string( double v )
{
   if ( std::isnan( v ) )
      return "NaN";
   if ( std::isinf( v ) )
      return v < 0 ? "-Infinity" : "Infinity";
   if ( v == 0.0 )
      return "0";

   bool negative = std::signbit( v );
   double a = std::fabs( v );

   // Shortest round-trip representation via std::to_chars (locale-independent).
   char buf[64];
   auto res = std::to_chars( buf, buf + sizeof( buf ), a, std::chars_format::general );
   if ( res.ec != std::errc() )
      return "0";
   std::string digits( buf, res.ptr );

   // digits is a %g-like string: either fixed notation ("300", "0.0001",
   // "123.45") or exponential notation ("1e+06", "1e-07").
   size_t epos = digits.find_first_of( "eE" );

   if ( epos == std::string::npos )
      return negative ? '-' + digits : digits;

   // Exponential form from std::to_chars: mantissa + e±XX.
   std::string mant = digits.substr( 0, epos );
   int exp10 = std::atoi( digits.c_str() + epos + 1 );

   // Significant digits (mantissa has exactly one digit before the decimal).
   std::string sig;
   for ( char c : mant )
      if ( c != '.' )
         sig += c;

   std::string out;
   if ( negative )
      out += '-';

   if ( exp10 >= 21 || exp10 <= -7 )
   {
      // d[.ddd]e±X
      out += sig[0];
      if ( sig.size() > 1 )
      {
         out += '.';
         out.append( sig, 1, std::string::npos );
      }
      out += 'e';
      out += exp10 >= 0 ? '+' : '-';
      out += std::to_string( std::abs( exp10 ) );
   }
   else if ( exp10 >= 0 )
   {
      out += sig.substr( 0, size_t( exp10 ) + 1 );
      for ( size_t i = sig.size(); i <= size_t( exp10 ); i++ )
         out += '0';
      if ( sig.size() > size_t( exp10 ) + 1 )
      {
         out += '.';
         out.append( sig, size_t( exp10 ) + 1, std::string::npos );
      }
   }
   else
   {
      out += "0.";
      for ( int i = -1; i > exp10; i-- )
         out += '0';
      out += sig;
   }

   return out;
}

// ----------------------------------------------------------------------------
// Converts a parsed JSON value to a string using JavaScript String() rules
// for the value types that occur in FITS/XISF headers.
// ----------------------------------------------------------------------------

String ValueToString( const ABCGJSON::Value& v )
{
   switch ( v.type )
   {
      case ABCGJSON::StringType: return String( v.str.c_str() );
      case ABCGJSON::NumberType: return String( shortest_double_string( v.num ).c_str() );
      case ABCGJSON::BoolType:   return v.b ? String( "true" ) : String( "false" );
      default:                   return String();
   }
}

// ----------------------------------------------------------------------------
// FITS header value parsing. Mirrors the value handling of the JS
// readFITSHeaders() function.
//
// allowBoolean: the standard FITS keyword path recognizes 'T'/'F' as booleans;
// the HIERARCH path in the JS script does not, so it is replicated separately.
// ----------------------------------------------------------------------------

ABCGJSON::Value ParseFITSValueString( const std::string& raw, bool allowBoolean )
{
   ABCGJSON::Value out;
   out.type = ABCGJSON::StringType;

   std::string v = Trim( raw );
   if ( v.empty() )
      return out;

   if ( v[0] == '\'' )
   {
      if ( v.size() >= 2 && v.back() == '\'' )
         v = v.substr( 1, v.size() - 2 );
      else
         v = v.substr( 1 );
      v = Trim( v );
      out.str = v;
      return out;
   }

   if ( allowBoolean )
   {
      if ( v == "T" ) { out.type = ABCGJSON::BoolType; out.b = true;  return out; }
      if ( v == "F" ) { out.type = ABCGJSON::BoolType; out.b = false; return out; }
   }

   double num;
   if ( AstroBinCSVGeneratorEngine::JsNumber( v, num ) )
   {
      out.type = ABCGJSON::NumberType;
      out.num = num;
      return out;
   }

   out.str = v;
   return out;
}

// ----------------------------------------------------------------------------
// FITS inline comment stripping. A '/' outside of a single-quoted string
// introduces a comment. Mirrors the JS findFITSComment() helper.
// ----------------------------------------------------------------------------

std::string StripFITSComment( const std::string& s )
{
   bool inString = false;
   for ( size_t i = 0; i < s.size(); i++ )
   {
      if ( s[i] == '\'' )
         inString = !inString;
      else if ( s[i] == '/' && !inString )
         return Trim( s.substr( 0, i ) );
   }
   return s;
}

// ----------------------------------------------------------------------------
// Extracts the value of an XML attribute: name="value". Returns an empty
// string if the attribute is not present. Mirrors the JS regex match.
// ----------------------------------------------------------------------------

std::string GetXMLAttr( const std::string& s, const std::string& attr )
{
   std::string pat = attr + "=\"";
   size_t p = s.find( pat );
   if ( p == std::string::npos )
      return std::string();
   p += pat.size();
   size_t q = s.find( '"', p );
   if ( q == std::string::npos )
      return std::string();
   return s.substr( p, q - p );
}

// ----------------------------------------------------------------------------
// IMAGE_TYPE_MAP from the JS script. Normalizes the many IMAGETYP variants
// written by different capture software to a small set of canonical types.
// ----------------------------------------------------------------------------

struct ImageTypeEntry
{
   const char* raw;
   const char* canonical;
};

const ImageTypeEntry kImageTypeMap[] =
{
   { "LIGHT", "LIGHT" },
   { "LIGHT FRAME", "LIGHT" },
   { "LIGHTFRAME", "LIGHT" },
   { "LIGHTS", "LIGHT" },
   { "FLAT", "FLAT" },
   { "FLAT FRAME", "FLAT" },
   { "FLATFRAME", "FLAT" },
   { "FLATS", "FLAT" },
   { "DARK", "DARK" },
   { "DARK FRAME", "DARK" },
   { "DARKFRAME", "DARK" },
   { "DARKS", "DARK" },
   { "BIAS", "BIAS" },
   { "BIAS FRAME", "BIAS" },
   { "BIASFRAME", "BIAS" },
   { "BIASES", "BIAS" },
   { "DARKFLAT", "DARKFLAT" },
   { "DARK FLAT", "DARKFLAT" },
   { "DARKFLAT FRAME", "DARKFLAT" },
   { "MASTERLIGHT", "MASTERLIGHT" },
   { "MASTER LIGHT", "MASTERLIGHT" },
   { "MASTERFLAT", "MASTERFLAT" },
   { "MASTER FLAT", "MASTERFLAT" },
   { "MASTERDARK", "MASTERDARK" },
   { "MASTER DARK", "MASTERDARK" },
   { "MASTERBIAS", "MASTERBIAS" },
   { "MASTER BIAS", "MASTERBIAS" },
   { "MASTERDARKFLAT", "MASTERDARKFLAT" },
   { "MASTER DARKFLAT", "MASTERDARKFLAT" }
};

String NormalizeImageType( const String& rawType )
{
   if ( rawType.IsEmpty() )
      return "LIGHT";
   String t = rawType.Trimmed();
   t.ToUppercase();

   for ( const ImageTypeEntry& e : kImageTypeMap )
      if ( t == e.raw )
         return e.canonical;

   if ( t.Contains( "LIGHT" ) )
      return "LIGHT";
   if ( t.Contains( "FLAT" ) )
      return "FLAT";
   if ( t.Contains( "DARK" ) && t.Contains( "FLAT" ) )
      return "DARKFLAT";
   if ( t.Contains( "DARK" ) )
      return "DARK";
   if ( t.Contains( "BIAS" ) )
      return "BIAS";
   return "LIGHT";
}

// ----------------------------------------------------------------------------
// Converts a UTC Julian date to local wall-clock components, matching the
// JavaScript local-time getters (getFullYear/getMonth/getDate/getHours).
// ----------------------------------------------------------------------------

void ConvertUtcToLocal( double jdUtc, int& year, int& month, int& day, int& hour )
{
   double epoch = ( jdUtc - 2440587.5 ) * 86400.0;
   std::time_t t = std::time_t( std::floor( epoch ) );
   std::tm tmv;
#if defined( __PCL_WINDOWS )
   localtime_s( &tmv, &t );
#else
   localtime_r( &t, &tmv );
#endif
   year  = tmv.tm_year + 1900;
   month = tmv.tm_mon + 1;
   day   = tmv.tm_mday;
   hour  = tmv.tm_hour;
}

// ----------------------------------------------------------------------------
// Network transfer sink. NetworkTransfer download event handlers must be
// member functions of a Control-derived class.
// ----------------------------------------------------------------------------

class ABCGDownloadSink : public Control
{
public:

   IsoString data;

   bool e_Download( NetworkTransfer& sender, const void* buffer, fsize_type size )
   {
      data.Append( (const char*)buffer, size );
      return true;
   }
};

} // anon namespace

// ----------------------------------------------------------------------------
// ABCGJSON
// ----------------------------------------------------------------------------

const ABCGJSON::Value* ABCGJSON::Value::Find( const char* key ) const
{
   if ( IsObject() )
      for ( const auto& kv : obj )
         if ( kv.first == key )
            return &kv.second;
   return nullptr;
}

const ABCGJSON::Value* ABCGJSON::Value::Find( const std::string& key ) const
{
   return Find( key.c_str() );
}

void ABCGJSON::SkipWs( ParserState& st )
{
   while ( st.pos < st.s.size() )
   {
      char c = st.s[st.pos];
      if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' )
         break;
      st.pos++;
   }
}

bool ABCGJSON::Parse( const std::string& s, Value& out )
{
   ParserState st;
   st.s = s;
   SkipWs( st );
   if ( !ParseValue( st, out ) )
      return false;
   SkipWs( st );
   return st.pos == st.s.size();
}

bool ABCGJSON::ParseValue( ParserState& st, Value& out )
{
   if ( st.pos >= st.s.size() )
      return false;
   char c = st.s[st.pos];
   switch ( c )
   {
      case '{':
         return ParseObject( st, out );
      case '[':
         return ParseArray( st, out );
      case '"':
         {
            out.type = StringType;
            return ParseString( st, out.str );
         }
      case 't':
         return ParseLiteral( st, "true", out.b ) ? ( out.type = BoolType, true ) : false;
      case 'f':
         return ParseLiteral( st, "false", out.b ) ? ( out.type = BoolType, true ) : false;
      case 'n':
         {
            bool dummy;
            if ( !ParseLiteral( st, "null", dummy ) )
               return false;
            out = Value();
            return true;
         }
      default:
         {
            out.type = NumberType;
            return ParseNumber( st, out.num );
         }
   }
}

bool ABCGJSON::ParseObject( ParserState& st, Value& out )
{
   out = Value();
   out.type = ObjectType;
   st.pos++; // '{'
   SkipWs( st );
   if ( st.pos < st.s.size() && st.s[st.pos] == '}' )
   {
      st.pos++;
      return true;
   }
   for ( ;; )
   {
      SkipWs( st );
      std::string key;
      if ( !ParseString( st, key ) )
         return false;
      SkipWs( st );
      if ( st.pos >= st.s.size() || st.s[st.pos] != ':' )
         return false;
      st.pos++;
      SkipWs( st );
      Value v;
      if ( !ParseValue( st, v ) )
         return false;
      out.obj.push_back( { std::move( key ), std::move( v ) } );
      SkipWs( st );
      if ( st.pos >= st.s.size() )
         return false;
      char c = st.s[st.pos++];
      if ( c == ',' )
         continue;
      if ( c == '}' )
         return true;
      return false;
   }
}

bool ABCGJSON::ParseArray( ParserState& st, Value& out )
{
   out = Value();
   out.type = ArrayType;
   st.pos++; // '['
   SkipWs( st );
   if ( st.pos < st.s.size() && st.s[st.pos] == ']' )
   {
      st.pos++;
      return true;
   }
   for ( ;; )
   {
      SkipWs( st );
      Value v;
      if ( !ParseValue( st, v ) )
         return false;
      out.arr.push_back( std::move( v ) );
      SkipWs( st );
      if ( st.pos >= st.s.size() )
         return false;
      char c = st.s[st.pos++];
      if ( c == ',' )
         continue;
      if ( c == ']' )
         return true;
      return false;
   }
}

bool ABCGJSON::ParseString( ParserState& st, std::string& out )
{
   if ( st.pos >= st.s.size() || st.s[st.pos] != '"' )
      return false;
   st.pos++;
   out.clear();
   while ( st.pos < st.s.size() )
   {
      unsigned char c = (unsigned char)st.s[st.pos++];
      if ( c == '"' )
         return true;
      if ( c == '\\' )
      {
         if ( st.pos >= st.s.size() )
            return false;
         char e = st.s[st.pos++];
         switch ( e )
         {
            case '"':  out += '"';  break;
            case '\\': out += '\\'; break;
            case '/':  out += '/';  break;
            case 'b':  out += '\b'; break;
            case 'f':  out += '\f'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            case 'u':
               {
                  if ( st.pos + 4 > st.s.size() )
                     return false;
                  unsigned cp = 0;
                  for ( int i = 0; i < 4; i++ )
                  {
                     char h = st.s[st.pos++];
                     cp <<= 4;
                     if ( h >= '0' && h <= '9' )      cp |= unsigned( h - '0' );
                     else if ( h >= 'a' && h <= 'f' ) cp |= unsigned( h - 'a' + 10 );
                     else if ( h >= 'A' && h <= 'F' ) cp |= unsigned( h - 'A' + 10 );
                     else return false;
                  }
                  if ( cp >= 0xD800 && cp <= 0xDBFF && st.pos + 6 <= st.s.size() &&
                       st.s[st.pos] == '\\' && st.s[st.pos+1] == 'u' )
                  {
                     st.pos += 2;
                     unsigned lo = 0;
                     for ( int i = 0; i < 4; i++ )
                     {
                        char h = st.s[st.pos++];
                        lo <<= 4;
                        if ( h >= '0' && h <= '9' )      lo |= unsigned( h - '0' );
                        else if ( h >= 'a' && h <= 'f' ) lo |= unsigned( h - 'a' + 10 );
                        else if ( h >= 'A' && h <= 'F' ) lo |= unsigned( h - 'A' + 10 );
                        else return false;
                     }
                     cp = 0x10000 + ( ( cp - 0xD800 ) << 10 ) + ( lo - 0xDC00 );
                  }
                  if ( cp < 0x80 )
                     out += char( cp );
                  else if ( cp < 0x800 )
                  {
                     out += char( 0xC0 | ( cp >> 6 ) );
                     out += char( 0x80 | ( cp & 0x3F ) );
                  }
                  else if ( cp < 0x10000 )
                  {
                     out += char( 0xE0 | ( cp >> 12 ) );
                     out += char( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
                     out += char( 0x80 | ( cp & 0x3F ) );
                  }
                  else
                  {
                     out += char( 0xF0 | ( cp >> 18 ) );
                     out += char( 0x80 | ( ( cp >> 12 ) & 0x3F ) );
                     out += char( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
                     out += char( 0x80 | ( cp & 0x3F ) );
                  }
               }
               break;
            default:
               return false;
         }
      }
      else
         out += char( c );
   }
   return false;
}

bool ABCGJSON::ParseNumber( ParserState& st, double& out )
{
   size_t start = st.pos;
   while ( st.pos < st.s.size() )
   {
      char c = st.s[st.pos];
      if ( ( c >= '0' && c <= '9' ) || c == '-' || c == '+' || c == '.' ||
           c == 'e' || c == 'E' )
         st.pos++;
      else
         break;
   }
   std::string tok = st.s.substr( start, st.pos - start );
   if ( tok.empty() )
      return false;
   char* end = nullptr;
   out = std::strtod( tok.c_str(), &end );
   if ( end != tok.c_str() + tok.size() )
      return false;
   return true;
}

bool ABCGJSON::ParseLiteral( ParserState& st, const char* literal, bool& out )
{
   size_t n = std::strlen( literal );
   if ( st.pos + n > st.s.size() )
      return false;
   if ( st.s.compare( st.pos, n, literal ) != 0 )
      return false;
   st.pos += n;
   out = ( literal[0] == 't' );
   return true;
}

std::string ABCGJSON::Serialize( const Value& v )
{
   std::string s;
   SerializeValue( s, v );
   return s;
}

void ABCGJSON::SerializeValue( std::string& s, const Value& v )
{
   switch ( v.type )
   {
      case NullType:
         s += "null";
         break;
      case BoolType:
         s += v.b ? "true" : "false";
         break;
      case NumberType:
         if ( std::isnan( v.num ) || std::isinf( v.num ) )
            s += "null";
         else
            s += shortest_double_string( v.num );
         break;
      case StringType:
         SerializeString( s, v.str );
         break;
      case ArrayType:
         s += '[';
         for ( size_t i = 0; i < v.arr.size(); i++ )
         {
            if ( i )
               s += ',';
            SerializeValue( s, v.arr[i] );
         }
         s += ']';
         break;
      case ObjectType:
         s += '{';
         for ( size_t i = 0; i < v.obj.size(); i++ )
         {
            if ( i )
               s += ',';
            SerializeString( s, v.obj[i].first );
            s += ':';
            SerializeValue( s, v.obj[i].second );
         }
         s += '}';
         break;
   }
}

void ABCGJSON::SerializeString( std::string& s, const std::string& str )
{
   s += '"';
   for ( unsigned char c : str )
   {
      switch ( c )
      {
         case '"':  s += "\\\""; break;
         case '\\': s += "\\\\"; break;
         case '\b': s += "\\b";  break;
         case '\f': s += "\\f";  break;
         case '\n': s += "\\n";  break;
         case '\r': s += "\\r";  break;
         case '\t': s += "\\t";  break;
         default:
            if ( c < 0x20 )
            {
               char buf[8];
               std::snprintf( buf, sizeof( buf ), "\\u%04x", c );
               s += buf;
            }
            else
               s += char( c );
      }
   }
   s += '"';
}

// ----------------------------------------------------------------------------
// JS value conversion helpers
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::JsNumber( const std::string& s, double& out )
{
   const char* b = s.c_str();
   const char* e = b + s.size();
   while ( b < e && std::isspace( (unsigned char)*b ) )
      b++;
   while ( e > b && std::isspace( (unsigned char)e[-1] ) )
      e--;
   if ( b == e )
   {
      out = 0; // JS Number("") === 0
      return true;
   }
   std::string t( b, e );
   if ( t[0] == '+' )
      t.erase( t.begin() );
   if ( t.empty() )
   {
      out = 0;
      return true;
   }
   double d = 0;
   auto res = std::from_chars( t.c_str(), t.c_str() + t.size(), d );
   if ( res.ec == std::errc() && res.ptr == t.c_str() + t.size() )
   {
      out = d;
      return true;
   }
   if ( t.size() > 2 && t[0] == '0' && ( t[1] == 'x' || t[1] == 'X' ) )
   {
      char* ep = nullptr;
      unsigned long long h = std::strtoull( t.c_str(), &ep, 16 );
      if ( ep == t.c_str() + t.size() )
      {
         out = double( h );
         return true;
      }
   }
   if ( t == "Infinity" || t == "-Infinity" )
   {
      out = ( t[0] == '-' ) ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
      return true;
   }
   return false;
}

double AstroBinCSVGeneratorEngine::JsNumber( const ABCGJSON::Value& v )
{
   switch ( v.type )
   {
      case ABCGJSON::NullType:   return 0;
      case ABCGJSON::BoolType:   return v.b ? 1 : 0;
      case ABCGJSON::NumberType: return v.num;
      case ABCGJSON::StringType:
         {
            double out;
            if ( JsNumber( v.str, out ) )
               return out;
         }
         break;
   }
   return std::numeric_limits<double>::quiet_NaN();
}

std::string AstroBinCSVGeneratorEngine::JsString( double v )
{
   return shortest_double_string( v );
}

bool AstroBinCSVGeneratorEngine::JsTruthy( const ABCGJSON::Value& v )
{
   switch ( v.type )
   {
      case ABCGJSON::NullType:   return false;
      case ABCGJSON::BoolType:   return v.b;
      case ABCGJSON::NumberType: return v.num != 0 && !std::isnan( v.num );
      case ABCGJSON::StringType: return !v.str.empty();
      default:                   return true;
   }
}

const ABCGJSON::Value* AstroBinCSVGeneratorEngine::FirstTruthy(
   const std::map<std::string,ABCGJSON::Value>& kw,
   std::initializer_list<const char*> keys )
{
   for ( const char* key : keys )
   {
      auto it = kw.find( key );
      if ( it != kw.end() && JsTruthy( it->second ) )
         return &it->second;
   }
   return nullptr;
}

// ----------------------------------------------------------------------------
// Date/time helpers
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::ParseDateTime( const String& s, bool& hasTz, int& tzMinutes,
   int& year, int& month, int& day, int& hour, int& minute, double& seconds )
{
   std::string t = s.ToIsoString().c_str();
   size_t p = 0;

   auto readDigits = [&]( int n ) -> int
   {
      int v = 0;
      for ( int i = 0; i < n; i++ )
      {
         if ( p >= t.size() || t[p] < '0' || t[p] > '9' )
            return -1;
         v = v*10 + ( t[p] - '0' );
         p++;
      }
      return v;
   };

   year = readDigits( 4 );
   if ( year < 0 )
      return false;
   if ( p >= t.size() || t[p] != '-' )
      return false;
   p++;
   month = readDigits( 2 );
   if ( month < 0 || month < 1 || month > 12 )
      return false;
   if ( p >= t.size() || t[p] != '-' )
      return false;
   p++;
   day = readDigits( 2 );
   if ( day < 0 || day < 1 || day > 31 )
      return false;

   hour = minute = 0;
   seconds = 0;
   hasTz = false;
   tzMinutes = 0;

   if ( p < t.size() && ( t[p] == 'T' || t[p] == ' ' ) )
   {
      p++;
      hour = readDigits( 2 );
      if ( hour < 0 || hour > 23 )
         return false;
      if ( p < t.size() && t[p] == ':' )
      {
         p++;
         minute = readDigits( 2 );
         if ( minute < 0 || minute > 59 )
            return false;
      }
      if ( p < t.size() && t[p] == ':' )
      {
         p++;
         int sec = readDigits( 2 );
         if ( sec < 0 || sec > 60 )
            return false;
         seconds = sec;
         if ( p < t.size() && t[p] == '.' )
         {
            p++;
            double frac = 0;
            double scale = 0.1;
            while ( p < t.size() && t[p] >= '0' && t[p] <= '9' )
            {
               frac += ( t[p] - '0' ) * scale;
               scale *= 0.1;
               p++;
            }
            seconds += frac;
         }
      }
   }

   if ( p < t.size() )
   {
      if ( t[p] == 'Z' || t[p] == 'z' )
      {
         hasTz = true;
         tzMinutes = 0;
         p++;
      }
      else if ( t[p] == '+' || t[p] == '-' )
      {
         int sign = ( t[p] == '-' ) ? -1 : 1;
         p++;
         int tzH = readDigits( 2 );
         if ( tzH < 0 || tzH > 23 )
            return false;
         int tzM = 0;
         if ( p < t.size() && t[p] == ':' )
         {
            p++;
            tzM = readDigits( 2 );
            if ( tzM < 0 || tzM > 59 )
               return false;
         }
         hasTz = true;
         tzMinutes = sign * ( tzH*60 + tzM );
      }
   }

   while ( p < t.size() && std::isspace( (unsigned char)t[p] ) )
      p++;
   return p == t.size();
}

double AstroBinCSVGeneratorEngine::JulianDate( int year, int month, int day,
   int hour, int minute, double seconds )
{
   int y = year;
   int m = month;
   if ( m <= 2 )
   {
      y--;
      m += 12;
   }
   int a = y / 100;
   int b = 2 - a + a / 4;
   double jd = int( 365.25*( y + 4716 ) ) + int( 30.6001*( m + 1 ) ) + day + b - 1524.5;
   jd += ( hour + ( minute + seconds/60.0 )/60.0 )/24.0;
   return jd;
}

void AstroBinCSVGeneratorEngine::CalendarDate( double jd, int& year, int& month, int& day )
{
   double z = std::floor( jd + 0.5 );
   double a = z;
   if ( z >= 2299161.0 )
   {
      double alpha = std::floor( ( z - 1867216.25 )/36524.25 );
      a = z + 1 + alpha - std::floor( alpha/4 );
   }
   double b = a + 1524;
   double c = std::floor( ( b - 122.1 )/365.25 );
   double d = std::floor( 365.25*c );
   double e = std::floor( ( b - d )/30.6001 );
   day   = int( b - d - std::floor( 30.6001*e ) );
   month = int( e - 1 - 12*std::floor( e/14 ) );
   year  = int( c - 4715 - std::floor( ( 7 + month )/10 ) );
}

void AstroBinCSVGeneratorEngine::AddDays( int& year, int& month, int& day, int days )
{
   static const int kDaysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
   auto isLeap = []( int y )
   {
      return ( y%4 == 0 && y%100 != 0 ) || ( y%400 == 0 );
   };
   if ( days < 0 )
   {
      while ( days++ < 0 )
      {
         day--;
         if ( day < 1 )
         {
            month--;
            if ( month < 1 )
            {
               month = 12;
               year--;
            }
            day = kDaysInMonth[month-1] + ( month == 2 && isLeap( year ) ? 1 : 0 );
         }
      }
   }
   else
   {
      while ( days-- > 0 )
      {
         int dim = kDaysInMonth[month-1] + ( month == 2 && isLeap( year ) ? 1 : 0 );
         day++;
         if ( day > dim )
         {
            day = 1;
            month++;
            if ( month > 12 )
            {
               month = 1;
               year++;
            }
         }
      }
   }
}

String AstroBinCSVGeneratorEngine::FormatDate( int year, int month, int day )
{
   return String().Format( "%04d-%02d-%02d", year, month, day );
}

String AstroBinCSVGeneratorEngine::NowIsoString()
{
   using namespace std::chrono;
   system_clock::time_point now = system_clock::now();
   std::time_t t = system_clock::to_time_t( now );
   long ms = duration_cast<milliseconds>( now.time_since_epoch() ).count() % 1000;
   std::tm tmv;
#if defined( __PCL_WINDOWS )
   gmtime_s( &tmv, &t );
#else
   gmtime_r( &t, &tmv );
#endif
   return String().Format( "%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ",
      tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
      tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms );
}

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorEngine::Configure()
{
   m_filterMap.clear();
   m_keywordOverrides.clear();

   ABCGJSON::Value root;
   std::string fm = FilterMapJSON.ToIsoString().c_str();
   if ( ABCGJSON::Parse( fm, root ) && root.IsObject() )
      for ( const auto& kv : root.obj )
         m_filterMap.push_back( { String( kv.first.c_str() ), ValueToString( kv.second ) } );

   ABCGJSON::Value kor;
   std::string ko = KeywordOverridesJSON.ToIsoString().c_str();
   if ( ABCGJSON::Parse( ko, kor ) && kor.IsObject() )
      for ( const auto& kv : kor.obj )
         m_keywordOverrides.push_back( { String( kv.first.c_str() ), ValueToString( kv.second ) } );
}

// ----------------------------------------------------------------------------
// Filter database
// ----------------------------------------------------------------------------

String AstroBinCSVGeneratorEngine::ResolveDatabasePath() const
{
   if ( !FilterDatabasePath.IsEmpty() )
      return FilterDatabasePath;
   return File::HomeDirectory() + "/PixInsight/AstroBinFilters.json";
}

size_type AstroBinCSVGeneratorEngine::LoadFilterDatabase()
{
   m_filters.clear();
   m_lastUpdated.Clear();
   m_dbPath = ResolveDatabasePath();

   Console c;
   c.WriteLn( "  Filter DB path: " + m_dbPath );
   c.WriteLn( "  Filter DB exists: " + String( File::Exists( m_dbPath ) ? "true" : "false" ) );

   if ( !File::Exists( m_dbPath ) )
      return 0;

   try
   {
      IsoString content = File::ReadTextFile( m_dbPath );
      if ( content.IsEmpty() )
         return 0;

      ABCGJSON::Value root;
      std::string text = content.c_str();
      if ( !ABCGJSON::Parse( text, root ) )
         return 0;

      const ABCGJSON::Value* filters = root.Find( "filters" );
      if ( filters == nullptr || !filters->IsArray() )
         return 0;

      const ABCGJSON::Value* lu = root.Find( "lastUpdated" );
      if ( lu != nullptr && lu->type == ABCGJSON::StringType )
         m_lastUpdated = String( lu->str.c_str() );

      for ( const ABCGJSON::Value& el : filters->arr )
      {
         FilterEntry fe;
         fe.raw = el;
         const ABCGJSON::Value* id = el.Find( "id" );
         if ( id != nullptr )
            fe.id = ValueToString( *id );
         const ABCGJSON::Value* name = el.Find( "name" );
         if ( name != nullptr )
            fe.name = ValueToString( *name );
         const ABCGJSON::Value* sn = el.Find( "searchFriendlyName" );
         if ( sn != nullptr )
            fe.searchFriendlyName = ValueToString( *sn );
         const ABCGJSON::Value* bn = el.Find( "brandName" );
         if ( bn != nullptr )
            fe.brandName = ValueToString( *bn );
         if ( fe.searchFriendlyName.IsEmpty() )
            fe.searchFriendlyName = fe.name;
         m_filters.push_back( fe );
      }
   }
   catch ( ... )
   {
      return 0;
   }

   c.WriteLn( "  Filter DB loaded: " + String( m_filters.size() ) + " filters" );
   return m_filters.size();
}

bool AstroBinCSVGeneratorEngine::SaveFilterDatabase()
{
   ABCGJSON::Value root;
   root.type = ABCGJSON::ObjectType;

   ABCGJSON::Value arr;
   arr.type = ABCGJSON::ArrayType;
   for ( const FilterEntry& fe : m_filters )
      arr.arr.push_back( fe.raw );
   root.obj.push_back( { "filters", arr } );

   ABCGJSON::Value lu;
   lu.type = ABCGJSON::StringType;
   lu.str = m_lastUpdated.ToIsoString().c_str();
   root.obj.push_back( { "lastUpdated", lu } );

   std::string json = ABCGJSON::Serialize( root );

   try
   {
      String dir = File::ExtractDirectory( m_dbPath );
      if ( !File::DirectoryExists( dir ) )
         File::CreateDirectory( dir );
      File::WriteTextFile( m_dbPath, IsoString( json.c_str() ) );
      return true;
   }
   catch ( ... )
   {
      return false;
   }
}

bool AstroBinCSVGeneratorEngine::DownloadFilterDatabase()
{
   Console c;
   c.WriteLn( "API: " + String( ABCG_ASTROBIN_API_BASE ) );
   c.WriteLn();

   std::vector<FilterEntry> all;
   int page = 1;
   int totalCount = 0;

   auto strOrEmpty = []( const ABCGJSON::Value& r, const char* key ) -> std::string
   {
      const ABCGJSON::Value* v = r.Find( key );
      if ( v == nullptr || v->type != ABCGJSON::StringType )
         return std::string();
      return v->str;
   };

   for ( ;; )
   {
      String url = String( ABCG_ASTROBIN_API_BASE ) + "?format=json&page=" + String( page );
      c.WriteLn( "Fetching page " + String( page ) + "..." );

      String response = HttpGet( url );
      if ( response.IsEmpty() )
      {
         c.CriticalLn( "Failed to fetch page " + String( page ) + ". Aborting." );
         return false;
      }

      ABCGJSON::Value data;
      std::string text = response.ToIsoString().c_str();
      if ( !ABCGJSON::Parse( text, data ) )
      {
         c.CriticalLn( "Invalid JSON on page " + String( page ) + ". Aborting." );
         return false;
      }

      if ( page == 1 )
      {
         const ABCGJSON::Value* count = data.Find( "count" );
         if ( count != nullptr && count->type == ABCGJSON::NumberType )
         {
            totalCount = int( count->num );
            c.WriteLn( "Total filters in database: " + String( totalCount ) );
         }
      }

      const ABCGJSON::Value* results = data.Find( "results" );
      if ( results == nullptr || !results->IsArray() || results->arr.empty() )
         break;

      for ( const ABCGJSON::Value& r : results->arr )
      {
         FilterEntry fe;

         const ABCGJSON::Value* id = r.Find( "id" );
         if ( id != nullptr )
            fe.id = ValueToString( *id );

         std::string rawName     = strOrEmpty( r, "name" );
         std::string rawSN       = strOrEmpty( r, "searchFriendlyName" );
         std::string rawBrand    = strOrEmpty( r, "brandName" );
         std::string rawType     = strOrEmpty( r, "type" );
         std::string rawSize     = strOrEmpty( r, "size" );

         fe.name = String( rawName.c_str() );
         fe.searchFriendlyName = rawSN.empty() ? fe.name : String( rawSN.c_str() );
         fe.brandName = String( rawBrand.c_str() );

         // Preserve the full object with the same key order the JS script
         // stores in its local cache.
         const char* keys[] = { "id", "name", "searchFriendlyName", "brandName",
                                "type", "bandwidth", "size" };
         fe.raw.type = ABCGJSON::ObjectType;
         for ( const char* k : keys )
         {
            const ABCGJSON::Value* fv = r.Find( k );
            ABCGJSON::Value v;
            if ( fv != nullptr )
               v = *fv;
            if ( strcmp( k, "bandwidth" ) == 0 && ( v.type != ABCGJSON::NumberType || v.num == 0 ) )
               v.type = ABCGJSON::NullType; // JS: r.bandwidth || null
            fe.raw.obj.push_back( { k, v } );
         }

         // name/searchFriendlyName/size/type fall back to empty strings in JS.
         fe.raw.obj[1].second.type = ABCGJSON::StringType;
         fe.raw.obj[1].second.str  = rawName;
         fe.raw.obj[2].second.type = ABCGJSON::StringType;
         fe.raw.obj[2].second.str  = rawSN.empty() ? rawName : rawSN;
         fe.raw.obj[3].second.type = ABCGJSON::StringType;
         fe.raw.obj[3].second.str  = rawBrand;
         fe.raw.obj[4].second.type = ABCGJSON::StringType;
         fe.raw.obj[4].second.str  = rawType;
         fe.raw.obj[6].second.type = ABCGJSON::StringType;
         fe.raw.obj[6].second.str  = rawSize;

         all.push_back( fe );
      }

      c.WriteLn( "  Got " + String( results->arr.size() ) + " filters (total so far: " +
                 String( all.size() ) + ")" );

      if ( int( results->arr.size() ) < 50 )
         break;
      if ( totalCount > 0 && int( all.size() ) >= totalCount )
         break;
      page++;

      // Keep the GUI responsive during long downloads (matches the JS script's
      // CoreApplication.processEvents() between pages). On a worker thread this
      // also lets the user abort the process.
      Module->ProcessEvents();
   }

   m_filters = all;
   m_lastUpdated = NowIsoString();

   if ( SaveFilterDatabase() )
   {
      c.WriteLn();
      c.WriteLn( "<b>Download complete:</b> " + String( all.size() ) +
                 " filters saved to " + m_dbPath );
      return true;
   }

   c.CriticalLn( "Downloaded filters but failed to save to disk." );
   return false;
}

String AstroBinCSVGeneratorEngine::HttpGet( const String& url )
{
   NetworkTransfer transfer;
   transfer.SetURL( url );
   transfer.SetCustomHTTPHeaders(
      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36\n"
      "Accept: application/json" );

   ABCGDownloadSink sink;
   transfer.OnDownloadDataAvailable( (NetworkTransfer::download_event_handler)&ABCGDownloadSink::e_Download, sink );

   if ( !transfer.Download() )
   {
      Console c;
      c.CriticalLn( "HTTP GET failed: " + transfer.ErrorInformation() );
      return String();
   }

   return String( sink.data );
}

String AstroBinCSVGeneratorEngine::SearchFilterDatabase( const String& name ) const
{
   String n = name.Trimmed();
   if ( n.IsEmpty() )
      return String();
   IsoString lower = n.ToIsoString();
   lower.ToLowercase();

   // Pass 1: exact match on searchFriendlyName
   for ( const FilterEntry& f : m_filters )
   {
      IsoString s = f.searchFriendlyName.ToIsoString();
      s.ToLowercase();
      if ( s == lower )
         return f.id;
   }

   // Pass 2: exact match on name
   for ( const FilterEntry& f : m_filters )
   {
      IsoString s = f.name.ToIsoString();
      s.ToLowercase();
      if ( s == lower )
         return f.id;
   }

   // Pass 3: substring match on searchFriendlyName
   for ( const FilterEntry& f : m_filters )
   {
      IsoString s = f.searchFriendlyName.ToIsoString();
      s.ToLowercase();
      if ( s.Contains( lower ) )
         return f.id;
   }

   // Pass 4: substring match on name
   for ( const FilterEntry& f : m_filters )
   {
      IsoString s = f.name.ToIsoString();
      s.ToLowercase();
      if ( s.Contains( lower ) )
         return f.id;
   }

   // Pass 5: reverse substring — input contains the filter name
   for ( const FilterEntry& f : m_filters )
   {
      IsoString s = f.searchFriendlyName.ToIsoString();
      s.ToLowercase();
      if ( s.Length() > 2 && lower.Contains( s ) )
         return f.id;
   }

   return String();
}

String AstroBinCSVGeneratorEngine::MapFilter( const String& name ) const
{
   if ( name.IsEmpty() )
      return String();
   String n = name.Trimmed();
   if ( n.IsEmpty() )
      return String();

   // Pass 1: exact match on the user's custom filter map
   for ( const auto& p : m_filterMap )
      if ( p.first == n )
         return p.second;

   // Pass 2: case-insensitive match on the custom filter map
   IsoString lower = n.ToIsoString();
   lower.ToLowercase();
   for ( const auto& p : m_filterMap )
   {
      IsoString k = p.first.ToIsoString();
      k.ToLowercase();
      if ( k == lower )
         return p.second;
   }

   // Pass 3: search the downloaded AstroBin filter database
   if ( !m_filters.empty() )
   {
      String db = SearchFilterDatabase( n );
      if ( !db.IsEmpty() )
      {
         Console c;
         c.WriteLn( "  Filter '" + n + "' -> AstroBin DB: " + db );
         return db;
      }
   }

   // Pass 4: use the default filter if enabled
   if ( UseDefaultFilter && !DefaultFilter.IsEmpty() )
   {
      Console c;
      c.WriteLn( "  Filter '" + n + "' -> using default: " + DefaultFilter );
      return DefaultFilter;
   }

   // No match found — return the raw name
   Console c;
   c.WriteLn( "  Warning: Filter '" + n + "' has no AstroBin mapping!" );
   return n;
}

// ----------------------------------------------------------------------------
// FITS / XISF header readers
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::ReadFITSHeaders( const String& filePath,
   std::map<std::string,ABCGJSON::Value>& keywords )
{
   try
   {
      File f;
      f.OpenForReading( filePath );
      if ( !f.IsOpen() )
         return false;

      char card[ABCG_FITS_CARD_SIZE];
      for ( ;; )
      {
         fsize_type remaining = f.Size() - f.Position();
         if ( remaining <= 0 )
            break;
         fsize_type toRead = std::min<fsize_type>( remaining, ABCG_FITS_CARD_SIZE );
         f.Read( card, toRead );

         std::string cardStr( card, size_t( toRead ) );

         std::string name = Trim( cardStr.substr( 0, 8 ) );
         if ( name.empty() )
            break;
         {
            std::string up = name;
            for ( char& c : up )
               c = char( std::toupper( (unsigned char)c ) );
            if ( up == "END" )
               break;
         }

         if ( name == "HIERARCH" )
         {
            size_t eqPos = cardStr.find( "=", 9 );
            if ( eqPos != std::string::npos )
            {
               std::string hName = Trim( cardStr.substr( 9, eqPos - 9 ) );
               std::string hValue = StripFITSComment( cardStr.substr( eqPos + 1 ) );
               keywords[hName] = ParseFITSValueString( hValue, false );
            }
         }
         else if ( cardStr.size() > 8 && cardStr[8] == '=' )
         {
            std::string value = StripFITSComment( cardStr.substr( 9 ) );
            keywords[name] = ParseFITSValueString( value, true );
         }
      }

      f.Close();
      return true;
   }
   catch ( ... )
   {
      return false;
   }
}

bool AstroBinCSVGeneratorEngine::ReadXISFHeaders( const String& filePath,
   std::map<std::string,ABCGJSON::Value>& keywords )
{
   try
   {
      File f;
      f.OpenForReading( filePath );
      if ( !f.IsOpen() )
         return false;

      char sig[8];
      f.Read( sig, 8 );
      if ( std::string( sig, 8 ) != "XISF0100" )
         return false;

      char lenB[4];
      f.Read( lenB, 4 );
      unsigned headerLength = (unsigned char)lenB[0]
         | ( (unsigned char)lenB[1] << 8 )
         | ( (unsigned char)lenB[2] << 16 )
         | ( (unsigned char)lenB[3] << 24 );
      if ( headerLength < 65 )
         return false;

      char pad[4];
      f.Read( pad, 4 );

      std::string xml;
      xml.resize( headerLength );
      f.Read( &xml[0], headerLength );
      f.Close();

      // Extract FITSKeyword entries
      size_t s = 0;
      for ( ;; )
      {
         s = xml.find( "<FITSKeyword", s );
         if ( s == std::string::npos )
            break;
         s++;
         size_t e = xml.find( "/>", s );
         if ( e == std::string::npos )
            break;
         std::string kwStr = xml.substr( s, e - s );
         std::string kwName = GetXMLAttr( kwStr, "name" );
         if ( !kwName.empty() )
         {
            std::string kwValue = GetXMLAttr( kwStr, "value" );
            if ( kwValue.size() >= 2 && kwValue[0] == '\'' && kwValue.back() == '\'' )
               kwValue = Trim( kwValue.substr( 1, kwValue.size() - 2 ) );
            double num;
            ABCGJSON::Value v;
            v.type = ABCGJSON::StringType;
            if ( !kwValue.empty() && JsNumber( kwValue, num ) )
            {
               v.type = ABCGJSON::NumberType;
               v.num = num;
            }
            else
               v.str = kwValue;
            keywords[kwName] = v;
         }
         s = e;
      }

      // Extract Property entries
      s = 0;
      for ( ;; )
      {
         s = xml.find( "<Property", s );
         if ( s == std::string::npos )
            break;
         s++;
         size_t e = xml.find( "/>", s );
         if ( e == std::string::npos )
         {
            e = xml.find( "</Property>", s );
            if ( e == std::string::npos )
               break;
         }
         std::string propStr = xml.substr( s, e - s );
         std::string propName = GetXMLAttr( propStr, "id" );
         std::string propVal = GetXMLAttr( propStr, "value" );
         if ( !propName.empty() && !propVal.empty() )
         {
            if ( propVal.size() >= 2 && propVal[0] == '\'' && propVal.back() == '\'' )
               propVal = Trim( propVal.substr( 1, propVal.size() - 2 ) );
            double num;
            ABCGJSON::Value v;
            v.type = ABCGJSON::StringType;
            if ( !propVal.empty() && JsNumber( propVal, num ) )
            {
               v.type = ABCGJSON::NumberType;
               v.num = num;
            }
            else
               v.str = propVal;
            keywords[propName] = v;
         }
         s = e;
      }

      return true;
   }
   catch ( ... )
   {
      return false;
   }
}

// ----------------------------------------------------------------------------
// Frame extraction
// ----------------------------------------------------------------------------

AstroBinCSVGeneratorEngine::FrameData AstroBinCSVGeneratorEngine::ExtractFrameData(
   const std::map<std::string,ABCGJSON::Value>& rawKeywords, const String& filePath ) const
{
   FrameData frame;
   frame.filePath = filePath;
   frame.fileName = File::ExtractNameAndExtension( filePath );

   // Apply keyword overrides: canonical keyword -> source keyword present in
   // the file. Mirrors the JS settings.keywordOverrides handling.
   std::map<std::string,ABCGJSON::Value> kw = rawKeywords;
   for ( const auto& ov : m_keywordOverrides )
   {
      std::string canonical = ov.first.ToIsoString().c_str();
      std::string source = ov.second.ToIsoString().c_str();
      if ( !source.empty() && kw.count( source ) > 0 )
         kw[canonical] = kw[source];
   }

   // Image type
   const ABCGJSON::Value* typeVal = FirstTruthy( kw, { "IMAGETYP", "IMGTYPE", "FRAME" } );
   frame.imagetyp = NormalizeImageType( typeVal != nullptr ? ValueToString( *typeVal ) : String() );

   // Master frame detection (uses only IMAGETYP/IMGTYPE, as in the JS script)
   typeVal = FirstTruthy( kw, { "IMAGETYP", "IMGTYPE" } );
   String rawType = ( typeVal != nullptr ) ? ValueToString( *typeVal ) : String();
   rawType.ToUppercase();
   frame.isMaster = rawType.Contains( "MASTER" );

   // Exposure
   const ABCGJSON::Value* v = FirstTruthy( kw, { "EXPOSURE", "EXPTIME", "EXPOTIME" } );
   frame.exposure = ( v != nullptr ) ? JsNumber( *v ) : 0;

   // Observation date
   v = FirstTruthy( kw, { "DATE-OBS", "DATE", "DATETIME" } );
   frame.dateObs = ( v != nullptr ) ? ValueToString( *v ) : String();

   if ( !frame.dateObs.IsEmpty() )
   {
      bool hasTz = false;
      int tzMinutes = 0;
      int y = 0, mo = 0, d = 0, h = 0, mi = 0;
      double sec = 0;
      if ( ParseDateTime( frame.dateObs, hasTz, tzMinutes, y, mo, d, h, mi, sec ) )
      {
         frame.hasDate = true;
         if ( !hasTz )
         {
            // No timezone: JS interprets the string as local wall-clock time.
            frame.lYear = y;
            frame.lMonth = mo;
            frame.lDay = d;
            frame.lHour = h;
            frame.jd = JulianDate( y, mo, d, h, mi, sec );
         }
         else
         {
            // Timezone present: convert to UTC, then back to local components
            // so that the getHours()-style checks match the JS behavior.
            double jdUtc = JulianDate( y, mo, d, h, mi, sec ) - tzMinutes/1440.0;
            frame.jd = jdUtc;
            ConvertUtcToLocal( jdUtc, frame.lYear, frame.lMonth, frame.lDay, frame.lHour );
         }
      }
   }

   // Binning
   v = FirstTruthy( kw, { "XBINNING", "BINX", "BINNING" } );
   frame.xbinning = ( v != nullptr ) ? JsNumber( *v ) : 1;
   v = FirstTruthy( kw, { "YBINNING", "BINY" } );
   frame.ybinning = ( v != nullptr ) ? JsNumber( *v ) : frame.xbinning;

   // Gain
   v = FirstTruthy( kw, { "GAIN" } );
   frame.gain = ( v != nullptr ) ? JsNumber( *v ) : double( DefaultGain );
   v = FirstTruthy( kw, { "EGAIN" } );
   frame.egain = ( v != nullptr ) ? JsNumber( *v ) : 0;

   // Sensor temperature
   v = FirstTruthy( kw, { "CCD-TEMP", "CCDTEMP", "SENSORTMP", "TEMPERAT", "SET-TEMP", "TEMP" } );
   frame.ccdTemp = ( v != nullptr ) ? JsNumber( *v ) : DefaultTemperature;

   // Optical parameters
   v = FirstTruthy( kw, { "FOCALLEN", "FOC-LEN", "FOCLENGTH", "EFL" } );
   frame.focalLength = ( v != nullptr ) ? JsNumber( *v ) : FocalLength;
   v = FirstTruthy( kw, { "XPIXSZ", "YPIXSZ", "PIXSIZE" } );
   frame.pixelSize = ( v != nullptr ) ? JsNumber( *v ) : PixelSize;
   v = FirstTruthy( kw, { "FOCRATIO", "FOCUS" } );
   frame.focalRatio = ( v != nullptr ) ? JsNumber( *v ) : FocalRatio;

   // Image scale
   v = FirstTruthy( kw, { "IMSCALE" } );
   frame.imscale = ( v != nullptr ) ? JsNumber( *v ) : 0;
   if ( frame.imscale == 0 && frame.focalLength > 0 && frame.pixelSize > 0 )
      frame.imscale = ( 206.265 * frame.pixelSize ) / frame.focalLength;

   // FWHM / HFR
   v = FirstTruthy( kw, { "FWHM" } );
   frame.fwhm = ( v != nullptr ) ? JsNumber( *v ) : 0;
   v = FirstTruthy( kw, { "HFR" } );
   frame.hfr = ( v != nullptr ) ? JsNumber( *v ) : 0;
   if ( frame.fwhm == 0 && frame.hfr > 0 && frame.imscale > 0 )
      frame.fwhm = frame.hfr * frame.imscale * ABCG_FWHM_TO_HFR_FACTOR;

   // Filter
   v = FirstTruthy( kw, { "FILTER", "FILTERNAME", "FWHEEL" } );
   frame.filter = ( v != nullptr ) ? ValueToString( *v ) : String();
   {
      String fl = frame.filter;
      fl.ToLowercase();
      if ( fl == "nofilter" )
         frame.filter.Clear();
   }

   // Target
   v = FirstTruthy( kw, { "OBJECT", "OBJCTNAME", "OBJNAME", "TARGNAME" } );
   frame.object = ( v != nullptr ) ? ValueToString( *v ) : String( "Unknown" );

   // RA / Dec
   v = FirstTruthy( kw, { "RA", "OBJCTRA" } );
   frame.ra = ( v != nullptr ) ? ValueToString( *v ) : String();
   v = FirstTruthy( kw, { "DEC", "OBJCTDEC" } );
   frame.dec = ( v != nullptr ) ? ValueToString( *v ) : String();

   // Telescope / camera
   v = FirstTruthy( kw, { "TELESCOP", "INSTRUME" } );
   frame.telescope = ( v != nullptr ) ? ValueToString( *v ) : String();
   v = FirstTruthy( kw, { "CAMERA", "DETNAME", "DETSERNO" } );
   frame.camera = ( v != nullptr ) ? ValueToString( *v ) : String();

   // Site
   v = FirstTruthy( kw, { "SITE", "SITENAME", "OBSERVAT" } );
   frame.site = ( v != nullptr ) ? ValueToString( *v ) : SiteName;
   v = FirstTruthy( kw, { "SITELAT", "OBSGEO-B", "LAT-OBS", "LATITUDE" } );
   frame.siteLat = ( v != nullptr ) ? JsNumber( *v ) : SiteLatitude;
   v = FirstTruthy( kw, { "SITELONG", "OBSGEO-L", "LONG-OBS", "LONGITUDE" } );
   frame.siteLon = ( v != nullptr ) ? JsNumber( *v ) : SiteLongitude;

   // Sky conditions
   v = FirstTruthy( kw, { "BORTLE" } );
   frame.bortle = ( v != nullptr ) ? JsNumber( *v ) : double( Bortle );
   v = FirstTruthy( kw, { "SQM", "SKYQUAL" } );
   frame.sqm = ( v != nullptr ) ? JsNumber( *v ) : SQM;

   // Ambient temperature
   v = FirstTruthy( kw, { "FOCTEMP", "AMBIENT", "AOCAMBT" } );
   frame.foctemp = ( v != nullptr ) ? JsNumber( *v ) : 20;

   // Software
   v = FirstTruthy( kw, { "SWCREATE", "CREATOR", "SWCREATOR" } );
   frame.swcreate = ( v != nullptr ) ? ValueToString( *v ) : String( "Unknown" );

   return frame;
}

// ----------------------------------------------------------------------------

AstroBinCSVGeneratorEngine::FrameData AstroBinCSVGeneratorEngine::ExtractFrame( const String& filePath ) const
{
   std::map<std::string,ABCGJSON::Value> keywords;

   String ext = File::ExtractExtension( filePath );
   ext.ToLowercase();

   if ( ext == ".xisf" )
      ReadXISFHeaders( filePath, keywords );
   else if ( ext == ".fits" || ext == ".fit" || ext == ".fts" )
      ReadFITSHeaders( filePath, keywords );

   return ExtractFrameData( keywords, filePath );
}

// ----------------------------------------------------------------------------
// Session detection
// ----------------------------------------------------------------------------

void AstroBinCSVGeneratorEngine::DetectSessions( std::vector<FrameData>& frames,
   double gapHours, bool shiftSessions )
{
   std::vector<FrameData*> dated;
   for ( FrameData& f : frames )
      if ( f.hasDate )
         dated.push_back( &f );

   std::stable_sort( dated.begin(), dated.end(),
      []( const FrameData* a, const FrameData* b ) { return a->jd < b->jd; } );

   if ( !dated.empty() )
   {
      int sessionId = 0;
      dated[0]->sessionId = sessionId;

      for ( size_t i = 1; i < dated.size(); i++ )
      {
         double hoursDiff = ( dated[i]->jd - dated[i-1]->jd ) * 24.0;
         if ( hoursDiff > gapHours )
            sessionId++;
         dated[i]->sessionId = sessionId;
      }

      // Assign session dates. The first frame of each session defines the date.
      for ( size_t i = 0; i < dated.size(); i++ )
      {
         FrameData* f = dated[i];
         if ( !f->sessionDate.IsEmpty() )
            continue;

         std::vector<FrameData*> sess;
         for ( FrameData* s : dated )
            if ( s->sessionId == f->sessionId )
               sess.push_back( s );

         FrameData* first = sess[0];
         int y = first->lYear;
         int mo = first->lMonth;
         int d = first->lDay;
         if ( shiftSessions && first->lHour < 12 )
            AddDays( y, mo, d, -1 );
         String dateStr = FormatDate( y, mo, d );
         for ( FrameData* s : sess )
            s->sessionDate = dateStr;
      }
   }

   // Frames without a valid date
   for ( FrameData& f : frames )
      if ( f.sessionDate.IsEmpty() )
      {
         f.sessionDate = "Unknown";
         f.sessionId = -1;
      }
}

// ----------------------------------------------------------------------------
// Aggregation
// ----------------------------------------------------------------------------

std::vector<AstroBinCSVGeneratorEngine::AggregateRow>
AstroBinCSVGeneratorEngine::AggregateFrames( const std::vector<FrameData>& frames ) const
{
   // Group by sessionDate, filter (or override id), gain, binning, exposure,
   // object, image type — joined with the same "|||" delimiter as the JS.
   std::vector< std::pair<std::string, std::vector<const FrameData*> > > groups;

   for ( const FrameData& f : frames )
   {
      if ( f.isMaster )
         continue;
      if ( f.imagetyp != "LIGHT" )
         continue;

      std::string key;
      key += std::string( f.sessionDate.ToIsoString().c_str() );
      key += "|||";
      if ( f.hasFilterOverride )
         key += std::string( f.filterOverrideId.ToIsoString().c_str() );
      else
         key += std::string( f.filter.ToIsoString().c_str() );
      key += "|||";
      key += shortest_double_string( f.gain );
      key += "|||";
      key += shortest_double_string( f.xbinning );
      key += "|||";
      key += shortest_double_string( f.exposure );
      key += "|||";
      key += std::string( f.object.ToIsoString().c_str() );
      key += "|||LIGHT";

      bool found = false;
      for ( auto& g : groups )
         if ( g.first == key )
         {
            g.second.push_back( &f );
            found = true;
            break;
         }
      if ( !found )
         groups.push_back( { key, { &f } } );
   }

   std::vector<AggregateRow> results;
   for ( const auto& g : groups )
   {
      const std::vector<const FrameData*>& group = g.second;
      const FrameData& f0 = *group[0];

      AggregateRow agg;
      agg.sessionDate = f0.sessionDate;
      if ( f0.hasFilterOverride )
      {
         agg.filter = f0.filterOverrideLabel;
         agg.filterCode = f0.filterOverrideId;
      }
      else
      {
         agg.filter = f0.filter;
         agg.filterCode = MapFilter( f0.filter );
      }
      agg.gain = f0.gain;
      agg.xbinning = f0.xbinning;
      agg.exposure = f0.exposure;
      agg.object = f0.object;
      agg.number = int( group.size() );

      double sumTemp = 0, sumFwhm = 0, sumSqm = 0, sumFoctemp = 0, sumFocratio = 0;
      int countTemp = 0, countFwhm = 0, countSqm = 0, countFoctemp = 0, countFocratio = 0;
      for ( const FrameData* f : group )
      {
         if ( f->ccdTemp != 0 )     { sumTemp += f->ccdTemp; countTemp++; }
         if ( f->fwhm != 0 )        { sumFwhm += f->fwhm; countFwhm++; }
         if ( f->sqm != 0 )         { sumSqm += f->sqm; countSqm++; }
         if ( f->foctemp != 0 )     { sumFoctemp += f->foctemp; countFoctemp++; }
         if ( f->focalRatio != 0 )  { sumFocratio += f->focalRatio; countFocratio++; }
      }

      agg.sensorCooling = ( countTemp > 0 ) ? JsRound( sumTemp/countTemp ) : DefaultTemperature;
      agg.meanFwhm      = ( countFwhm > 0 ) ? JsRound2( sumFwhm/countFwhm ) : 0;
      agg.meanSqm       = ( countSqm > 0 ) ? JsRound2( sumSqm/countSqm ) : SQM;
      agg.temperature   = ( countFoctemp > 0 ) ? JsRound2( sumFoctemp/countFoctemp ) : 20;
      agg.fNumber       = ( countFocratio > 0 ) ? JsRound2( sumFocratio/countFocratio ) : FocalRatio;
      agg.bortle        = JsRound( f0.bortle );

      results.push_back( agg );
   }

   // Sort by session date, then filter, then gain (stable, like V8's TimSort).
   std::vector<size_t> idx( results.size() );
   for ( size_t i = 0; i < idx.size(); i++ )
      idx[i] = i;
   std::stable_sort( idx.begin(), idx.end(),
      [&results]( size_t a, size_t b )
      {
         const AggregateRow& x = results[a];
         const AggregateRow& y = results[b];
         if ( x.sessionDate < y.sessionDate )
            return true;
         if ( y.sessionDate < x.sessionDate )
            return false;
         if ( x.filter < y.filter )
            return true;
         if ( y.filter < x.filter )
            return false;
         if ( x.gain < y.gain )
            return true;
         if ( y.gain < x.gain )
            return false;
         return false;
      } );
   std::vector<AggregateRow> sortedResults;
   sortedResults.reserve( results.size() );
   for ( size_t i : idx )
      sortedResults.push_back( results[i] );
   results = std::move( sortedResults );

   return results;
}

// ----------------------------------------------------------------------------
// Override file
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::LoadOverrideFile( const String& overrideFilePath,
   std::vector< std::pair<String, std::pair<String,String> > >& overrides )
{
   try
   {
      if ( !File::Exists( overrideFilePath ) )
         return false;

      std::string content = File::ReadTextFile( overrideFilePath ).c_str();

      size_t pos = 0;
      while ( pos < content.size() )
      {
         size_t e = content.find( '\n', pos );
         if ( e == std::string::npos )
            e = content.size();
         std::string line = Trim( content.substr( pos, e - pos ) );
         pos = e + 1;

         if ( line.empty() )
            continue;

         // Skip the header line (filename,filter_name,filter_id)
         {
            std::string lower = line;
            for ( char& c : lower )
               c = char( std::tolower( (unsigned char)c ) );
            if ( lower.compare( 0, 9, "filename," ) == 0 )
               continue;
         }

         std::vector<std::string> cols;
         size_t cpos = 0;
         while ( cpos <= line.size() )
         {
            size_t cm = line.find( ',', cpos );
            if ( cm == std::string::npos )
            {
               cols.push_back( Trim( line.substr( cpos ) ) );
               break;
            }
            cols.push_back( Trim( line.substr( cpos, cm - cpos ) ) );
            cpos = cm + 1;
         }

         if ( cols.size() < 2 )
            continue;

         String fileName = String( cols[0].c_str() );
         String filterName = String( cols[1].c_str() );
         String filterId = ( cols.size() >= 3 ) ? String( cols[2].c_str() ) : String();
         overrides.push_back( { fileName, { filterName, filterId } } );
      }

      return !overrides.empty();
   }
   catch ( ... )
   {
      return false;
   }
}

// ----------------------------------------------------------------------------
// File collection
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::CollectFiles( const String& dir, bool recursive,
   std::vector<String>& files ) const
{
   std::vector<String> pending;
   pending.push_back( dir );

   std::vector<String> found;
   while ( !pending.empty() )
   {
      String d = pending.back();
      pending.pop_back();

      std::vector<String> subdirs;
      File::Find f( d + "/" + "*" );
      FindFileInfo info;
      while ( f.NextItem( info ) )
      {
         if ( info.IsDirectory() )
         {
            if ( info.name != "." && info.name != ".." )
               subdirs.push_back( info.name );
         }
         else
         {
            String full = File::FullPath( d + "/" + info.name );
            if ( HasSupportedExtension( full ) )
               found.push_back( full );
         }
      }

      if ( recursive )
         for ( const String& sd : subdirs )
            pending.push_back( d + "/" + sd );
   }

   std::vector<size_t> idx( found.size() );
   for ( size_t i = 0; i < idx.size(); i++ )
      idx[i] = i;
   std::sort( idx.begin(), idx.end(),
      [&found]( size_t a, size_t b ) { return found[a] < found[b]; } );
   std::vector<String> sorted;
   sorted.reserve( found.size() );
   for ( size_t i : idx )
      sorted.push_back( found[i] );
   files = std::move( sorted );
   return true;
}

bool AstroBinCSVGeneratorEngine::HasSupportedExtension( const String& path )
{
   String ext = File::ExtractExtension( path );
   ext.ToLowercase();
   return ext == ".xisf" || ext == ".fits" || ext == ".fit" || ext == ".fts";
}

// ----------------------------------------------------------------------------
// CSV generation
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::WriteCSV( const std::vector<AggregateRow>& rows,
   const String& outputPath )
{
   IsoString text;
   text = "date,filter,number,duration,binning,gain,sensorCooling,fNumber,darks,flats,flatDarks,bias,bortle,meanSqm,meanFwhm,temperature";
   text += '\n';

   for ( const AggregateRow& a : rows )
   {
      std::string line;
      line += a.sessionDate.ToIsoString().c_str();
      line += ',';
      line += a.filterCode.ToIsoString().c_str();
      line += ',';
      line += std::to_string( a.number );
      line += ',';
      line += shortest_double_string( JsRound2( a.exposure ) );
      line += ',';
      line += shortest_double_string( a.xbinning );
      line += ',';
      line += shortest_double_string( a.gain );
      line += ',';
      line += shortest_double_string( a.sensorCooling );
      line += ',';
      line += shortest_double_string( a.fNumber );
      line += ",0,0,0,0,";
      line += shortest_double_string( a.bortle );
      line += ',';
      line += shortest_double_string( a.meanSqm );
      line += ',';
      line += shortest_double_string( a.meanFwhm );
      line += ',';
      line += shortest_double_string( a.temperature );
      text.Append( line.c_str() );
      text += '\n';
   }

   try
   {
      File::WriteTextFile( outputPath, text );
      return true;
   }
   catch ( ... )
   {
      return false;
   }
}

// ----------------------------------------------------------------------------
// Main entry point
// ----------------------------------------------------------------------------

bool AstroBinCSVGeneratorEngine::Generate( const String& inputDirectory,
   const String& outputDirectory, const String& outputFileName,
   bool recursive, const String& overrideFilePath, const String& fileListJSON )
{
   Configure();

   // Load the local filter database cache if present. A missing database is
   // downloaded by the process instance before calling Generate().
   LoadFilterDatabase();

   // Optional per-file filter overrides attached to an explicit file list.
   // Each entry: { path, filterId, filterLabel }.
   struct InlineOverride
   {
      String path;
      String id;
      String label;
   };

   std::vector<InlineOverride> inlineOverrides;
   bool hasFileList = !fileListJSON.IsEmpty();

   std::vector<String> files;
   if ( hasFileList )
   {
      ABCGJSON::Value root;
      std::string text = fileListJSON.ToIsoString().c_str();
      if ( ABCGJSON::Parse( text, root ) && root.IsArray() )
      {
         for ( const ABCGJSON::Value& el : root.arr )
         {
            if ( !el.IsObject() )
               continue;

            InlineOverride io;
            const ABCGJSON::Value* p = el.Find( "path" );
            if ( p != nullptr && p->type == ABCGJSON::StringType )
               io.path = String( p->str.c_str() );
            if ( io.path.IsEmpty() )
               continue;

            const ABCGJSON::Value* id = el.Find( "filterId" );
            if ( id != nullptr && id->type == ABCGJSON::StringType )
               io.id = String( id->str.c_str() );
            const ABCGJSON::Value* label = el.Find( "filterLabel" );
            if ( label != nullptr && label->type == ABCGJSON::StringType )
               io.label = String( label->str.c_str() );

            files.push_back( io.path );
            inlineOverrides.push_back( io );
         }
      }
   }
   else
   {
      CollectFiles( inputDirectory, recursive, files );
   }

   std::vector< std::pair<String, std::pair<String,String> > > overrides;
   if ( !overrideFilePath.IsEmpty() )
      LoadOverrideFile( overrideFilePath, overrides );

   std::vector<FrameData> frames;
   int errors = 0;

   Console c;
   for ( const String& filePath : files )
   {
      String ext = File::ExtractExtension( filePath );
      ext.ToLowercase();

      std::map<std::string,ABCGJSON::Value> keywords;
      bool ok = false;
      if ( ext == ".xisf" )
         ok = ReadXISFHeaders( filePath, keywords );
      else if ( ext == ".fits" || ext == ".fit" || ext == ".fts" )
         ok = ReadFITSHeaders( filePath, keywords );
      else
         continue;

      if ( !ok || keywords.empty() )
      {
         c.WriteLn( "Warning: No keywords found in " + File::ExtractNameAndExtension( filePath ) );
         errors++;
         continue;
      }

      FrameData frame = ExtractFrameData( keywords, filePath );

      if ( hasFileList )
      {
         // Apply the per-file filter override attached to this file list entry.
         for ( const auto& io : inlineOverrides )
            if ( io.path == filePath )
            {
               if ( io.id.IsEmpty() )
                  frame.filter = io.label; // blank id -> normal mapping
               else
               {
                  frame.hasFilterOverride = true;
                  frame.filterOverrideId = io.id;
                  frame.filterOverrideLabel = io.label;
               }
               break;
            }
      }
      else
      {
         // Apply a per-file filter override if one exists in the override file.
         for ( const auto& ov : overrides )
            if ( ov.first == frame.fileName )
            {
               if ( ov.second.second.IsEmpty() )
                  frame.filter = ov.second.first; // blank id -> normal mapping
               else
               {
                  frame.hasFilterOverride = true;
                  frame.filterOverrideId = ov.second.second;
                  frame.filterOverrideLabel = ov.second.first;
               }
               break;
            }
      }

      frames.push_back( frame );
   }

   c.WriteLn( String().Format( "%d file(s) loaded, %d error(s).", int( frames.size() ), errors ) );

   DetectSessions( frames, SessionGapHours, ShiftOvernight && !UseObservingDate );

   m_results = AggregateFrames( frames );

   if ( m_results.empty() )
      return true;

   String outDir = outputDirectory.IsEmpty() ? inputDirectory : outputDirectory;
   String fileName = outputFileName.IsEmpty() ? String( "acquisition.csv" ) : outputFileName;
   String outputPath = outDir + "/" + fileName;

   return WriteCSV( m_results, outputPath );
}

// ----------------------------------------------------------------------------

} // pcl

// ----------------------------------------------------------------------------
// EOF AstroBinCSVGeneratorEngine.cpp

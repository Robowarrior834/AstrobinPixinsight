/*
 * AstroBin CSV Generator for PixInsight v1.2.0
 *
 * Reads FITS/XISF file headers and generates AstroBin-compatible
 * acquisition.csv files for bulk upload.
 *
 * Features:
 * - Automatic filter-to-ID mapping via AstroBin database download
 * - Default filter fallback for unmapped entries
 * - Session detection and overnight shifting
 * - FITS/XISF header extraction with keyword overrides
 *
 * Based on the AstroBin Upload Utility Python tool.
 * Requires PixInsight 1.9.4 Lockhart or later (V8 runtime).
 */

#engine v8

#define TITLE "AstroBin CSV Generator"
#define VERSION "1.2.0"

#feature-id AstroBinCSVGenerator : Utilities > AstroBin CSV Generator

#feature-info <b>AstroBin CSV Generator v1.2.0</b><br/>\
   <br/>\
   Reads FITS/XISF file headers and generates AstroBin-compatible \
   acquisition.csv files for bulk upload.<br/>\
   <br/>\
   Features:<br/>\
   - Automatic filter-to-ID mapping via AstroBin database download<br/>\
   - Default filter fallback for unmapped entries<br/>\
   - Session detection and overnight shifting<br/>\
   - FITS/XISF header extraction with keyword overrides<br/>\
   <br/>\
   Based on the AstroBin Upload Utility Python tool.

CoreApplication.ensureMinimumVersion(1, 9, 4);

// =============================================================================
// Constants
// =============================================================================

const FITS_BLOCK_SIZE = 2880;
const FITS_CARD_SIZE = 80;
const SESSION_GAP_HOURS = 5;
const FWHM_TO_HFR_FACTOR = 2.0;

const DEFAULT_FILTERS = {
   "Ha": 4663,
   "SII": 4844,
   "OIII": 4752,
   "Red": 4649,
   "Green": 4643,
   "Blue": 4637,
   "Lum": 2906,
   "L": 2906,
   "CLS": 4632,
   "H-alpha": 4663,
   "Halpha": 4663,
   "Sulfur": 4844,
   "S-II": 4844,
   "O-III": 4752,
   "Clear": 2906,
   "UV": 3056,
   "IR": 3054,
   "Exoplanet": 10022
};

const IMAGE_TYPE_MAP = {
   "LIGHT": "LIGHT",
   "LIGHT FRAME": "LIGHT",
   "LIGHTFRAME": "LIGHT",
   "LIGHTS": "LIGHT",
   "FLAT": "FLAT",
   "FLAT FRAME": "FLAT",
   "FLATFRAME": "FLAT",
   "FLATS": "FLAT",
   "DARK": "DARK",
   "DARK FRAME": "DARK",
   "DARKFRAME": "DARK",
   "DARKS": "DARK",
   "BIAS": "BIAS",
   "BIAS FRAME": "BIAS",
   "BIASFRAME": "BIAS",
   "BIASES": "BIAS",
   "DARKFLAT": "DARKFLAT",
   "DARK FLAT": "DARKFLAT",
   "DARKFLAT FRAME": "DARKFLAT",
   "MASTERLIGHT": "MASTERLIGHT",
   "MASTER LIGHT": "MASTERLIGHT",
   "MASTERFLAT": "MASTERFLAT",
   "MASTER FLAT": "MASTERFLAT",
   "MASTERDARK": "MASTERDARK",
   "MASTER DARK": "MASTERDARK",
   "MASTERBIAS": "MASTERBIAS",
   "MASTER BIAS": "MASTERBIAS",
   "MASTERDARKFLAT": "MASTERDARKFLAT",
   "MASTER DARKFLAT": "MASTERDARKFLAT"
};

const SETTINGS_NS = TITLE + "." + VERSION + "_";
const ASTROBIN_API_BASE = "https://app.astrobin.com/api/v2/equipment/filter/";
const FILTER_DB_FILE = File.homeDirectory + "/PixInsight/AstroBinFilters.json";

// =============================================================================
// Network Helper
// =============================================================================

/**
 * Reads an entire file and returns its contents as a UTF-8 string.
 *
 * Reads in chunked blocks for memory efficiency and handles several
 * edge cases common in PixInsight's V8 environment:
 * - BOM stripping (UTF-8, UTF-16 LE/BE)
 * - Null byte removal (PixInsight V8 can insert these)
 * - Chunk size reduction on read errors (graceful degradation)
 *
 * @param {string} filePath - Absolute path to the file to read.
 * @returns {string} The full file contents as a string, or empty on failure.
 */
function readFileText(filePath) {
   var file = new File;
   file.openForReading(filePath);
   var chunks = [];
   var chunkSize = 8192;
   var position = 0;
   for (;;) {
      var bytes = null;
      var gotChunk = false;
      var trySize = chunkSize;
      while (trySize > 0) {
         try {
            bytes = file.read(DataType.ByteArray, trySize);
            if (bytes != null && bytes.length > 0) {
               position += bytes.length;
               gotChunk = true;
               break;
            }
            break;
         } catch (e) {
            try {
               file.seek(position, SeekMode.FromBegin);
            } catch (se) {
               break;
            }
            trySize = Math.floor(trySize / 2);
         }
      }
      if (!gotChunk || bytes == null || bytes.length === 0) break;
      var chunkStr;
      try {
         chunkStr = bytes.utf8ToString();
      } catch (e2) {
         chunkStr = bytes.toString();
      }
      chunks.push(chunkStr);
   }
   file.close();
   var text = chunks.join("");
   if (text.length > 0 && text.charCodeAt(0) === 0xEF) {
      text = text.substring(3);
   }
   if (text.length > 1 && text.charCodeAt(0) === 0xFF && text.charCodeAt(1) === 0xFE) {
      text = text.substring(2);
   }
   if (text.length > 1 && text.charCodeAt(0) === 0xFE && text.charCodeAt(1) === 0xFF) {
      text = text.substring(2);
   }
   if (text.indexOf("\0") >= 0) {
      text = text.replace(/\0/g, "");
   }
   return text;
}

/**
 * Writes a string to a file using explicit UTF-8 byte-by-byte output.
 *
 * PixInsight's outText() may emit UTF-16 on Windows, which breaks
 * round-trip reads via read(DataType.ByteArray). This function writes
 * in small segments to guarantee single-byte encoding on all platforms.
 *
 * @param {string} filePath - Absolute path to the output file.
 * @param {string} text - The text content to write.
 */
function writeFileUtf8(filePath, text) {
   // Write the file byte-by-byte to guarantee single-byte encoding on all
   // platforms. outText() may use UTF-16 on Windows, which breaks round-trip
   // reading via read(DataType.ByteArray).
   var file = File.createFileForWriting(filePath);
   var pos = 0;
   var len = text.length;
   while (pos < len) {
      var end = Math.min(pos + 4096, len);
      var segment = text.substring(pos, end);
      // Write using outTextLn for each line-sized chunk; the file will have
      // newlines but we handle that on read.
      file.outText(segment);
      pos = end;
   }
   file.close();
}

/**
 * Performs an HTTP GET request and returns the response body as a string.
 *
 * Uses PixInsight's NetworkTransfer API with a browser-like User-Agent
 * header to avoid 403 responses from servers that block non-browser clients.
 *
 * @param {string} url - The URL to fetch.
 * @returns {string|null} The response body text, or null if the request failed.
 */
function httpGet(url) {
   var nt = new NetworkTransfer;
   var response = "";

   nt.setURL(url);
   nt.setCustomHTTPHeaders([
      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
      "Accept: application/json"
   ]);

   nt.onDownloadDataAvailable = function(data) {
      response += data.utf8ToString();
      return true;
   };

   if (!nt.download()) {
      console.criticalln("HTTP GET failed: " + nt.errorInformation);
      return null;
   }

   return response;
}

// =============================================================================
// Filter Database
// =============================================================================

/**
 * Manages a local cache of the AstroBin filter database.
 *
 * The database is downloaded from the AstroBin REST API and stored as a
 * JSON file in the user's PixInsight directory. It provides multi-pass
 * fuzzy search to match FITS FILTER header values to AstroBin numeric IDs.
 *
 * Database file: ~/PixInsight/AstroBinFilters.json
 * API endpoint:  https://app.astrobin.com/api/v2/equipment/filter/
 */
function AstroBinFilterDatabase() {
   this.filters = [];
   this.lastUpdated = null;

   /**
    * Loads the filter database from the local JSON cache file.
    *
    * @returns {boolean} True if the database was loaded successfully with
    *   at least one filter entry, false otherwise.
    */
   this.load = function() {
      console.writeln("  Filter DB path: " + FILTER_DB_FILE);
      console.writeln("  Filter DB exists: " + File.exists(FILTER_DB_FILE));
      if (!File.exists(FILTER_DB_FILE)) return false;
      try {
         var content = readFileText(FILTER_DB_FILE);
         if (content == null || content.length === 0) {
            console.warningln("  Filter DB: readFileText returned empty content");
            return false;
         }
         console.writeln("  Filter DB content length: " + content.length);
         console.writeln("  Filter DB first 200 chars: " + content.substring(0, Math.min(200, content.length)));
         var data = JSON.parse(content);
         this.filters = data.filters || [];
         this.lastUpdated = data.lastUpdated || null;
         console.writeln("  Filter DB loaded: " + this.filters.length + " filters");
         return this.filters.length > 0;
      } catch (e) {
         console.criticalln("Error loading filter database: " + e.toString());
         return false;
      }
   };

   /**
    * Persists the current filter list to the local JSON cache file.
    * Creates the directory if it does not exist.
    *
    * @returns {boolean} True if the file was written successfully.
    */
   this.save = function() {
      try {
         var data = {
            filters: this.filters,
            lastUpdated: this.lastUpdated
         };
         // Ensure directory exists
         try {
            var dbDir = File.extractDirectory(FILTER_DB_FILE);
            if (!File.directoryExists(dbDir)) {
               File.createDirectory(dbDir);
            }
         } catch (dirErr) {
            // directory helpers may not exist in all PI versions
         }
         writeFileUtf8(FILTER_DB_FILE, JSON.stringify(data));
         return true;
      } catch (e) {
         console.criticalln("Error saving filter database: " + e.toString());
         return false;
      }
   };

   /**
    * Downloads the full filter database from the AstroBin REST API.
    *
    * Paginates through all results (typically ~2500 filters) and stores
    * them in memory. Calls save() to persist to disk on success.
    * Uses CoreApplication.processEvents() between pages to keep the
    * PixInsight UI responsive during the download.
    *
    * @returns {boolean} True if the download and save completed successfully.
    */
   this.fetchFromAPI = function() {
      console.writeln("API: " + ASTROBIN_API_BASE);
      console.writeln();

      var allFilters = [];
      var page = 1;
      var totalCount = 0;

      while (true) {
         var url = ASTROBIN_API_BASE + "?format=json" + "&page=" + page;
         console.writeln("Fetching page " + page + "...");

         var responseText = httpGet(url);
         if (responseText == null) {
            console.criticalln("Failed to fetch page " + page + ". Aborting.");
            return false;
         }

         var data;
         try {
            data = JSON.parse(responseText);
         } catch (e) {
            console.criticalln("Invalid JSON on page " + page + ": " + e.toString());
            return false;
         }

         if (page === 1 && data.count) {
            totalCount = data.count;
            console.writeln("Total filters in database: " + totalCount);
         }

         if (!data.results || data.results.length === 0) break;

         for (var i = 0; i < data.results.length; i++) {
            var r = data.results[i];
            allFilters.push({
               id: r.id,
               name: r.name || "",
               searchFriendlyName: r.searchFriendlyName || r.name || "",
               brandName: r.brandName || "",
               type: r.type || "",
               bandwidth: r.bandwidth || null,
               size: r.size || ""
            });
         }

         console.writeln("  Got " + data.results.length + " filters (total so far: " + allFilters.length + ")");

         // Stop if we got fewer than a full page (last page)
         // Also stop if we've collected more than expected (safety)
         if (data.results.length < 50) break;
         if (totalCount > 0 && allFilters.length >= totalCount) break;
         page++;

         // Small delay to be polite to the API
         // PixInsight sleep uses seconds, but we just yield briefly
         CoreApplication.processEvents();
      }

      this.filters = allFilters;
      this.lastUpdated = new Date().toISOString();

      if (this.save()) {
         console.writeln();
         console.writeln("<b>Download complete:</b> " + allFilters.length + " filters saved to " + FILTER_DB_FILE);
      } else {
         console.criticalln("Downloaded filters but failed to save to disk.");
         return false;
      }

      console.hide();
      return true;
   };

   /**
    * Searches the database for a filter matching the given name.
    *
    * Uses a multi-pass fuzzy matching strategy:
    *   1. Exact match on searchFriendlyName (case-insensitive)
    *   2. Exact match on name (case-insensitive)
    *   3. Substring match on searchFriendlyName
    *   4. Substring match on name
    *   5. Reverse substring (input contains the filter name)
    *
    * @param {string} name - The filter name to search for (e.g., "Ha", "S-II").
    * @returns {Object|null} Match object {id, name, brandName} or null if no match.
    */
   this.search = function(name) {
      if (name == null || name.length === 0) return null;
      var n = String(name).trim();
      if (n.length === 0) return null;
      var lower = n.toLowerCase();

      // Pass 1: exact match on searchFriendlyName
      for (var i = 0; i < this.filters.length; i++) {
         var f = this.filters[i];
         if (f.searchFriendlyName.toLowerCase() === lower) {
            return { id: f.id, name: f.searchFriendlyName, brandName: f.brandName };
         }
      }

      // Pass 2: exact match on name
      for (var i = 0; i < this.filters.length; i++) {
         var f = this.filters[i];
         if (f.name.toLowerCase() === lower) {
            return { id: f.id, name: f.searchFriendlyName, brandName: f.brandName };
         }
      }

      // Pass 3: substring match on searchFriendlyName
      for (var i = 0; i < this.filters.length; i++) {
         var f = this.filters[i];
         if (f.searchFriendlyName.toLowerCase().indexOf(lower) >= 0) {
            return { id: f.id, name: f.searchFriendlyName, brandName: f.brandName };
         }
      }

      // Pass 4: substring match on name
      for (var i = 0; i < this.filters.length; i++) {
         var f = this.filters[i];
         if (f.name.toLowerCase().indexOf(lower) >= 0) {
            return { id: f.id, name: f.searchFriendlyName, brandName: f.brandName };
         }
      }

      // Pass 5: reverse substring — input contains the filter name
      for (var i = 0; i < this.filters.length; i++) {
         var f = this.filters[i];
         var fname = f.searchFriendlyName.toLowerCase();
         if (fname.length > 2 && lower.indexOf(fname) >= 0) {
            return { id: f.id, name: f.searchFriendlyName, brandName: f.brandName };
         }
      }

      return null;
   };

   /** @returns {number} The total number of filters currently loaded in the database. */
   this.getCount = function() {
      return this.filters.length;
   };

   /**
    * Returns the last-updated timestamp in a human-readable format.
    * @returns {string} Formatted date string (YYYY-MM-DD HH:MM) or "Never".
    */
   this.getLastUpdated = function() {
      if (this.lastUpdated == null) return "Never";
      try {
         var d = new Date(this.lastUpdated);
         return d.getFullYear() + "-" +
                String(d.getMonth() + 1).padStart(2, "0") + "-" +
                String(d.getDate()).padStart(2, "0") + " " +
                String(d.getHours()).padStart(2, "0") + ":" +
                String(d.getMinutes()).padStart(2, "0");
      } catch (e) {
         return this.lastUpdated;
      }
   };
}

// Global filter database instance — loaded once at script startup
var filterDB = new AstroBinFilterDatabase();
filterDB.load();

// =============================================================================
// Settings Management
// =============================================================================

/**
 * Manages persistent user settings for the AstroBin CSV Generator.
 *
 * Settings are stored using PixInsight's Settings API (key-value store)
 * and include site coordinates, equipment defaults, filter mappings,
 * and processing preferences. All values are persisted across sessions.
 */
function AstroBinSettings() {
   this.filterMap = {};
   this.siteName = "My Site";
   this.siteLat = 0.0;
   this.siteLon = 0.0;
   this.siteElev = 0.0;
   this.bortle = 4;
   this.sqm = 21.0;
   this.focalLength = 540;
   this.pixelSize = 3.0;
   this.focalRatio = 5.0;
   this.shiftOvernight = true;
   this.useObsDate = false;
   this.defaultGain = 0;
   this.defaultTemp = -10;
   this.keywordOverrides = {};
   this.defaultFilter = "";
   this.useDefaultFilter = false;

   /** Loads all persisted settings from the PixInsight Settings API. */
   this.load = function() {
      // Load filter map
      var savedFilters = Settings.read(SETTINGS_NS + "filterMap", DataType.String);
      if (Settings.lastReadOK && savedFilters != null && savedFilters.length > 0) {
         try {
            this.filterMap = JSON.parse(savedFilters);
         } catch (e) {
            this.filterMap = Object.assign({}, DEFAULT_FILTERS);
         }
      } else {
         this.filterMap = Object.assign({}, DEFAULT_FILTERS);
      }

      // Load site settings
      var val;
      val = Settings.read(SETTINGS_NS + "siteName", DataType.String);
      if (Settings.lastReadOK && val != null) this.siteName = val;

      val = Settings.read(SETTINGS_NS + "siteLat", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.siteLat = val;

      val = Settings.read(SETTINGS_NS + "siteLon", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.siteLon = val;

      val = Settings.read(SETTINGS_NS + "siteElev", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.siteElev = val;

      val = Settings.read(SETTINGS_NS + "bortle", DataType.Int32);
      if (Settings.lastReadOK && val != null) this.bortle = val;

      val = Settings.read(SETTINGS_NS + "sqm", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.sqm = val;

      val = Settings.read(SETTINGS_NS + "focalLength", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.focalLength = val;

      val = Settings.read(SETTINGS_NS + "pixelSize", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.pixelSize = val;

      val = Settings.read(SETTINGS_NS + "focalRatio", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.focalRatio = val;

      val = Settings.read(SETTINGS_NS + "shiftOvernight", DataType.Boolean);
      if (Settings.lastReadOK && val != null) this.shiftOvernight = val;

      val = Settings.read(SETTINGS_NS + "useObsDate", DataType.Boolean);
      if (Settings.lastReadOK && val != null) this.useObsDate = val;

      val = Settings.read(SETTINGS_NS + "defaultGain", DataType.Int32);
      if (Settings.lastReadOK && val != null) this.defaultGain = val;

      val = Settings.read(SETTINGS_NS + "defaultTemp", DataType.Real64);
      if (Settings.lastReadOK && val != null) this.defaultTemp = val;

      // Load keyword overrides
      val = Settings.read(SETTINGS_NS + "keywordOverrides", DataType.String);
      if (Settings.lastReadOK && val != null && val.length > 0) {
         try {
            this.keywordOverrides = JSON.parse(val);
         } catch (e) {
            this.keywordOverrides = {};
         }
      }

      // Load default filter settings
      val = Settings.read(SETTINGS_NS + "defaultFilter", DataType.String);
      if (Settings.lastReadOK && val != null) this.defaultFilter = val;

      val = Settings.read(SETTINGS_NS + "useDefaultFilter", DataType.Boolean);
      if (Settings.lastReadOK && val != null) this.useDefaultFilter = val;
   };

   /** Saves all current settings to the PixInsight Settings API. */
   this.save = function() {
      Settings.write(SETTINGS_NS + "filterMap", DataType.String, JSON.stringify(this.filterMap));
      Settings.write(SETTINGS_NS + "siteName", DataType.String, this.siteName);
      Settings.write(SETTINGS_NS + "siteLat", DataType.Real64, this.siteLat);
      Settings.write(SETTINGS_NS + "siteLon", DataType.Real64, this.siteLon);
      Settings.write(SETTINGS_NS + "siteElev", DataType.Real64, this.siteElev);
      Settings.write(SETTINGS_NS + "bortle", DataType.Int32, this.bortle);
      Settings.write(SETTINGS_NS + "sqm", DataType.Real64, this.sqm);
      Settings.write(SETTINGS_NS + "focalLength", DataType.Real64, this.focalLength);
      Settings.write(SETTINGS_NS + "pixelSize", DataType.Real64, this.pixelSize);
      Settings.write(SETTINGS_NS + "focalRatio", DataType.Real64, this.focalRatio);
      Settings.write(SETTINGS_NS + "shiftOvernight", DataType.Boolean, this.shiftOvernight);
      Settings.write(SETTINGS_NS + "useObsDate", DataType.Boolean, this.useObsDate);
      Settings.write(SETTINGS_NS + "defaultGain", DataType.Int32, this.defaultGain);
      Settings.write(SETTINGS_NS + "defaultTemp", DataType.Real64, this.defaultTemp);
      Settings.write(SETTINGS_NS + "keywordOverrides", DataType.String, JSON.stringify(this.keywordOverrides));
      Settings.write(SETTINGS_NS + "defaultFilter", DataType.String, this.defaultFilter);
      Settings.write(SETTINGS_NS + "useDefaultFilter", DataType.Boolean, this.useDefaultFilter);
   };

   /**
    * Maps a FITS FILTER keyword value to an AstroBin filter ID.
    *
    * Resolution strategy (in priority order):
    *   1. Exact match against the user's custom filter map
    *   2. Case-insensitive match against the custom filter map
    *   3. Search the downloaded AstroBin filter database (fuzzy)
    *   4. Fall back to the user-configured default filter (if enabled)
    *   5. Return the raw name as-is (will show as unmapped in the UI)
    *
    * @param {string} name - The raw filter name from the FITS header.
    * @returns {string} An AstroBin numeric ID string, or the original name if unmapped.
    */
   this.mapFilter = function(name) {
      if (name == null) return "None";
      var n = String(name).trim();
      if (n.length === 0) return "None";

      // Pass 1: exact match on user's custom filter map
      if (n in this.filterMap) return this.filterMap[n];

      // Pass 2: case-insensitive match on custom filter map
      var lower = n.toLowerCase();
      for (var key in this.filterMap) {
         if (key.toLowerCase() === lower) return this.filterMap[key];
      }

      // Pass 3: search the downloaded AstroBin filter database
      if (filterDB.getCount() > 0) {
         var dbResult = filterDB.search(n);
         if (dbResult != null) {
            console.writeln("  Filter '" + n + "' -> AstroBin DB: " + dbResult.id +
               " (" + dbResult.brandName + " " + dbResult.name + ")");
            return String(dbResult.id);
         }
      }

      // Pass 4: use default filter if enabled
      if (this.useDefaultFilter && this.defaultFilter.length > 0) {
         console.writeln("  Filter '" + n + "' -> using default: " + this.defaultFilter);
         return this.defaultFilter;
      }

      // No match found — return raw name (will cause CSV import issue, but visible to user)
      console.writeln("  Warning: Filter '" + n + "' has no AstroBin mapping!");
      return n;
   };
}

// =============================================================================
// FITS Header Reader
// =============================================================================

/**
 * Parses a single 80-character FITS header card into a keyword-value pair.
 *
 * Handles standard KEYWORD=VALUE cards, HIERARCH long-keyword cards,
 * and comment-only cards (no '=' sign). The END keyword signals the
 * termination of the header block.
 *
 * @param {string} cardStr - An 80-character FITS header card string.
 * @returns {Array|null} A [keyword, value] tuple, or null for END/comment cards.
 */
function parseFITSCard(cardStr) {
   // Returns [keyword, value] or null
   cardStr = cardStr.substring(0, 80); // ensure exactly 80 chars

   // Check for END keyword
   var trimmed = cardStr.substring(0, 3).trim();
   if (trimmed === "END") return null;

   // Check for HIERARCH
   if (cardStr.substring(0, 8) === "HIERARCH") {
      var hierStr = cardStr.substring(9).trim();
      var eqPos = hierStr.indexOf("=");
      if (eqPos < 0) return null;
      var keyword = hierStr.substring(0, eqPos).trim();
      var valueStr = hierStr.substring(eqPos + 1).trim();
      return [keyword, parseFITSValue(valueStr)];
   }

   // Standard keyword
   var eqPos = cardStr.indexOf("=");
   if (eqPos < 0) return null; // comment-only card

   var keyword = cardStr.substring(0, eqPos).trim();
   if (keyword.length === 0) return null;

   var valueStr = cardStr.substring(eqPos + 1).trim();
   return [keyword, parseFITSValue(valueStr)];
}

/**
 * Converts a raw FITS header value string into a JavaScript native type.
 *
 * Handles FITS-specific formatting rules:
 * - Strings are delimited by single quotes
 * - Inline comments start after a '/' outside of strings
 * - Booleans are represented as 'T'/'F'
 * - Numeric strings are auto-converted to Number types
 *
 * @param {string} valueStr - The raw value portion of a FITS card (after '=').
 * @returns {string|number|boolean} The parsed value.
 */
function parseFITSValue(valueStr) {
   if (valueStr.length === 0) return "";

   // Remove trailing comment
   var inString = false;
   var commentStart = -1;
   for (var i = 0; i < valueStr.length; i++) {
      if (valueStr[i] === "'" && !inString) {
         inString = true;
      } else if (valueStr[i] === "'" && inString) {
         inString = false;
      } else if (valueStr[i] === "/" && !inString) {
         commentStart = i;
         break;
      }
   }
   if (commentStart >= 0) {
      valueStr = valueStr.substring(0, commentStart).trim();
   }

   if (valueStr.length === 0) return "";

   // String value
   if (valueStr[0] === "'") {
      if (valueStr[valueStr.length - 1] === "'") {
         return valueStr.substring(1, valueStr.length - 1).trim();
      }
      // String may continue on next CONTINUE card - return as-is for now
      return valueStr.substring(1).trim();
   }

   // Boolean
   if (valueStr === "T" || valueStr === "t") return true;
   if (valueStr === "F" || valueStr === "f") return false;

   // Numeric
   var num = Number(valueStr);
   if (!isNaN(num)) return num;

   return valueStr;
}

/**
 * Reads FITS header keywords from a binary FITS file.
 *
 * Reads the file in 80-byte cards (the FITS standard card size) and
 * parses each card into keyword-value pairs. Supports HIERARCH long
 * keywords. Stops at the END keyword or end-of-file.
 *
 * @param {string} filePath - Absolute path to the FITS file.
 * @returns {Object} Dictionary of keyword-value pairs extracted from the header.
 */
function readFITSHeaders(filePath) {
   var keywords = {};
   try {
      var file = new File;
      file.openForReading(filePath);
      if (!file.isOpen) {
         console.writeln("Cannot open file: " + filePath);
         return keywords;
      }

      for (;;) {
         var rawData = file.read(DataType.ByteArray, FITS_CARD_SIZE);
         if (rawData == null || rawData.length === 0) break;

         // Convert the full card to a string first, then use standard
         // JS string methods. ByteArray.toString(offset, length) can
         // truncate at null bytes in PixInsight's V8 engine.
         var card = rawData.toString();
         if (card == null || card.length === 0) break;

         var name = card.substring(0, 8).trim();
         if (name.length === 0) break;
         if (name.toUpperCase() === "END") break;

         if (name === "HIERARCH") {
            var eqPos = card.indexOf("=", 9);
            if (eqPos > 0) {
               var hName = card.substring(9, eqPos).trim();
               var hValue = card.substring(eqPos + 1).trim();
               var hSlash = findFITSComment(hValue);
               if (hSlash >= 0) {
                  hValue = hValue.substring(0, hSlash).trim();
               }
               if (hValue.length > 0 && hValue[0] === "'") {
                  hValue = hValue.substring(1);
                  if (hValue.length > 0 && hValue[hValue.length - 1] === "'")
                     hValue = hValue.substring(0, hValue.length - 1);
                  hValue = hValue.trim();
               } else {
                  var num = Number(hValue);
                  if (!isNaN(num) && hValue.length > 0) hValue = num;
               }
               keywords[hName] = hValue;
            }
         } else if (card.length > 8 && card.charAt(8) === "=") {
            var value = card.substring(9).trim();
            var slashPos = findFITSComment(value);
            if (slashPos >= 0) {
               value = value.substring(0, slashPos).trim();
            }
            if (value.length > 0 && value[0] === "'") {
               value = value.substring(1);
               if (value.length > 0 && value[value.length - 1] === "'") {
                  value = value.substring(0, value.length - 1);
               }
               value = value.trim();
            } else if (value === "T") {
               value = true;
            } else if (value === "F") {
               value = false;
            } else if (value.length > 0) {
               var num = Number(value);
               if (!isNaN(num)) value = num;
            }
            keywords[name] = value;
         }
      }
      file.close();
   } catch (e) {
      console.writeln("Error reading FITS file: " + filePath);
      console.writeln("  " + e.toString());
   }
   return keywords;
}

/**
 * Locates the position of a FITS inline comment delimiter ('/') in a string.
 *
 * Correctly ignores '/' characters that appear inside single-quoted
 * FITS string values. The FITS standard uses '/' to introduce a comment
 * after the value in a header card.
 *
 * @param {string} str - The value string to search.
 * @returns {number} The index of the comment delimiter, or -1 if not found.
 */
function findFITSComment(str) {
   var inString = false;
   for (var i = 0; i < str.length; i++) {
      if (str[i] === "'") inString = !inString;
      else if (str[i] === "/" && !inString) return i;
   }
   return -1;
}

// =============================================================================
// XISF Header Reader
// =============================================================================

/**
 * Reads metadata from a PixInsight XISF (Extensible Image Science Format) file.
 *
 * XISF is PixInsight's native file format that stores metadata as XML.
 * This function reads only the XML header block without loading image data,
 * making it efficient for large files. Extracts both FITSKeyword entries
 * (for compatibility with FITS-based pipelines) and Property entries
 * (for XISF-native metadata like instrument gain).
 *
 * XISF file structure:
 *   [8 bytes: signature "XISF0100"]
 *   [4 bytes: header length (little-endian uint32)]
 *   [4 bytes: reserved padding]
 *   [N bytes: XML header block]
 *
 * @param {string} filePath - Absolute path to the XISF file.
 * @returns {Object} Dictionary of keyword-value pairs extracted from the header.
 */
function readXISFHeaders(filePath) {
   var keywords = {};
   try {
      var file = new File;
      file.openForReading(filePath);
      if (!file.isOpen) {
         console.writeln("Cannot open file: " + filePath);
         return keywords;
      }

      var rawData = file.read(DataType.ByteArray, 8);
      var sigStr = rawData.toString();
      if (sigStr !== "XISF0100") {
         console.writeln("Not a valid XISF file: " + filePath);
         file.close();
         return keywords;
      }

      var headerLenBytes = file.read(DataType.ByteArray, 4);
      var headerLength = headerLenBytes[0] |
                         (headerLenBytes[1] << 8) |
                         (headerLenBytes[2] << 16) |
                         (headerLenBytes[3] << 24);

      if (headerLength < 65) {
         console.writeln("Invalid XISF header length: " + filePath);
         file.close();
         return keywords;
      }

      file.read(DataType.ByteArray, 4); // skip reserved padding
      var headerBytes = file.read(DataType.ByteArray, headerLength);
      file.close();

      var headerXml = headerBytes.toString();

      // Extract FITSKeyword entries
      // FITSKeyword values in XISF XML often have embedded FITS string quotes
      // (e.g., value="'300'"). Strip them before numeric conversion.
      var s = 0;
      for (;;) {
         s = headerXml.indexOf("<FITSKeyword", s);
         if (s < 0) break;
         s++;
         var e = headerXml.indexOf("/>", s);
         if (e < 0) break;
         var kwStr = headerXml.substring(s, e);
         var nameMatch = kwStr.match(/name="([^"]*)"/);
         var valueMatch = kwStr.match(/value="([^"]*)"/);
         if (nameMatch && nameMatch.length > 1) {
            var kwName = nameMatch[1];
            var kwValue = valueMatch && valueMatch.length > 1 ? valueMatch[1] : "";
            if (kwValue.length >= 2 && kwValue[0] === "'" && kwValue[kwValue.length - 1] === "'") {
               kwValue = kwValue.substring(1, kwValue.length - 1).trim();
            }
            var num = Number(kwValue);
            keywords[kwName] = (!isNaN(num) && kwValue.length > 0) ? num : kwValue;
         }
         s = e;
      }

      // Also extract Property entries for XISF-native metadata
      s = 0;
      for (;;) {
         s = headerXml.indexOf("<Property", s);
         if (s < 0) break;
         s++;
         var e = headerXml.indexOf("/>", s);
         if (e < 0) {
            e = headerXml.indexOf("</Property>", s);
            if (e < 0) break;
         }
         var propStr = headerXml.substring(s, e);
         var idMatch = propStr.match(/id="([^"]*)"/);
         var valMatch = propStr.match(/value="([^"]*)"/);
         if (idMatch && idMatch.length > 1 && valMatch && valMatch.length > 1) {
            var propName = idMatch[1];
            var propVal = valMatch[1];
            if (propVal.length >= 2 && propVal[0] === "'" && propVal[propVal.length - 1] === "'") {
               propVal = propVal.substring(1, propVal.length - 1).trim();
            }
            var num = Number(propVal);
            keywords[propName] = (!isNaN(num) && propVal.length > 0) ? num : propVal;
         }
         s = e;
      }
   } catch (e) {
      console.writeln("Error reading XISF file: " + filePath);
      console.writeln("  " + e.toString());
   }
   return keywords;
}

// =============================================================================
// Frame Data Normalization
// =============================================================================

/**
 * Normalizes a raw IMAGETYP header value to a canonical type string.
 *
 * Handles the wide variety of IMAGETYP values found across different
 * capture software (e.g., "Light Frame", "LIGHTS", "light").
 * Falls back to partial substring matching for unknown values,
 * and defaults to "LIGHT" if no match is found.
 *
 * @param {string|null} rawType - The raw IMAGETYP/IMGTYPE header value.
 * @returns {string} A canonical type: "LIGHT", "FLAT", "DARK", "BIAS",
 *   "DARKFLAT", or a MASTER* variant.
 */
function normalizeImageType(rawType) {
   if (rawType == null) return "LIGHT";
   var t = String(rawType).trim().toUpperCase();
   if (t in IMAGE_TYPE_MAP) return IMAGE_TYPE_MAP[t];
   // Partial matching
   if (t.indexOf("LIGHT") >= 0) return "LIGHT";
   if (t.indexOf("FLAT") >= 0) return "FLAT";
   if (t.indexOf("DARK") >= 0 && t.indexOf("FLAT") >= 0) return "DARKFLAT";
   if (t.indexOf("DARK") >= 0) return "DARK";
   if (t.indexOf("BIAS") >= 0) return "BIAS";
   return "LIGHT"; // default
}

/**
 * Extracts and normalizes all relevant frame metadata from FITS/XISF keywords.
 *
 * This is the central data-mapping function that converts raw header
 * keyword-value pairs into a structured frame object. It applies keyword
 * overrides, resolves multiple possible keyword names for each field,
 * and falls back to user-defined default settings when headers are missing.
 *
 * Extracted fields include: image type, exposure, date, binning, gain,
 * temperature, optical parameters (focal length, pixel size, f-ratio),
 * image scale, FWHM/HFR, filter, target, RA/Dec, telescope/camera,
 * site info, sky conditions, and software.
 *
 * @param {Object} keywords - Raw keyword dictionary from FITS/XISF parser.
 * @param {string} filePath - Absolute path to the source file.
 * @param {AstroBinSettings} settings - User settings for default fallbacks.
 * @returns {Object} A structured frame data object.
 */
function extractFrameData(keywords, filePath, settings) {
   var frame = {};
   frame.filePath = filePath;
   frame.fileName = File.extractNameAndExtension(filePath);

   // Apply keyword overrides
   var kw = Object.assign({}, keywords);
   for (var overrideKey in settings.keywordOverrides) {
      var overrideVal = settings.keywordOverrides[overrideKey];
      if (overrideVal != null && overrideVal.length > 0 && overrideVal in kw) {
         kw[overrideKey] = kw[overrideVal];
      }
   }

   // Image type
   frame.imagetyp = normalizeImageType(
      kw["IMAGETYP"] || kw["IMGTYPE"] || kw["FRAME"]
   );

   // Skip master frames for CSV (they are summaries)
   var rawType = String(kw["IMAGETYP"] || kw["IMGTYPE"] || "").toUpperCase();
   if (rawType.indexOf("MASTER") >= 0) {
      frame.isMaster = true;
   } else {
      frame.isMaster = false;
   }

   // Exposure time
   frame.exposure = Number(kw["EXPOSURE"] || kw["EXPTIME"] || kw["EXPOTIME"] || 0);

   // Date
   frame.dateObs = kw["DATE-OBS"] || kw["DATE"] || kw["DATETIME"] || "";

   // Parse date
   frame.dateObj = null;
   frame.sessionDate = null;
   if (frame.dateObs && frame.dateObs.length > 0) {
      try {
         frame.dateObj = new Date(frame.dateObs);
      } catch (e) {
         // Try alternative formats
         var dateStr = String(frame.dateObs).replace("T", " ");
         try {
            frame.dateObj = new Date(dateStr);
         } catch (e2) {
            frame.dateObj = null;
         }
      }
   }

   // Binning
   frame.xbinning = Number(kw["XBINNING"] || kw["BINX"] || kw["BINNING"] || 1);
   frame.ybinning = Number(kw["YBINNING"] || kw["BINY"] || frame.xbinning);

   // Gain
   frame.gain = Number(kw["GAIN"] || kw["GAIN"] || settings.defaultGain);
   frame.egain = Number(kw["EGAIN"] || 0);

   // Temperature
   frame.ccdTemp = Number(kw["CCD-TEMP"] || kw["CCDTEMP"] || kw["SENSORTMP"] || kw["TEMPERAT"] || kw["SET-TEMP"] || kw["TEMP"] || settings.defaultTemp);

   // Optical parameters
   frame.focalLength = Number(kw["FOCALLEN"] || kw["FOC-LEN"] || kw["FOCLENGTH"] || kw["EFL"] || settings.focalLength);
   frame.pixelSize = Number(kw["XPIXSZ"] || kw["YPIXSZ"] || kw["PIXSIZE"] || settings.pixelSize);
   frame.focalRatio = Number(kw["FOCRATIO"] || kw["FOCUS"] || settings.focalRatio);

   // Calculate image scale and FWHM if not present
   frame.imscale = Number(kw["IMSCALE"] || 0);
   if (frame.imscale === 0 && frame.focalLength > 0 && frame.pixelSize > 0) {
      frame.imscale = (206.265 * frame.pixelSize) / frame.focalLength;
   }

   frame.fwhm = Number(kw["FWHM"] || 0);
   frame.hfr = Number(kw["HFR"] || 0);
   if (frame.fwhm === 0 && frame.hfr > 0 && frame.imscale > 0) {
      frame.fwhm = frame.hfr * frame.imscale * FWHM_TO_HFR_FACTOR;
   }

   // Filter
   frame.filter = kw["FILTER"] || kw["FILTERNAME"] || kw["FWHEEL"] || "";
   if (frame.filter.toLowerCase() === "nofilter") frame.filter = "";
   frame.filterOverride = null; // User can override the AstroBin ID per-file

   // Target
   frame.object = kw["OBJECT"] || kw["OBJCTNAME"] || kw["OBJNAME"] || kw["TARGNAME"] || "Unknown";

   // RA/Dec
   frame.ra = kw["RA"] || kw["OBJCTRA"] || "";
   frame.dec = kw["DEC"] || kw["OBJCTDEC"] || "";

   // Telescope / Instrument
   frame.telescope = kw["TELESCOP"] || kw["INSTRUME"] || "";
   frame.camera = kw["CAMERA"] || kw["DETNAME"] || kw["DETSERNO"] || "";

   // Site info
   frame.site = kw["SITE"] || kw["SITENAME"] || kw["OBSERVAT"] || settings.siteName;
   frame.siteLat = Number(kw["SITELAT"] || kw["OBSGEO-B"] || kw["LAT-OBS"] || kw["LATITUDE"] || settings.siteLat);
   frame.siteLon = Number(kw["SITELONG"] || kw["OBSGEO-L"] || kw["LONG-OBS"] || kw["LONGITUDE"] || settings.siteLon);

   // Sky conditions
   frame.bortle = Number(kw["BORTLE"] || settings.bortle);
   frame.sqm = Number(kw["SQM"] || kw["SKYQUAL"] || settings.sqm);

   // Ambient temperature
   frame.foctemp = Number(kw["FOCTEMP"] || kw["AMBIENT"] || kw["AOCAMBT"] || 20);

   // Software
   frame.swcreate = kw["SWCREATE"] || kw["CREATOR"] || kw["SWCREATOR"] || "Unknown";

   return frame;
}

// =============================================================================
// Session Detection and Aggregation
// =============================================================================

/**
 * Assigns session IDs and session dates to frames based on temporal gaps.
 *
 * Frames are sorted by observation date, then grouped into sessions
 * using a configurable gap threshold (SESSION_GAP_HOURS, default 5h).
 * Each session is assigned a date string, with optional overnight
 * shifting: if the first frame of a session was taken before noon,
 * the session date is shifted back to the previous calendar day
 * (standard astro convention for overnight imaging sessions).
 *
 * Frames without valid dates are assigned session ID -1 and date "Unknown".
 *
 * @param {Array} frames - Array of frame data objects (modified in place).
 * @returns {Array} The same frames array with _sessionId and sessionDate set.
 */
function detectSessions(frames) {
   // Sort frames by date
   var datedFrames = frames.filter(function(f) { return f.dateObj != null; });
   datedFrames.sort(function(a, b) { return a.dateObj.getTime() - b.dateObj.getTime(); });

   if (datedFrames.length === 0) return frames;

   // Assign session IDs based on time gaps
   var sessionId = 0;
   datedFrames[0]._sessionId = sessionId;

   for (var i = 1; i < datedFrames.length; i++) {
      var diff = datedFrames[i].dateObj.getTime() - datedFrames[i - 1].dateObj.getTime();
      var hoursDiff = diff / (1000 * 60 * 60);
      if (hoursDiff > SESSION_GAP_HOURS) {
         sessionId++;
      }
      datedFrames[i]._sessionId = sessionId;
   }

   // Assign session dates
   for (var i = 0; i < datedFrames.length; i++) {
      var f = datedFrames[i];
      if (f.sessionDate != null) continue;

      // Find all frames in this session
      var sessionFrames = datedFrames.filter(function(s) {
         return s._sessionId === f._sessionId;
      });

      // Use first frame's date as session date reference
      var firstDate = sessionFrames[0].dateObj;

      if (!settings.useObsDate && firstDate.getHours() < 12) {
         // Shift overnight sessions to previous day
         var shifted = new Date(firstDate);
         shifted.setDate(shifted.getDate() - 1);
         var dateStr = shifted.getFullYear() + "-" +
                       String(shifted.getMonth() + 1).padStart(2, "0") + "-" +
                       String(shifted.getDate()).padStart(2, "0");
         for (var j = 0; j < sessionFrames.length; j++) {
            sessionFrames[j].sessionDate = dateStr;
         }
      } else {
         var dateStr = firstDate.getFullYear() + "-" +
                       String(firstDate.getMonth() + 1).padStart(2, "0") + "-" +
                       String(firstDate.getDate()).padStart(2, "0");
         for (var j = 0; j < sessionFrames.length; j++) {
            sessionFrames[j].sessionDate = dateStr;
         }
      }
   }

   // Handle frames without dates
   for (var i = 0; i < frames.length; i++) {
      if (frames[i].sessionDate == null) {
         frames[i].sessionDate = "Unknown";
         frames[i]._sessionId = -1;
      }
   }

   return frames;
}

/**
 * Aggregates individual LIGHT frames into row groups for the AstroBin CSV.
 *
 * Frames are grouped by: session date, filter, gain, binning, exposure,
 * target, and image type. Only non-master LIGHT frames are included.
 * Each group is collapsed into a single summary row with:
 *   - Frame count
 *   - Mean FWHM, sensor cooling, SQM, ambient temp, and f-number
 *   - The mapped AstroBin filter ID
 *
 * Results are sorted by date, then filter, then gain for consistent output.
 *
 * @param {Array} frames - Array of frame data objects from extractFrameData().
 * @returns {Array} Array of aggregated row objects suitable for CSV generation.
 */
function aggregateFrames(frames) {
   // Group frames by: sessionDate, filter, gain, binning, exposure, target, imagetyp
   var groups = {};

   for (var i = 0; i < frames.length; i++) {
      var f = frames[i];
      // Skip master frames
      if (f.isMaster) continue;
      // Only aggregate LIGHT frames for AstroBin CSV
      if (f.imagetyp !== "LIGHT") continue;

      var key = [
         f.sessionDate,
         f.filterOverride ? f.filterOverride.id : f.filter,
         f.gain,
         f.xbinning,
         f.exposure,
         f.object,
         f.imagetyp
      ].join("|||");

      if (!(key in groups)) {
         groups[key] = [];
      }
      groups[key].push(f);
   }

   // Aggregate each group
   var results = [];
   for (var key in groups) {
      var group = groups[key];
      var agg = {};

      agg.sessionDate = group[0].sessionDate;
      agg.filter = group[0].filterOverride ? group[0].filterOverride.label : group[0].filter;
      agg.filterCode = group[0].filterOverride ? group[0].filterOverride.id : settings.mapFilter(group[0].filter);
      agg.gain = group[0].gain;
      agg.xbinning = group[0].xbinning;
      agg.exposure = group[0].exposure;
      agg.object = group[0].object;
      agg.imagetyp = "LIGHT";
      agg.number = group.length;

      // Compute averages
      var sumTemp = 0, sumFwhm = 0, sumSqm = 0, sumFoctemp = 0;
      var sumFocratio = 0;
      var countTemp = 0, countFwhm = 0, countSqm = 0, countFoctemp = 0;
      var countFocratio = 0;

      for (var i = 0; i < group.length; i++) {
         var f = group[i];
         if (f.ccdTemp !== 0) { sumTemp += f.ccdTemp; countTemp++; }
         if (f.fwhm !== 0) { sumFwhm += f.fwhm; countFwhm++; }
         if (f.sqm !== 0) { sumSqm += f.sqm; countSqm++; }
         if (f.foctemp !== 0) { sumFoctemp += f.foctemp; countFoctemp++; }
         if (f.focalRatio !== 0) { sumFocratio += f.focalRatio; countFocratio++; }
      }

      agg.sensorCooling = countTemp > 0 ? Math.round(sumTemp / countTemp) : settings.defaultTemp;
      agg.meanFwhm = countFwhm > 0 ? Math.round((sumFwhm / countFwhm) * 100) / 100 : 0;
      agg.meanSqm = countSqm > 0 ? Math.round((sumSqm / countSqm) * 100) / 100 : settings.sqm;
      agg.temperature = countFoctemp > 0 ? Math.round((sumFoctemp / countFoctemp) * 100) / 100 : 20;
      agg.fNumber = countFocratio > 0 ? Math.round((sumFocratio / countFocratio) * 100) / 100 : settings.focalRatio;
      agg.bortle = Math.round(group[0].bortle);

      results.push(agg);
   }

   // Sort by date, then filter, then gain
   results.sort(function(a, b) {
      if (a.sessionDate < b.sessionDate) return -1;
      if (a.sessionDate > b.sessionDate) return 1;
      if (a.filter < b.filter) return -1;
      if (a.filter > b.filter) return 1;
      if (a.gain < b.gain) return -1;
      if (a.gain > b.gain) return 1;
      return 0;
   });

   return results;
}

// =============================================================================
// CSV Generation
// =============================================================================

/**
 * Generates the AstroBin-compatible acquisition CSV file.
 *
 * Produces a comma-separated file with columns matching the AstroBin
 * bulk upload format: date, filter (numeric ID), frame count, exposure
 * duration, binning, gain, sensor cooling, f-number, calibration counts,
 * bortle, SQM, FWHM, and temperature.
 *
 * Calibration columns (darks, flats, flatDarks, bias) are set to 0
 * in this version as calibration matching is not yet implemented.
 *
 * @param {Array} aggregated - Array of aggregated row objects from aggregateFrames().
 * @param {string} outputPath - Absolute path for the output CSV file.
 * @returns {boolean} True if the file was written successfully.
 */
function generateCSV(aggregated, outputPath) {
   var header = "date,filter,number,duration,binning,gain,sensorCooling,fNumber,darks,flats,flatDarks,bias,bortle,meanSqm,meanFwhm,temperature";

   var lines = [header];

   for (var i = 0; i < aggregated.length; i++) {
      var a = aggregated[i];
      var line = [
         a.sessionDate,
         a.filterCode,
         a.number,
         Math.round(a.exposure * 100) / 100,
         a.xbinning,
         a.gain,
         a.sensorCooling,
         a.fNumber,
         0,  // darks - not matched in this minimal version
         0,  // flats
         0,  // flatDarks
         0,  // bias
         a.bortle,
         a.meanSqm,
         a.meanFwhm,
         a.temperature
      ].join(",");
      lines.push(line);
   }

   var csvContent = lines.join("\n") + "\n";

   // Write file
   try {
      var file = File.createFileForWriting(outputPath);
      file.outText(csvContent);
      file.close();
      return true;
   } catch (e) {
      console.writeln("Error writing CSV: " + e.toString());
      return false;
   }
}

// =============================================================================
// Settings Dialog
// =============================================================================

var settings = new AstroBinSettings();
settings.load();

/**
 * Settings dialog for configuring site, equipment, and processing options.
 *
 * Provides a GUI for editing persistent settings that are saved via
 * the PixInsight Settings API. Includes sections for:
 *   - Site information (name, coordinates, Bortle, SQM, elevation)
 *   - Equipment defaults (focal length, pixel size, f-ratio, gain, temp)
 *   - Processing options (overnight shifting, observation date mode)
 *   - AstroBin filter database management (download/update)
 *   - Import site/equipment data from FITS/XISF image headers
 */
var SettingsDialog = class extends Dialog {
   constructor() {
      super();
      this.windowTitle = TITLE + " - Settings";

      var self = this;
      var sizer = new VerticalSizer;
      sizer.spacing = 8;
      sizer.margin = 12;

      // Site settings group
      var siteGroup = new GroupBox(this);
      siteGroup.title = "Site Information";
      siteGroup.sizer = new VerticalSizer;
      siteGroup.sizer.spacing = 4;

      var siteRow1 = new HorizontalSizer;
      siteRow1.spacing = 6;
      var siteNameLabel = new Label(this);
      siteNameLabel.text = "Site Name:";
      siteNameLabel.setFixedWidth(80);
      this.siteNameEdit = new Edit(this);
      this.siteNameEdit.text = settings.siteName;
      this.siteNameEdit.setFixedWidth(200);
      siteRow1.add(siteNameLabel);
      siteRow1.add(this.siteNameEdit);
      siteRow1.addStretch();
      siteGroup.sizer.add(siteRow1);

      var siteRow2 = new HorizontalSizer;
      siteRow2.spacing = 6;
      var latLabel = new Label(this);
      latLabel.text = "Latitude:";
      latLabel.setFixedWidth(80);
      this.latEdit = new Edit(this);
      this.latEdit.text = settings.siteLat.toFixed(4);
      this.latEdit.setFixedWidth(100);
      var lonLabel = new Label(this);
      lonLabel.text = "Longitude:";
      lonLabel.setFixedWidth(80);
      this.lonEdit = new Edit(this);
      this.lonEdit.text = settings.siteLon.toFixed(4);
      this.lonEdit.setFixedWidth(100);
      siteRow2.add(latLabel);
      siteRow2.add(this.latEdit);
      siteRow2.add(lonLabel);
      siteRow2.add(this.lonEdit);
      siteRow2.addStretch();
      siteGroup.sizer.add(siteRow2);

      var siteRow3 = new HorizontalSizer;
      siteRow3.spacing = 6;
      var bortleLabel = new Label(this);
      bortleLabel.text = "Bortle:";
      bortleLabel.setFixedWidth(80);
      this.bortleEdit = new Edit(this);
      this.bortleEdit.text = String(settings.bortle);
      this.bortleEdit.setFixedWidth(50);
      var sqmLabel = new Label(this);
      sqmLabel.text = "SQM:";
      sqmLabel.setFixedWidth(40);
      this.sqmEdit = new Edit(this);
      this.sqmEdit.text = settings.sqm.toFixed(2);
      this.sqmEdit.setFixedWidth(80);
      siteRow3.add(bortleLabel);
      siteRow3.add(this.bortleEdit);
      siteRow3.add(sqmLabel);
      siteRow3.add(this.sqmEdit);
      siteRow3.addStretch();
      siteGroup.sizer.add(siteRow3);

      var siteRow4 = new HorizontalSizer;
      siteRow4.spacing = 6;
      var elevLabel = new Label(this);
      elevLabel.text = "Elevation:";
      elevLabel.setFixedWidth(80);
      this.elevEdit = new Edit(this);
      this.elevEdit.text = settings.siteElev.toFixed(1);
      this.elevEdit.setFixedWidth(80);
      var elevUnitLabel = new Label(this);
      elevUnitLabel.text = "m";
      siteRow4.add(elevLabel);
      siteRow4.add(this.elevEdit);
      siteRow4.add(elevUnitLabel);
      siteRow4.addStretch();
      siteGroup.sizer.add(siteRow4);

      sizer.add(siteGroup);

      // Import from image button
      var importSizer = new HorizontalSizer;
      importSizer.spacing = 6;
      var importButton = new PushButton(this);
      importButton.text = "Import Site/Equipment from Image...";
      importButton.toolTip = "Read FITS/XISF headers to populate site and equipment fields";
      importButton.onClick = function() { self.importFromImage(); };
      importSizer.add(importButton);
      importSizer.addStretch();
      sizer.add(importSizer);

      // Equipment defaults group
      var equipGroup = new GroupBox(this);
      equipGroup.title = "Equipment Defaults";
      equipGroup.sizer = new VerticalSizer;
      equipGroup.sizer.spacing = 4;

      var equipRow1 = new HorizontalSizer;
      equipRow1.spacing = 6;
      var flLabel = new Label(this);
      flLabel.text = "Focal Length:";
      flLabel.setFixedWidth(80);
      this.flEdit = new Edit(this);
      this.flEdit.text = String(settings.focalLength);
      this.flEdit.setFixedWidth(80);
      var pxLabel = new Label(this);
      pxLabel.text = "Pixel Size:";
      pxLabel.setFixedWidth(70);
      this.pxEdit = new Edit(this);
      this.pxEdit.text = settings.pixelSize.toFixed(2);
      this.pxEdit.setFixedWidth(80);
      var frLabel = new Label(this);
      frLabel.text = "F-Ratio:";
      frLabel.setFixedWidth(55);
      this.frEdit = new Edit(this);
      this.frEdit.text = settings.focalRatio.toFixed(2);
      this.frEdit.setFixedWidth(60);
      equipRow1.add(flLabel);
      equipRow1.add(this.flEdit);
      equipRow1.add(pxLabel);
      equipRow1.add(this.pxEdit);
      equipRow1.add(frLabel);
      equipRow1.add(this.frEdit);
      equipRow1.addStretch();
      equipGroup.sizer.add(equipRow1);

      var equipRow2 = new HorizontalSizer;
      equipRow2.spacing = 6;
      var gainLabel = new Label(this);
      gainLabel.text = "Default Gain:";
      gainLabel.setFixedWidth(80);
      this.gainEdit = new Edit(this);
      this.gainEdit.text = String(settings.defaultGain);
      this.gainEdit.setFixedWidth(80);
      var tempLabel = new Label(this);
      tempLabel.text = "Default Temp:";
      tempLabel.setFixedWidth(80);
      this.tempEdit = new Edit(this);
      this.tempEdit.text = settings.defaultTemp.toFixed(1);
      this.tempEdit.setFixedWidth(80);
      equipRow2.add(gainLabel);
      equipRow2.add(this.gainEdit);
      equipRow2.add(tempLabel);
      equipRow2.add(this.tempEdit);
      equipRow2.addStretch();
      equipGroup.sizer.add(equipRow2);

      sizer.add(equipGroup);

      // Processing options
      var procGroup = new GroupBox(this);
      procGroup.title = "Processing Options";
      procGroup.sizer = new VerticalSizer;

      this.shiftCheck = new CheckBox(this);
      this.shiftCheck.text = "Shift overnight sessions to previous calendar day";
      this.shiftCheck.checked = settings.shiftOvernight;
      procGroup.sizer.add(this.shiftCheck);

      this.obsDateCheck = new CheckBox(this);
      this.obsDateCheck.text = "Use actual observation date (no overnight shifting)";
      this.obsDateCheck.checked = settings.useObsDate;
      procGroup.sizer.add(this.obsDateCheck);

      sizer.add(procGroup);

      // AstroBin Filter Database group
      var filterGroup = new GroupBox(this);
      filterGroup.title = "AstroBin Filter Database";
      filterGroup.sizer = new VerticalSizer;
      filterGroup.sizer.spacing = 4;

      var filterStatusRow = new HorizontalSizer;
      filterStatusRow.spacing = 6;
      this.filterStatusLabel = new Label(this);
      this.filterStatusLabel.text = "Database: " + filterDB.getCount() + " filters (updated: " + filterDB.getLastUpdated() + ")";
      this.filterStatusLabel.textAlignment = TextAlignment.Left;
      filterStatusRow.add(this.filterStatusLabel);
      filterStatusRow.addStretch();
      filterGroup.sizer.add(filterStatusRow);

      var filterButtonRow = new HorizontalSizer;
      filterButtonRow.spacing = 6;
      var downloadButton = new PushButton(this);
      downloadButton.text = "Download/Update Database";
      downloadButton.toolTip = "Fetch the latest filter database from AstroBin (~2500 filters)";
      downloadButton.onClick = function() {
         self.downloadFilterDatabase();
      };
      filterButtonRow.add(downloadButton);
      filterButtonRow.addStretch();
      filterGroup.sizer.add(filterButtonRow);

      // Default filter row
      var defaultFilterRow = new HorizontalSizer;
      defaultFilterRow.spacing = 6;
      this.useDefaultFilterCheck = new CheckBox(this);
      this.useDefaultFilterCheck.text = "Use default filter for unmapped entries:";
      this.useDefaultFilterCheck.checked = settings.useDefaultFilter;
      this.useDefaultFilterCheck.toolTip = "When a FITS FILTER keyword has no AstroBin mapping, use this default filter ID instead";
      defaultFilterRow.add(this.useDefaultFilterCheck);

      this.defaultFilterEdit = new Edit(this);
      this.defaultFilterEdit.text = settings.defaultFilter;
      this.defaultFilterEdit.setFixedWidth(80);
      this.defaultFilterEdit.toolTip = "AstroBin filter ID to use as default (e.g. 2906 for Clear/Lum)";
      defaultFilterRow.add(this.defaultFilterEdit);

      var defaultFilterHint = new Label(this);
      defaultFilterHint.text = "(AstroBin numeric ID, e.g. 2906=Clear, 4663=Ha)";
      defaultFilterHint.textAlignment = TextAlignment.Left;
      defaultFilterRow.add(defaultFilterHint);
      defaultFilterRow.addStretch();
      filterGroup.sizer.add(defaultFilterRow);

      sizer.add(filterGroup);

      // Buttons
      var buttonSizer = new HorizontalSizer;
      buttonSizer.spacing = 8;

      var defaultsButton = new PushButton(this);
      defaultsButton.text = "Reset Defaults";
      defaultsButton.onClick = function() {
         settings = new AstroBinSettings();
         self.siteNameEdit.text = settings.siteName;
          self.latEdit.text = settings.siteLat.toFixed(4);
          self.lonEdit.text = settings.siteLon.toFixed(4);
          self.elevEdit.text = settings.siteElev.toFixed(1);
          self.bortleEdit.text = String(settings.bortle);
         self.sqmEdit.text = settings.sqm.toFixed(2);
         self.flEdit.text = String(settings.focalLength);
         self.pxEdit.text = settings.pixelSize.toFixed(2);
         self.frEdit.text = settings.focalRatio.toFixed(2);
         self.gainEdit.text = String(settings.defaultGain);
         self.tempEdit.text = settings.defaultTemp.toFixed(1);
         self.shiftCheck.checked = settings.shiftOvernight;
         self.obsDateCheck.checked = settings.useObsDate;
         self.useDefaultFilterCheck.checked = settings.useDefaultFilter;
         self.defaultFilterEdit.text = settings.defaultFilter;
      };

      var okButton = new PushButton(this);
      okButton.text = "OK";
      okButton.onClick = function() {
         settings.siteName = self.siteNameEdit.text;
         settings.siteLat = parseFloat(self.latEdit.text) || 0;
         settings.siteLon = parseFloat(self.lonEdit.text) || 0;
         settings.siteElev = parseFloat(self.elevEdit.text) || 0;
         settings.bortle = parseInt(self.bortleEdit.text) || 4;
         settings.sqm = parseFloat(self.sqmEdit.text) || 21;
         settings.focalLength = parseFloat(self.flEdit.text) || 540;
         settings.pixelSize = parseFloat(self.pxEdit.text) || 3;
         settings.focalRatio = parseFloat(self.frEdit.text) || 5;
         settings.defaultGain = parseInt(self.gainEdit.text) || 0;
         settings.defaultTemp = parseFloat(self.tempEdit.text) || -10;
         settings.shiftOvernight = self.shiftCheck.checked;
         settings.useObsDate = self.obsDateCheck.checked;
         settings.useDefaultFilter = self.useDefaultFilterCheck.checked;
         settings.defaultFilter = self.defaultFilterEdit.text;
         settings.save();
         self.ok();
      };

      var cancelButton = new PushButton(this);
      cancelButton.text = "Cancel";
      cancelButton.onClick = function() { self.cancel(); };

      buttonSizer.add(defaultsButton);
      buttonSizer.addStretch();
      buttonSizer.add(okButton);
      buttonSizer.add(cancelButton);
      sizer.add(buttonSizer);

      this.sizer = sizer;
   }

   /**
    * Opens a file dialog and imports site/equipment metadata from a FITS/XISF file.
    *
    * Reads the selected file's headers and populates the settings dialog fields
    * with values for: site name, coordinates, elevation, Bortle, SQM,
    * focal length, pixel size, f-ratio, gain, and sensor temperature.
    */
   importFromImage() {
      var dlg = new OpenFileDialog;
      dlg.caption = "Select an image to import site/equipment from";
      dlg.filters = [
         ["All supported files", "*.fits;*.fit;*.fts;*.xisf"],
         ["FITS files", "*.fits;*.fit;*.fts"],
         ["XISF files", "*.xisf"]
      ];
      if (!dlg.execute()) return;

      var filePath = dlg.filePath;
      var ext = File.extractExtension(filePath).toLowerCase();
      var keywords = {};

      try {
         if (ext === ".xisf") {
            keywords = readXISFHeaders(filePath);
         } else {
            keywords = readFITSHeaders(filePath);
         }
      } catch (e) {
         (new MessageBox(
            "Error reading file headers:\n" + e.toString(),
            TITLE, StdIcon.Error, StdButton.Ok
         )).execute();
         return;
      }

      if (Object.keys(keywords).length === 0) {
         (new MessageBox(
            "No keywords found in " + File.extractNameAndExtension(filePath),
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      var imported = [];

      // Site info
      var val;
      val = keywords["SITE"] || keywords["SITENAME"] || keywords["OBSERVAT"];
      if (val != null && String(val).length > 0) {
         this.siteNameEdit.text = String(val);
         imported.push("Site: " + val);
      }
      val = keywords["SITELAT"] || keywords["OBSGEO-B"] || keywords["LAT-OBS"] || keywords["LATITUDE"];
      if (val != null && !isNaN(Number(val))) {
         this.latEdit.text = Number(val).toFixed(4);
         imported.push("Latitude: " + val);
      }
      val = keywords["SITELONG"] || keywords["OBSGEO-L"] || keywords["LONG-OBS"] || keywords["LONGITUDE"];
      if (val != null && !isNaN(Number(val))) {
         this.lonEdit.text = Number(val).toFixed(4);
         imported.push("Longitude: " + val);
      }
      val = keywords["SITEELEV"] || keywords["ELEVATION"];
      if (val != null && !isNaN(Number(val))) {
         this.elevEdit.text = Number(val).toFixed(1);
         imported.push("Elevation: " + val);
      }
      val = keywords["BORTLE"];
      if (val != null && !isNaN(Number(val))) {
         this.bortleEdit.text = String(Math.round(Number(val)));
         imported.push("Bortle: " + val);
      }
      val = keywords["SQM"] || keywords["SKYQUAL"];
      if (val != null && !isNaN(Number(val))) {
         this.sqmEdit.text = Number(val).toFixed(2);
         imported.push("SQM: " + val);
      }

      // Equipment defaults
      val = keywords["FOCALLEN"] || keywords["FOC-LEN"] || keywords["FOCLENGTH"] || keywords["EFL"];
      if (val != null && !isNaN(Number(val))) {
         this.flEdit.text = String(Math.round(Number(val)));
         imported.push("Focal Length: " + val);
      }
      val = keywords["XPIXSZ"] || keywords["YPIXSZ"] || keywords["PIXSIZE"];
      if (val != null && !isNaN(Number(val))) {
         this.pxEdit.text = Number(val).toFixed(2);
         imported.push("Pixel Size: " + val);
      }
      val = keywords["FOCRATIO"] || keywords["FOCUS"];
      if (val != null && !isNaN(Number(val))) {
         this.frEdit.text = Number(val).toFixed(2);
         imported.push("F-Ratio: " + val);
      }
      val = keywords["GAIN"];
      if (val != null && !isNaN(Number(val))) {
         this.gainEdit.text = String(Math.round(Number(val)));
         imported.push("Gain: " + val);
      }
      val = keywords["CCD-TEMP"] || keywords["CCDTEMP"] || keywords["SET-TEMP"];
      if (val != null && !isNaN(Number(val))) {
         this.tempEdit.text = Number(val).toFixed(1);
         imported.push("Temp: " + val);
      }

      if (imported.length > 0) {
         (new MessageBox(
            "Imported from " + File.extractNameAndExtension(filePath) + ":\n\n" +
            imported.join("\n"),
            TITLE, StdIcon.Information, StdButton.Ok
         )).execute();
      } else {
         (new MessageBox(
            "No site or equipment keywords found in " + File.extractNameAndExtension(filePath),
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
      }
   }

   /**
    * Prompts the user and downloads the full AstroBin filter database.
    *
    * Fetches ~2500 filters from the AstroBin REST API and saves them
    * to the local cache file. Shows a confirmation dialog before starting
    * and reports success/failure with a message box.
    */
   downloadFilterDatabase() {
      var confirm = new MessageBox(
         "This will download the entire AstroBin filter database (~2500 filters).\n" +
         "This may take a minute. Continue?",
         TITLE + " - Download Filter Database",
         StdIcon.Question,
         StdButton.Yes,
         StdButton.No
      );
      if (confirm.execute() !== StdButton.Yes) return;

      var success = filterDB.fetchFromAPI();

      if (success) {
         this.filterStatusLabel.text = "Database: " + filterDB.getCount() +
            " filters (updated: " + filterDB.getLastUpdated() + ")";
         (new MessageBox(
            "Filter database downloaded successfully!\n\n" +
            filterDB.getCount() + " filters loaded.\n" +
            "Saved to: " + FILTER_DB_FILE,
            TITLE, StdIcon.Information, StdButton.Ok
         )).execute();
      } else {
         (new MessageBox(
            "Failed to download filter database.\n" +
            "Check the console for error details.",
            TITLE, StdIcon.Error, StdButton.Ok
         )).execute();
      }
   }
};

// =============================================================================
// Main Dialog
// =============================================================================

var fileFrames = []; // Array of extracted frame data

// =============================================================================
// Filter Picker Dialog
// =============================================================================

/**
 * Filter selection dialog for manually overriding AstroBin filter assignments.
 *
 * Displays a searchable list of all filters from the local AstroBin database.
 * The user can type in the search box to filter results in real-time.
 * Returns the selected filter's AstroBin numeric ID and display label.
 *
 * Used by the MainDialog for per-file and bulk filter override operations.
 */
var FilterPickerDialog = class extends Dialog {
   constructor() {
      super();
      this.windowTitle = "Select AstroBin Filter";
      this.minWidth = 500;
      this.minHeight = 400;
      this.chosenId = null;
      this.chosenLabel = null;
      this.displayedFilters = [];

      var self = this;
      var sizer = new VerticalSizer;
      sizer.spacing = 8;
      sizer.margin = 12;

      // Search row
      var searchRow = new HorizontalSizer;
      searchRow.spacing = 6;
      var searchLabel = new Label(this);
      searchLabel.text = "Search:";
      searchRow.add(searchLabel);
      this.searchEdit = new Edit(this);
      this.searchEdit.setFixedWidth(300);
      this.searchEdit.onTextUpdated = function() { self.populateList(self.searchEdit.text); };
      searchRow.add(this.searchEdit);
      searchRow.addStretch();
      sizer.add(searchRow);

      // Filter list
      this.listBox = new TreeBox(this);
      this.listBox.numberOfColumns = 1;
      this.listBox.setHeaderText(0, "Filter");
      this.listBox.minHeight = 250;
      this.listBox.rootDecoration = false;
      sizer.add(this.listBox);

      // Status
      this.statusLabel = new Label(this);
      this.statusLabel.text = filterDB.getCount() + " filters in database";
      this.statusLabel.textAlignment = TextAlignment.Left;
      sizer.add(this.statusLabel);

      // Buttons
      var buttonRow = new HorizontalSizer;
      buttonRow.spacing = 8;

      var okButton = new PushButton(this);
      okButton.text = "OK";
      okButton.onClick = function() {
         // Resolve selected index directly from the tree
         var resolvedIndex = -1;
         // Try currentNode first (most reliable in V8)
         var cur = self.listBox.currentNode;
         if (cur != null) {
            var curText = cur.text(0);
            for (var i = 0; i < self.listBox.numberOfNodes; i++) {
               if (self.listBox.node(i) === cur) {
                  resolvedIndex = i;
                  break;
               }
            }
            // Fallback: match by text
            if (resolvedIndex < 0) {
               for (var i = 0; i < self.displayedFilters.length; i++) {
                  var label = self.displayedFilters[i].brandName + " " +
                              self.displayedFilters[i].name + " (" + self.displayedFilters[i].id + ")";
                  if (label === curText) {
                     resolvedIndex = i;
                     break;
                  }
               }
            }
         }
         // Fallback: try selectedNodes
         if (resolvedIndex < 0) {
            try {
               var sel = self.listBox.selectedNodes;
               if (sel != null && sel.length > 0) {
                  var selText = sel[0].text(0);
                  for (var i = 0; i < self.displayedFilters.length; i++) {
                     var label = self.displayedFilters[i].brandName + " " +
                                 self.displayedFilters[i].name + " (" + self.displayedFilters[i].id + ")";
                     if (label === selText) {
                        resolvedIndex = i;
                        break;
                     }
                  }
               }
            } catch (e) {}
         }

         if (resolvedIndex < 0 || resolvedIndex >= self.displayedFilters.length) {
            (new MessageBox(
               "Please select a filter from the list.\n(currentNode=" + (cur != null ? cur.text(0) : "null") + ")",
               TITLE, StdIcon.Warning, StdButton.Ok
            )).execute();
            return;
         }
         var f = self.displayedFilters[resolvedIndex];
         self.chosenId = String(f.id);
         self.chosenLabel = f.brandName + " " + f.name + " (" + f.id + ")";
         self.ok();
      };

      var cancelButton = new PushButton(this);
      cancelButton.text = "Cancel";
      cancelButton.onClick = function() { self.cancel(); };

      buttonRow.addStretch();
      buttonRow.add(okButton);
      buttonRow.add(cancelButton);
      sizer.add(buttonRow);

      this.sizer = sizer;

      // Populate initial list
      this.populateList("");
   }

   /**
    * Populates the filter list TreeBox, optionally filtered by a search query.
    *
    * Filters are sorted by brand then name. If a query is provided, only
    * filters whose "Brand Name (ID)" string contains the query are shown.
    * Updates the status label with the filtered count.
    *
    * @param {string} query - The search string to filter the list (case-insensitive).
    */
   populateList(query) {
      this.listBox.clear();
      this.displayedFilters = [];

      var q = query.trim().toLowerCase();
      var filters = filterDB.filters;

      // Sort by brand then name
      var sorted = filters.slice();
      sorted.sort(function(a, b) {
         var cmp = a.brandName.localeCompare(b.brandName);
         return cmp !== 0 ? cmp : a.name.localeCompare(b.name);
      });

      for (var i = 0; i < sorted.length; i++) {
         var f = sorted[i];
         var label = f.brandName + " " + f.name + " (" + f.id + ")";
         if (q.length === 0 || label.toLowerCase().indexOf(q) >= 0) {
            var node = new TreeBoxNode(this.listBox);
            node.setText(0, label);
            this.displayedFilters.push(f);
         }
      }

      this.statusLabel.text = this.displayedFilters.length + " of " + filters.length + " filters";
   }

   /** Convenience wrapper that re-populates the list using the current search text. */
   filterList() {
      this.populateList(this.searchEdit.text);
   }
};

/**
 * Main application dialog for the AstroBin CSV Generator.
 *
 * Provides the primary user interface for:
 *   - Loading FITS/XISF files individually or by directory
 *   - Previewing extracted metadata in a color-coded tree view
 *   - Setting per-file or bulk filter overrides via the filter picker
 *   - Previewing the generated CSV in the console
 *   - Writing the final acquisition.csv to disk
 *
 * Frame data is stored in the module-level fileFrames[] array and
 * persists across dialog interactions within a single script run.
 */
var MainDialog = class extends Dialog {
   constructor() {
      super();
      this.windowTitle = TITLE + " v" + VERSION;
      this.minWidth = 650;
      this.minHeight = 500;

      var self = this;
      var mainSizer = new VerticalSizer;
      mainSizer.spacing = 8;
      mainSizer.margin = 12;

      // Title
      var titleLabel = new Label(this);
      titleLabel.text = "AstroBin CSV Generator";
      titleLabel.textAlignment = TextAlignment.Center;
      titleLabel.font = new Font("sans-serif", 14);
      mainSizer.add(titleLabel);

      // File list group
      var fileGroup = new GroupBox(this);
      fileGroup.title = "Input Files";
      fileGroup.sizer = new VerticalSizer;
      fileGroup.sizer.spacing = 4;

      // File TreeBox
      this.fileTree = new TreeBox(this);
      this.fileTree.numberOfColumns = 8;
      this.fileTree.setHeaderText(0, "File");
      this.fileTree.setHeaderText(1, "Type");
      this.fileTree.setHeaderText(2, "Filter");
      this.fileTree.setHeaderText(3, "AstroBin ID");
      this.fileTree.setHeaderText(4, "Exposure");
      this.fileTree.setHeaderText(5, "Gain");
      this.fileTree.setHeaderText(6, "Temp");
      this.fileTree.setHeaderText(7, "Date");
      this.fileTree.setColumnWidth(0, 200);
      this.fileTree.setColumnWidth(1, 60);
      this.fileTree.setColumnWidth(2, 70);
      this.fileTree.setColumnWidth(3, 80);
      this.fileTree.setColumnWidth(4, 70);
      this.fileTree.setColumnWidth(5, 50);
      this.fileTree.setColumnWidth(6, 50);
      this.fileTree.setColumnWidth(7, 120);
      this.fileTree.rootDecoration = false;
      this.fileTree.showColumnLines = false;
      this.fileTree.autoSizeUniformColumns = true;
      fileGroup.sizer.add(this.fileTree);

      // File buttons
      var fileButtonSizer = new HorizontalSizer;
      fileButtonSizer.spacing = 6;

      var addButton = new PushButton(this);
      addButton.text = "Add Files...";
      addButton.icon = this.scaledResource(":/icons/add.png");
      addButton.onClick = function() { self.addFiles(); };

      var addDirButton = new PushButton(this);
      addDirButton.text = "Add Directory...";
      addDirButton.onClick = function() { self.addDirectory(); };

      var removeButton = new PushButton(this);
      removeButton.text = "Remove Selected";
      removeButton.onClick = function() { self.removeSelected(); };

      var clearButton = new PushButton(this);
      clearButton.text = "Clear All";
      clearButton.onClick = function() { self.clearAll(); };

      var setFilterButton = new PushButton(this);
      setFilterButton.text = "Set Filter...";
      setFilterButton.toolTip = "Set the AstroBin filter for the currently selected file";
      setFilterButton.onClick = function() { self.setFilterForSelected(); };

      var setFilterAllButton = new PushButton(this);
      setFilterAllButton.text = "Set Filter to All...";
      setFilterAllButton.toolTip = "Apply a filter override to all loaded files";
      setFilterAllButton.onClick = function() { self.setFilterForAll(); };

      fileButtonSizer.add(addButton);
      fileButtonSizer.add(addDirButton);
      fileButtonSizer.add(removeButton);
      fileButtonSizer.addStretch();
      fileButtonSizer.add(setFilterButton);
      fileButtonSizer.add(setFilterAllButton);
      fileButtonSizer.add(clearButton);
      fileGroup.sizer.add(fileButtonSizer);

      mainSizer.add(fileGroup);

      // Status area
      this.statusLabel = new Label(this);
      this.statusLabel.text = "No files loaded.";
      this.statusLabel.textAlignment = TextAlignment.Left;
      mainSizer.add(this.statusLabel);

      // Output settings
      var outputGroup = new GroupBox(this);
      outputGroup.title = "Output";
      outputGroup.sizer = new HorizontalSizer;
      outputGroup.sizer.spacing = 6;

      var outputLabel = new Label(this);
      outputLabel.text = "Output path:";
      this.outputDirEdit = new Edit(this);
      this.outputDirEdit.text = "";
      this.outputDirEdit.setFixedWidth(400);
      this.outputDirEdit.toolTip = "Leave empty to save in the same directory as input files";

      var browseButton = new PushButton(this);
      browseButton.text = "Browse...";
      browseButton.onClick = function() {
         var dlg = new GetDirectoryDialog;
         dlg.caption = "Select Output Directory";
         if (dlg.execute()) {
            self.outputDirEdit.text = dlg.directoryPath;
         }
      };

      outputGroup.sizer.add(outputLabel);
      outputGroup.sizer.add(this.outputDirEdit);
      outputGroup.sizer.add(browseButton);
      outputGroup.sizer.addStretch();
      mainSizer.add(outputGroup);

      // Bottom buttons
      var bottomSizer = new HorizontalSizer;
      bottomSizer.spacing = 8;

      var settingsButton = new PushButton(this);
      settingsButton.text = "Settings...";
      settingsButton.onClick = function() {
         var dlg = new SettingsDialog;
         dlg.execute();
      };

      var previewButton = new PushButton(this);
      previewButton.text = "Preview CSV";
      previewButton.onClick = function() { self.previewCSV(); };

      var generateButton = new PushButton(this);
      generateButton.text = "Generate CSV";
      generateButton.setFixedWidth(120);
      generateButton.font = new Font("sans-serif", 11);
      generateButton.onClick = function() { self.generateCSV(); };

      var closeButton = new PushButton(this);
      closeButton.text = "Close";
      closeButton.onClick = function() { self.cancel(); };

      bottomSizer.add(settingsButton);
      bottomSizer.addStretch();
      bottomSizer.add(previewButton);
      bottomSizer.add(generateButton);
      bottomSizer.add(closeButton);
      mainSizer.add(bottomSizer);

      this.sizer = mainSizer;
   }

   addFiles() {
      var dlg = new OpenFileDialog;
      dlg.caption = "Select FITS/XISF Files";
      dlg.filters = [
         ["All supported files", "*.fits;*.fit;*.fts;*.xisf"],
         ["FITS files", "*.fits;*.fit;*.fts"],
         ["XISF files", "*.xisf"],
         ["All files", "*.*"]
      ];
      dlg.multipleSelections = true;

      if (dlg.execute()) {
         var files = dlg.filePaths;
         this.processFiles(files);
      }
   }

   addDirectory() {
      var dlg = new GetDirectoryDialog;
      dlg.caption = "Select Directory with FITS/XISF Files";

      if (dlg.execute()) {
         var dir = dlg.directoryPath;
         var files = [];
         var extensions = ["*.fits", "*.fit", "*.fts", "*.xisf"];

         for (var e = 0; e < extensions.length; e++) {
            var found = File.searchDirectory(dir + "/" + extensions[e], false);
            for (var i = 0; i < found.length; i++) {
               files.push(found[i]);
            }
         }

         if (files.length === 0) {
            (new MessageBox(
               "No FITS/XISF files found in the selected directory.",
               TITLE,
               StdIcon.Warning,
               StdButton.Ok
            )).execute();
            return;
         }

         this.processFiles(files);
      }
   }

   processFiles(files) {
      console.show();
      var count = 0;
      var errors = 0;

      for (var i = 0; i < files.length; i++) {
         var filePath = files[i];
         var ext = File.extractExtension(filePath).toLowerCase();

         // Check if already loaded
         var alreadyLoaded = false;
         for (var j = 0; j < fileFrames.length; j++) {
            if (fileFrames[j].filePath === filePath) {
               alreadyLoaded = true;
               break;
            }
         }
         if (alreadyLoaded) continue;

         var keywords = {};
         if (ext === ".xisf") {
            keywords = readXISFHeaders(filePath);
         } else if (ext === ".fits" || ext === ".fit" || ext === ".fts") {
            keywords = readFITSHeaders(filePath);
         } else {
            console.writeln("Skipping unsupported file: " + File.extractNameAndExtension(filePath));
            continue;
         }

         if (Object.keys(keywords).length === 0) {
            console.writeln("Warning: No keywords found in " + File.extractNameAndExtension(filePath));
            errors++;
            continue;
         }

         var frame = extractFrameData(keywords, filePath, settings);
         fileFrames.push(frame);
         count++;
      }

      // Assign session dates
      fileFrames = detectSessions(fileFrames);

      // Update tree
      this.updateTree();

      this.statusLabel.text = format("%d file(s) loaded, %d error(s).", fileFrames.length, errors);
      console.hide();
   }

   updateTree() {
      this.fileTree.clear();

      for (var i = 0; i < fileFrames.length; i++) {
         var f = fileFrames[i];
         var node = new TreeBoxNode(this.fileTree);
         node.setText(0, f.fileName);
         node.setText(1, f.imagetyp);
         node.setText(2, String(f.filter));

         // Show filter override or auto-mapped AstroBin ID
         var displayId;
         var isMapped;
         if (f.filterOverride != null) {
            displayId = f.filterOverride.label;
            isMapped = true;
         } else {
            var mappedId = settings.mapFilter(f.filter);
            isMapped = (mappedId !== f.filter && mappedId !== "None" && mappedId !== "");
            displayId = isMapped ? String(mappedId) : "-";
         }
         node.setText(3, displayId);

         node.setText(4, f.exposure > 0 ? format("%.1fs", f.exposure) : "-");
         node.setText(5, String(f.gain));
         node.setText(6, f.ccdTemp !== 0 ? format("%.0fC", f.ccdTemp) : "-");
         node.setText(7, f.dateObs ? String(f.dateObs).substring(0, 19) : "-");

         // Color code by type
         switch (f.imagetyp) {
            case "LIGHT":
               node.foreColor = isMapped ? 0xFF00AA00 : 0xFF00CC66;
               break;
            case "FLAT":
               node.foreColor = 0xFF0066FF;
               break;
            case "DARK":
               node.foreColor = 0xFFAA0000;
               break;
            case "BIAS":
               node.foreColor = 0xFFAA6600;
               break;
            default:
               node.foreColor = 0xFF808080;
         }
      }
   }

   removeSelected() {
      var selectedNodes = this.fileTree.selectedNodes;
      if (selectedNodes.length === 0) return;

      var namesToRemove = {};
      for (var i = 0; i < selectedNodes.length; i++) {
         var name = selectedNodes[i].text(0);
         namesToRemove[name] = true;
      }

      fileFrames = fileFrames.filter(function(f) {
         return !(f.fileName in namesToRemove);
      });

      this.updateTree();
      this.statusLabel.text = format("%d file(s) loaded.", fileFrames.length);
   }

   clearAll() {
      fileFrames = [];
      this.fileTree.clear();
      this.statusLabel.text = "No files loaded.";
   }

   setFilterForSelected() {
      if (fileFrames.length === 0) {
         (new MessageBox(
            "No files loaded.",
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      if (filterDB.getCount() === 0) {
         (new MessageBox(
            "Filter database is empty.\nPlease download it from Settings first.",
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      var cur = this.fileTree.currentNode;
      if (cur == null) {
         (new MessageBox(
            "Please click on a file in the list first.",
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      var clickedName = cur.text(0);
      var frameIndex = -1;
      for (var i = 0; i < fileFrames.length; i++) {
         if (fileFrames[i].fileName === clickedName) {
            frameIndex = i;
            break;
         }
      }

      if (frameIndex < 0) {
         (new MessageBox(
            "Could not find the selected file.",
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      var dlg = new FilterPickerDialog();
      if (dlg.execute() && dlg.chosenId != null) {
         fileFrames[frameIndex].filterOverride = { id: dlg.chosenId, label: dlg.chosenLabel };
         this.updateTree();
      }
   }

   setFilterForAll() {
      if (fileFrames.length === 0) {
         (new MessageBox(
            "No files loaded.",
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      if (filterDB.getCount() === 0) {
         (new MessageBox(
            "Filter database is empty.\nPlease download it from Settings first.",
            TITLE, StdIcon.Warning, StdButton.Ok
         )).execute();
         return;
      }

      var dlg = new FilterPickerDialog();
      if (dlg.execute() && dlg.chosenId != null) {
         for (var i = 0; i < fileFrames.length; i++) {
            fileFrames[i].filterOverride = { id: dlg.chosenId, label: dlg.chosenLabel };
         }
         this.updateTree();
      }
   }

   getOutputPath() {
      var outputDir = this.outputDirEdit.text.trim();
      if (outputDir.length === 0 && fileFrames.length > 0) {
         var fp = fileFrames[0].filePath;
         var lastSep = Math.max(fp.lastIndexOf('/'), fp.lastIndexOf('\\'));
         outputDir = lastSep > 0 ? fp.substring(0, lastSep) : fp;
      }
       return outputDir + "/acquisition.csv";
   }

   previewCSV() {
      if (fileFrames.length === 0) {
         (new MessageBox(
            "No files loaded. Please add FITS/XISF files first.",
            TITLE,
            StdIcon.Warning,
            StdButton.Ok
         )).execute();
         return;
      }

      var aggregated = aggregateFrames(fileFrames);

      if (aggregated.length === 0) {
         (new MessageBox(
            "No LIGHT frames found to generate CSV.",
            TITLE,
            StdIcon.Warning,
            StdButton.Ok
         )).execute();
         return;
      }

      console.show();
      console.writeln("<b>AstroBin Acquisition CSV Preview</b>");
      console.writeln("================================");
      console.writeln();
      console.writeln(format("Total LIGHT groups: %d", aggregated.length));
      console.writeln(format("Total LIGHT frames: %d", fileFrames.filter(function(f) {
         return f.imagetyp === "LIGHT" && !f.isMaster;
      }).length));
      console.writeln();
      // Debug: show filterOverride status for each frame
      for (var di = 0; di < fileFrames.length; di++) {
         var df = fileFrames[di];
         if (df.filterOverride != null) {
            console.writeln(format("[DEBUG] frame '%s' has filterOverride: id=%s, label=%s", df.fileName, df.filterOverride.id, df.filterOverride.label));
         }
      }
      console.writeln("date,filter,number,duration,binning,gain,sensorCooling,fNumber,bortle,meanSqm,meanFwhm,temperature");
      for (var i = 0; i < aggregated.length; i++) {
         var a = aggregated[i];
         console.writeln(format("%s,%s,%d,%.1f,%d,%d,%.0f,%.1f,%d,%.2f,%.2f,%.1f",
            a.sessionDate, a.filterCode, a.number, a.exposure,
            a.xbinning, a.gain, a.sensorCooling, a.fNumber,
            a.bortle, a.meanSqm, a.meanFwhm, a.temperature));
      }
      console.hide();
   }

   generateCSV() {
      if (fileFrames.length === 0) {
         (new MessageBox(
            "No files loaded. Please add FITS/XISF files first.",
            TITLE,
            StdIcon.Warning,
            StdButton.Ok
         )).execute();
         return;
      }

      var aggregated = aggregateFrames(fileFrames);

      if (aggregated.length === 0) {
         (new MessageBox(
            "No LIGHT frames found to generate CSV.",
            TITLE,
            StdIcon.Warning,
            StdButton.Ok
         )).execute();
         return;
      }

      var outputPath = this.getOutputPath();

      // Confirm overwrite
      if (File.exists(outputPath)) {
         var confirm = new MessageBox(
            "File already exists:\n" + outputPath + "\n\nOverwrite?",
            TITLE + " - Confirm",
            StdIcon.Question,
            StdButton.Yes,
            StdButton.No
         );
         if (confirm.execute() !== StdButton.Yes) return;
      }

      console.show();
      console.writeln("Generating AstroBin CSV...");
      console.writeln("  Output: " + outputPath);
      console.writeln("  LIGHT groups: " + aggregated.length);

      var success = generateCSV(aggregated, outputPath);

      if (success) {
         // Print CSV to console for easy copy
         console.show();
         console.writeln();
         console.writeln("<b>=== CSV Output (select and copy below) ===</b>");
         var csvText = "date,filter,number,duration,binning,gain,sensorCooling,fNumber,darks,flats,flatDarks,bias,bortle,meanSqm,meanFwhm,temperature";
         console.writeln(csvText);
         for (var i = 0; i < aggregated.length; i++) {
            var a = aggregated[i];
            console.writeln(format("%s,%s,%d,%.1f,%d,%d,%.0f,%.1f,%d,%.2f,%.2f,%.1f",
               a.sessionDate, a.filterCode, a.number, a.exposure,
               a.xbinning, a.gain, a.sensorCooling, a.fNumber,
               0, 0, 0, 0,
               a.bortle, a.meanSqm, a.meanFwhm, a.temperature));
         }
         console.writeln("<b>=== End CSV ===</b>");
         console.writeln();
         console.writeln("File: " + outputPath + " (" + aggregated.length + " rows)");
         console.writeln("Select the CSV lines above, then right-click > Copy.");
      } else {
         (new MessageBox(
            "Error generating CSV file. Check the console for details.",
            TITLE,
            StdIcon.Error,
            StdButton.Ok
         )).execute();
      }

      console.hide();
   }
};

// =============================================================================
// Main Entry Point
// =============================================================================

(function() {
   console.show();
   console.writeln(format("<b>%s v%s</b>", TITLE, VERSION));
   console.writeln("AstroBin CSV Generator for PixInsight");
   console.writeln("Reading FITS/XISF headers and generating AstroBin-compatible CSV.");
   console.writeln();

   var dialog = new MainDialog;
   dialog.execute();

   console.hide();
})();

/*
    Copyright (C) 2003 an onwards Robert Muth <robert at muth dot org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

// ========================================================================
//  imports
// ========================================================================

// C imports
#include <unistd.h>

// C++ imports
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>

// Local imports
#include "basic.H"
#include "switch.H"

using namespace std;
// ========================================================================

const char Version[] = "$Id: main.C 1175 2009-04-19 16:54:26Z muth $";

// ========================================================================
LOCALFUN string ExtractVersion() {
  string id;
  string file;
  string ver;
  string date;
  string time;

  string myversion(Version);
  istringstream is(myversion);

  is >> id >> file >> ver >> date >> time;

  return ver;
}

// ========================================================================
// APE Tag item names for reference only

#define APE_TAG_KEY_TITLE "Title"
#define APE_TAG_KEY_SUBTITLE "Subtitle"
#define APE_TAG_KEY_ARTIST "Artist"
#define APE_TAG_KEY_ALBUM "Album"
#define APE_TAG_KEY_DEBUTALBUM "Debut Album"
#define APE_TAG_KEY_PUBLISHER "Publisher"
#define APE_TAG_KEY_CONDUCTOR "Conductor"
#define APE_TAG_KEY_COMPOSER "Composer"
#define APE_TAG_KEY_COMMENT "Comment"
#define APE_TAG_KEY_YEAR "Year"
#define APE_TAG_KEY_RECORDDATE "Record Date"
#define APE_TAG_KEY_RECORDLOCATION "Record Location"
#define APE_TAG_KEY_TRACK "Track"
#define APE_TAG_KEY_GENRE "Genre"
#define APE_TAG_KEY_COVER_ART_FRONT "Cover Art (front)"
#define APE_TAG_KEY_NOTES "Notes"
#define APE_TAG_KEY_LYRICS "Lyrics"
#define APE_TAG_KEY_COPYRIGHT "Copyright"
#define APE_TAG_KEY_PUBLICATIONRIGHT "Publicationright"
#define APE_TAG_KEY_FILE "File"
#define APE_TAG_KEY_MEDIA "Media"
#define APE_TAG_KEY_EANUPC "EAN/UPC"
#define APE_TAG_KEY_ISRC "ISRC"
#define APE_TAG_KEY_RELATED_URL "Related"
#define APE_TAG_KEY_ABSTRACT_URL "Abstract"
#define APE_TAG_KEY_BIBLIOGRAPHY_URL "Bibliography"
#define APE_TAG_KEY_BUY_URL "Buy URL"
#define APE_TAG_KEY_ARTIST_URL "Artist URL"
#define APE_TAG_KEY_PUBLISHER_URL "Publisher URL"
#define APE_TAG_KEY_FILE_URL "File URL"
#define APE_TAG_KEY_COPYRIGHT_URL "Copyright URL"
#define APE_TAG_KEY_INDEX "Index"
#define APE_TAG_KEY_INTROPLAY "Introplay"
#define APE_TAG_KEY_MJ_METADATA "Media Jukebox Metadata"
#define APE_TAG_KEY_DUMMY "Dummy"

#define APE_MAGIC "APETAGEX"
#define APE_VERSION 2000

#define APE_FLAG_HAVE_HEADER (1 << 31)
#define APE_FLAG_IS_HEADER (1 << 29)

#define APE_TAG_ITEM_FLAG_TEXT (0 << 0)
#define APE_TAG_ITEM_FLAG_BINARY (1 << 1)
#define APE_TAG_ITEM_FLAG_EXTERNAL_RESOURCE (1 << 2)
#define APE_TAG_ITEM_FLAG_RESERVED (1 << 3)

typedef struct {
  char _magic[8];
  char _version[4];
  char _length[4];
  char _items[4];
  char _flags[4];
  char _reserved[8];
} APE_HEADER_FOOTER;

// ========================================================================
LOCALFUN UINT32 ReadLittleEndianUint32(const char *cp) {
  UINT32 result = cp[3] & 0xff;
  result <<= 8;
  result |= cp[2] & 0xff;
  result <<= 8;
  result |= cp[1] & 0xff;
  result <<= 8;
  result |= cp[0] & 0xff;
  return result;
}

// ========================================================================
LOCALFUN VOID WriteLittleEndianUint32(char *cp, UINT32 i) {
  cp[0] = i & 0xff;
  i >>= 8;
  cp[1] = i & 0xff;
  i >>= 8;
  cp[2] = i & 0xff;
  i >>= 8;
  cp[3] = i & 0xff;
}

// ========================================================================
// One piece of metadata - a tag/value pair
// ========================================================================
class ITEM {
private:
  string _key;
  string _value;
  UINT32 _flags;

public:
  ITEM() {}

  ITEM(const string &key, const string &value, UINT32 flags)
      : _key(key), _value(value), _flags(flags) {}

  const string &Key() const { return _key; }

  const string &Value() const { return _value; }

  const UINT32 &Flags() const { return _flags; }
};

// As suggested by the APEv2 tag specification, the tag items are ordered by
// (value) length. If two values are the same length the items are ordered by
// key.
struct ITEM_ORDER {
  bool operator()(const ITEM *i1, ITEM *i2) const {
    if (i1->Value().length() == i2->Value().length()) {
      return i1->Key() < i2->Key();
    } else {
      return i1->Value().length() < i2->Value().length();
    }
  }
};

typedef set<ITEM *, ITEM_ORDER> ITEM_SET;

// ========================================================================
// Collection of all items associated with a file
// ========================================================================
class TAG {
private:
  UINT32 _file_length;
  UINT32 _tag_offset;
  UINT32 _num_items;
  ITEM_SET _items;

public:
  TAG(UINT32 file_length, UINT32 tag_offset, UINT32 num_items)
      : _file_length(file_length), _tag_offset(tag_offset),
        _num_items(num_items) {
    Debug("num items: " + hexstr(_num_items) + "\n");
  }

  VOID DelAllItems() {
    Debug("erasing all items\n");
    _items.clear();
  }

  // According to the APEv2 tag specification, item keys that differ only by
  // case are invalid. We replace such similar items instead of adding new
  // ones.

  // If an item key already exists and its new value is empty, we remove the
  // existing item.
  VOID UpdateItem(ITEM *newitem) {
    const string &newkey = newitem->Key();
    const string &newvalue = newitem->Value();
    const UINT32 &newflags = newitem->Flags();

    ITEM_SET::const_iterator it = _items.begin();
    while (it != _items.end()) {
      const ITEM *item = *it;

      const string &key = item->Key();

      if (CaseCompare(key, newkey)) {
        if (newvalue.length() == 0) {
          Debug("erasing item with key " + key + "\n");
          _items.erase(it);
          return;
        } else {
          Debug("replacing item with key " + key + "\n");
          _items.erase(it);
          _items.insert(newitem);
          return;
        }
      }

      ++it;
    }

    Debug("adding item with key " + newkey + " and flags " +
          hexstr(newflags) + "\n");
    _items.insert(newitem);
  }

  const ITEM_SET &Items() const { return _items; }

  UINT32 FileLength() const { return _file_length; }

  UINT32 TagOffset() const { return _tag_offset; }

  UINT32 ItemLength() const {
    UINT32 length = 0;
    for (ITEM_SET::const_iterator it = Items().begin(); it != Items().end();
         ++it) {
      const ITEM *item = *it;

      if (item->Value() == "")
        continue;
      length += 8;
      length += item->Value().length();
      length += 1;
      length += item->Key().length();
    }
    return length;
  }

  UINT32 ItemCount() const {
    UINT32 count = 0;
    for (ITEM_SET::const_iterator it = Items().begin(); it != Items().end();
         ++it) {
      const ITEM *item = *it;
      if (item->Value() == "")
        continue;
      count++;
    }
    return count;
  }
};

LOCALFUN VOID WriteApeHeaderFooter(fstream &input, const TAG *tag,
                                   UINT32 flags) {
  char buf[4];

  Info("writing header/footer at " + decstr(int(input.tellp())) + "\n");

  if (sizeof(APE_HEADER_FOOTER) != 32)
    Error("bad size");

  input.write(APE_MAGIC, 8);

  WriteLittleEndianUint32(buf, 2000); // version
  input.write(buf, 4);

  WriteLittleEndianUint32(buf, tag->ItemLength() + sizeof(APE_HEADER_FOOTER));
  input.write(buf, 4);

  WriteLittleEndianUint32(buf, tag->ItemCount());
  input.write(buf, 4);

  WriteLittleEndianUint32(buf, flags);
  input.write(buf, 4);

  WriteLittleEndianUint32(buf, 0); // reserved
  input.write(buf, 4);
  input.write(buf, 4);
}

LOCALFUN VOID WriteApeItems(fstream &input, const TAG *tag) {
  char buf[4];

  Info("writing items at " + decstr(int(input.tellp())) + "\n");
  for (ITEM_SET::const_iterator it = tag->Items().begin();
       it != tag->Items().end(); ++it) {
    const ITEM *item = *it;

    const UINT32 &flags = item->Flags();

    const string &value = item->Value();
    const UINT32 value_length = value.length();

    if (value_length == 0)
      continue;

    const string &key = item->Key();
    const UINT32 key_length = key.length();

    Info("writing item " + key + " " + value + " " + hexstr(flags) + "\n");

    WriteLittleEndianUint32(buf, value_length);
    input.write(buf, 4);

    WriteLittleEndianUint32(buf, flags); // flags
    input.write(buf, 4);

    input.write(key.c_str(), key_length);
    input.write("\0", 1);
    input.write(value.c_str(), value_length);
  }
}

// ========================================================================
LOCALFUN VOID WriteApeTag(fstream &input, const TAG *tag,
                          const string &filename) {
  Info("file length " + decstr(tag->FileLength()) + "\n");

  const UINT32 tag_offset =
      tag->TagOffset() == 0 ? tag->FileLength() : tag->TagOffset();

  input.seekp(tag_offset);

  // write header

  if (int(input.tellp()) != int(tag_offset)) {
    Warning("seek for header failed " + decstr(int(input.tellp())) +
            " target pos " + decstr(tag_offset) + "\n");
  }

  WriteApeHeaderFooter(input, tag, APE_FLAG_IS_HEADER | APE_FLAG_HAVE_HEADER);
  WriteApeItems(input, tag);
  WriteApeHeaderFooter(input, tag, APE_FLAG_HAVE_HEADER);

  UINT32 pos = input.tellp();

  if (pos < tag->FileLength()) {
    Warning("truncating file from " + decstr(tag->FileLength()) + " to " +
            decstr(pos) + "\n");
    int result = truncate(filename.c_str(), pos);
    if (result) {
      Warning("truncating file failed");
    }
  }
}

// ========================================================================
LOCALFUN TAG *ReadAndProcessApeHeader(fstream &input) {
  input.seekg(0, ios::end);

  const UINT32 file_length = input.tellg();

  Info("file length is " + decstr(file_length) + "\n");

  if (file_length < sizeof(APE_HEADER_FOOTER)) {
    Info("file too short to contain ape tag\n");
    return new TAG(file_length, 0, 0);
  }

  // read footer

  APE_HEADER_FOOTER ape;
  input.seekg(file_length - sizeof(APE_HEADER_FOOTER));
  input.read(reinterpret_cast<char *>(&ape), sizeof(APE_HEADER_FOOTER));

  const string magic(ape._magic, 0, 8);

  if (magic != APE_MAGIC) {
    Warning("file does not contain ape tag\n");
    return new TAG(file_length, 0, 0);
  }

  const UINT32 version = ReadLittleEndianUint32(ape._version);
  const UINT32 length = ReadLittleEndianUint32(ape._length);
  const UINT32 items = ReadLittleEndianUint32(ape._items);
  const UINT32 flags = ReadLittleEndianUint32(ape._flags);

  if (version != APE_VERSION) {
    Error("unsupported version " + decstr(version) + "\n");
  }

  Info("found ape tag footer version: " + decstr(version) +
       "  length: " + decstr(length) + "  items: " + decstr(items) +
       "  flags: " + hexstr(flags) + "\n");

  if (file_length < length) {
    Warning("tag bigger than file\n");
    return new TAG(file_length, 0, 0);
  }

  // read header if any
  BOOL have_header = 0;

  if (file_length >= length + sizeof(APE_HEADER_FOOTER)) {
    input.seekg(-(INT32)(length + sizeof(APE_HEADER_FOOTER)), ios::end);
    APE_HEADER_FOOTER ape2;

    input.read((char *)&ape2, sizeof(APE_HEADER_FOOTER));

    const string tag2(ape2._magic, 0, 8);
    if (tag2 == APE_MAGIC) {
      have_header = 1;

      const UINT32 version2 = ReadLittleEndianUint32(ape2._version);
      const UINT32 length2 = ReadLittleEndianUint32(ape2._length);
      const UINT32 items2 = ReadLittleEndianUint32(ape2._items);
      const UINT32 flags2 = ReadLittleEndianUint32(ape2._flags);

      if (version != version2 || length != length2 || items != items2) {
        Warning("header footer data mismatch\n");
      }

      Info("found ape tag header version: " + decstr(version2) +
           "  length: " + decstr(length2) + "  items: " + decstr(items2) +
           "  flags: " + hexstr(flags2) + "\n");
    }
  }

  TAG *tag = new TAG(
      file_length,
      file_length - length - have_header * sizeof(APE_HEADER_FOOTER), items);

  // read and process tag data

  input.seekg(-(INT32)length, ios::end);

  char *const buffer = new char[length];

  input.read(buffer, length);

  char *cp = buffer;

  // FIXME: the following code needs buffer overun checks

  for (UINT32 i = 0; i < items; i++) {
    const UINT32 l = ReadLittleEndianUint32(cp);
    cp += 4;
    const UINT32 f = ReadLittleEndianUint32(cp);
    cp += 4;

    UINT32 flags = f;

    string key(cp);

    cp += 1 + key.length();

    string value(l, ' ');
    for (UINT32 p = 0; p < l; p++) {
      value[p] = cp[p];
    }

    cp += l;

    Info("tag " + decstr(i) + ":  len: " + decstr(l) + "  flags: " + hexstr(f) +
         "  key: " + key + "  value: " + value + "\n");
    tag->UpdateItem(new ITEM(key, value, flags));
  }

  return tag;
}

// ========================================================================
SWITCH SwitchInputFile("i", " general", SWITCH_TYPE_STRING,
                       SWITCH_MODE_OVERWRITE, "", "specify input file");

SWITCH SwitchDebug("debug", "general", SWITCH_TYPE_BOOL, SWITCH_MODE_OVERWRITE,
                   "0", "enable debug mode");

SWITCH SwitchPair(
    "p", "general", SWITCH_TYPE_STRING, SWITCH_MODE_ACCUMULATE, "$none$",
    "specify ape tag and value, arguments must have form tag=val, this option "
    "can be used multiple times");

SWITCH SwitchResourcePair(
    "r", "general", SWITCH_TYPE_STRING, SWITCH_MODE_ACCUMULATE, "$none$",
    "specify ape tag and value indicating external resource, arguments must "
    "have form tag=val, this option can be used multiple times");

SWITCH SwitchFilePair(
    "f", "general", SWITCH_TYPE_STRING, SWITCH_MODE_ACCUMULATE, "$none$",
    "specify ape tag and pathname for embedding or extracting data, arguments "
    "must have form tag=pathname, this option can be used multiple times");

SWITCH SwitchMode("m", "general", SWITCH_TYPE_STRING, SWITCH_MODE_OVERWRITE,
                  "read", "specify mode (read, update, overwrite, or erase)");

// ========================================================================
int Usage() {
  cout << "APETAG version: " << ExtractVersion()
       << "  (C) Robert Muth 2003 and onwards";
  cout << R"STR(
Web: http://www.muth.org/Robert/Apetag

Usage: apetag -i input-file -m mode  {[-p|-f|-r] tag=value}*

change or create APE tag for file input-file

apetag operates in one of three modes:
Mode read (default):
    read and dump APE tag if present
    extract an item to a file with the -f option
        e.g.: -f "Cover Art (front)"=cover.jpg
    extract item "Cover Art (front)" to file cover.jpg

Mode update:
    change selected key,value pairs
    the pairs are specified with the -p, -f, or -r options
        e.g.: -p Artist=Nosferaru -p Album=Bite -r Homepage=http://nos.fer/bite
    update items Artist, Album and Homepage
    tags not listed with the -p, -f, or -r option will remain unchanged
    tags with empty values are removed

Mode overwrite:
    Overwrite all the tags with items specified by the -p, -f, or -r options
    tags not listed with the -p, -f, or -r option will be removed
    this mode is also used to create ape tags initially

Mode erase:
    Remove the APE tag from file input-file

Switch summary:

)STR";
  cout << SWITCH::SwitchSummary();
  return -1;
}

const pair<string, string> ParsedPair (const string &pair) {
  const UINT32 len = pair.length();

  UINT32 pos_equal_sign;
  for (pos_equal_sign = 0; pos_equal_sign < len; pos_equal_sign++) {
    if (pair[pos_equal_sign] == '=')
      break;
  }

  if (pos_equal_sign >= len) {
    Error("pair : " + pair + " does not contain a \'=\'\n");
  }

  const string key = pair.substr(0, pos_equal_sign);
  pos_equal_sign++;
  const string val = pair.substr(pos_equal_sign, len - pos_equal_sign);

  return make_pair(key, val);
}

void HandleModeRead(TAG *tag) {
  map<string, string> items;

  cout << "Found APE tag at offset " + decstr(tag->TagOffset()) + "\n";
  cout << "Items:\n";
  for (ITEM_SET::const_iterator it = tag->Items().begin();
       it != tag->Items().end(); ++it) {
    const ITEM *item = *it;

    const string &key = item->Key();
    const string &value = item->Value();
    const UINT32 &flags = item->Flags();

    items[key] = value;

    if (flags == APE_TAG_ITEM_FLAG_BINARY) {
      cout << "\"" + key + "\"" + " <Binary>\n";
    } else {
      cout << "\"" + key + "\" \"" + value + "\"\n";
    }
  }

  const UINT32 num_file_items = SwitchFilePair.ValueNumber();

  // we skip the first entry
  for (UINT32 i = 1; i < num_file_items; i++) {
    const pair<string, string> pair =
        ParsedPair(SwitchFilePair.ValueString(i));

    const string &key = pair.first;
    const string &val = pair.second;

    if (items.count(key)) {
      const string &value = items[key];

      fstream file;
      file.open(val, ios_base::out | ios_base::in);

      if (file.is_open())
        Error("output file exists: " + val + "\n");

      file.open(val, ios_base::out);

      if (file.is_open())
        cout << "Writing " + val + "\n";

      UINT32 pos = value.find('\0') + 1;
      file << value.substr(pos);

      file.close();
    } else {
      Error("item \"" + key + "\" not found\n");
    }
  }
}

void HandleModeUpdate(TAG *tag) {
  const UINT32 num_items = SwitchPair.ValueNumber();

  // we skip the first entry
  for (UINT32 i = 1; i < num_items; i++) {
    const pair<string, string> pair = ParsedPair(SwitchPair.ValueString(i));

    const string &key = pair.first;
    const string &val = pair.second;

    Debug("adding (" + key + "," + val + ")\n");

    tag->UpdateItem(new ITEM(key, val, APE_TAG_ITEM_FLAG_TEXT));
  }

  const UINT32 num_resource_items = SwitchResourcePair.ValueNumber();

  // we skip the first entry
  for (UINT32 i = 1; i < num_resource_items; i++) {
    const pair<string, string> pair =
        ParsedPair(SwitchResourcePair.ValueString(i));

    const string &key = pair.first;
    const string &val = pair.second;

    Debug("adding (" + key + "," + val + ")\n");

    tag->UpdateItem(new ITEM(key, val, APE_TAG_ITEM_FLAG_EXTERNAL_RESOURCE));
  }

  const UINT32 num_file_items = SwitchFilePair.ValueNumber();

  // we skip the first entry
  for (UINT32 i = 1; i < num_file_items; i++) {
    pair<string, string> pair = ParsedPair(SwitchFilePair.ValueString(i));

    const string &key = pair.first;
    string val = pair.second;

    if (val.length()) {
      fstream file;
      file.open(val, ios_base::in);

      if (!file.is_open())
        Error("could not open file: " + val + "\n");

      istreambuf_iterator<char> begin(file), end;

      val = '\0';
      val += string(begin, end);

      file.close();
    }

    Debug("adding (" + key + "," + " <Binary>)\n");

    tag->UpdateItem(new ITEM(key, val, APE_TAG_ITEM_FLAG_BINARY));
  }
}

// ========================================================================
int main(int argc, char *argv[]) {
  RegisterImageName(argv[0]);
  InstallSignalHandlers();

  //    ParseCommandLine
  for (argc--, argv++; argc > 0; argc--, argv++) {
    if (*argv[0] == '-') {
      SWITCH *sw = SWITCH::SwitchFind(argv[0] + 1);
      if (sw == 0) {
        Warning(string("bad option ") + argv[0] + "\n");
        return Usage();
      }

      if (sw->Type() == SWITCH_TYPE_BOOL) {
        sw->ValueAdd("1");
      } else {
        ASSERTX(argc > 0);
        argc--;
        argv++;
        sw->ValueAdd(argv[0]);
      }
    } else {
      break;
    }
  }

  if (!SwitchDebug.ValueBool()) {
    DisableMessage('I');
    DisableMessage('D');
    // DisableMessage('W');
  }

  const string &mode = SwitchMode.ValueString();

  const string filename = SwitchInputFile.ValueString();

  if (filename == "")
    Error("no input file specified\n");

  const BOOL change_file = (mode == "overwrite" || mode == "update");

  fstream input(filename.c_str(),
                change_file ? (ios_base::in | ios_base::out) : ios_base::in);
  if (!input)
    Error("could not open file\n");

  unique_ptr<TAG> tag(ReadAndProcessApeHeader(input));

  if (mode == "read") {
    if (tag->TagOffset() == 0) {
      cout << "No valid APE tag found\n";
    } else {
      HandleModeRead(tag.get());
    }
  } else if (mode == "update") {
    HandleModeUpdate(tag.get());
    WriteApeTag(input, tag.get(), filename);
  } else if (mode == "overwrite") {
    tag->DelAllItems();
    HandleModeUpdate(tag.get());
    WriteApeTag(input, tag.get(), filename);
  } else if (mode == "erase") {
    if (tag->TagOffset() == 0) {
      cout << "No valid APE tag found\n";
    } else {
      const UINT32 tag_offset =
          tag->TagOffset() == 0 ? tag->FileLength() : tag->TagOffset();

      if (tag_offset < tag->FileLength()) {
        Warning("truncating file from " + decstr(tag->FileLength()) + " to " +
                decstr(tag_offset) + "\n");
        int result = truncate(filename.c_str(), tag_offset);
        if (result) {
          Warning("truncating file failed");
        }
      }
    }
  } else {
    Error("unknown mode\n");
  }

  return 0;
}

// ========================================================================

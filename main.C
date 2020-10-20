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

#define APE_FLAG_READWRITE (0 << 0)
#define APE_FLAG_READONLY (1 << 0)

#define APE_TAG_ITEM_FLAG_TEXT (0 << 0)
#define APE_TAG_ITEM_FLAG_BINARY (1 << 1)
#define APE_TAG_ITEM_FLAG_EXTERNAL_RESOURCE (1 << 2)
#define APE_TAG_ITEM_FLAG_RESERVED (1 << 3)

#define ID3V1_MAGIC "TAG"

typedef struct {
  char _magic[8];
  char _version[4];
  char _length[4];
  char _items[4];
  char _flags[4];
  char _reserved[8];
} APE_HEADER_FOOTER;

typedef struct {
  char _magic[3];
  char _tag[125];
} ID3v1_TAG;

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
  bool operator()(const ITEM *i1, const ITEM *i2) const {
    if (i1->Value().length() == i2->Value().length()) {
      return i1->Key() < i2->Key();
    } else {
      return i1->Value().length() < i2->Value().length();
    }
  }
};

typedef set<const ITEM *, ITEM_ORDER> ITEM_SET;

// ========================================================================
// Collection of all items associated with a file
// ========================================================================
class TAG {
private:
  UINT32 _file_length;
  UINT32 _tag_offset;
  UINT32 _num_items;
  ITEM_SET _items;
  UINT32 _flags;

public:
  TAG(UINT32 file_length, UINT32 tag_offset, UINT32 num_items, UINT32 flags)
      : _file_length(file_length), _tag_offset(tag_offset),
        _num_items(num_items), _flags(flags) {
    Debug("num items: " + hexstr(_num_items) + "\n");
  }

  VOID DelAllItems() {
    Debug("erasing all items\n");

    ITEM_SET items;

    ITEM_SET::const_iterator it = _items.begin();
    while (it != _items.end()) {
      const ITEM *item = *it;

      const string &key = item->Key();
      const UINT32 &flags = item->Flags();

      if ((flags & APE_FLAG_READONLY) == APE_FLAG_READONLY) {
        Warning("read only item with key " + key + " was not " +
                "erased\n");
        items.insert(item);
      }

      ++it;
    }

    _items.clear();
    _items = items;
  }

  // According to the APEv2 tag specification, item keys that differ only by
  // case are invalid. We replace such similar items instead of adding new
  // ones.

  // If an item key already exists and its new value is empty, we remove the
  // existing item.
  VOID UpdateItem(const ITEM *newitem) {
    const string &newkey = newitem->Key();
    const string &newvalue = newitem->Value();
    const UINT32 &newflags = newitem->Flags();

    const ITEM *item = FindItem(newkey);

    if (item->Key().size()) {
      const string &key = item->Key();
      const string &value = item->Value();
      const UINT32 &flags = item->Flags();

      if (((newvalue != value) && (newflags != flags)) && ((flags &
          APE_FLAG_READONLY) == APE_FLAG_READONLY)) {
        Warning("read only item with key " + key + " was not modified\n");
        return;
      }

      if (newvalue.length() == 0) {
        Debug("erasing item with key " + key + "\n");
        _items.erase(item);
        return;
      } else {
        Debug("replacing item with key " + key + "\n");
        _items.erase(item);
        _items.insert(newitem);
        return;
      }
    }

    Debug("adding item with key " + newkey + " and flags " +
          hexstr(newflags) + "\n");
    _items.insert(newitem);
  }

  const ITEM *FindItem(const string &findkey) const {
    ITEM_SET::const_iterator it = _items.begin();
    while (it != _items.end()) {
      const ITEM *item = *it;

      const string &key = item->Key();

      if (CaseCompare(key, findkey)) {
        return item;
      }

      ++it;
    }

    return new ITEM();
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

  VOID SetFlags(const UINT32 &flags) {
     Debug("setting tag with flags " + hexstr(flags) + "\n");
    _flags = flags;
  }

  UINT32 Flags() const {
    return _flags;
  }
};

LOCALFUN VOID WriteApeHeaderFooter(fstream &input, const TAG *tag,
                                   UINT32 flags) {
  char buf[4];

  Info("writing header/footer at " + decstr(int(input.tellp())) + " flags: " +
       hexstr(flags) + "\n");

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

    Info("writing item " + key + " " +
         (flags == APE_TAG_ITEM_FLAG_BINARY ? "<Embedded Binary>" : value) +
         " " + hexstr(flags) + "\n");

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
LOCALFUN VOID WriteId3v1(fstream &input, const ID3v1_TAG &tag) {
  Info("writing id3v1 tag at " + decstr(int(input.tellp())) + "\n");

  input.write(tag._magic, 3);
  input.write(tag._tag, sizeof(ID3v1_TAG) - 3);
}

// ========================================================================
LOCALFUN VOID WriteApeTag(fstream &input, const TAG *tag) {
  Info("file length " + decstr(tag->FileLength()) + "\n");

  const UINT32 tag_offset =
      tag->TagOffset() == 0 ? tag->FileLength() : tag->TagOffset();

  const UINT32 &flags = tag->Flags();

  input.seekp(tag_offset);

  // write header

  if (int(input.tellp()) != int(tag_offset)) {
    Warning("seek for header failed " + decstr(int(input.tellp())) +
            " target pos " + decstr(tag_offset) + "\n");
  }

  WriteApeHeaderFooter(input, tag, flags | APE_FLAG_IS_HEADER | APE_FLAG_HAVE_HEADER);
  WriteApeItems(input, tag);
  WriteApeHeaderFooter(input, tag, flags | APE_FLAG_HAVE_HEADER);
}

LOCALFUN VOID Truncate(fstream &input, const TAG *tag,
                       const string &filename) {
  UINT32 pos = input.tellp();

  if (pos < tag->FileLength()) {
    Info("truncating file from " + decstr(tag->FileLength()) + " to " +
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
    return new TAG(file_length, 0, 0, 0);
  }

  // The APEv2 specification says that the APEv2 tag, when placed at the end of
  // a file, must be placed after the last frame and before any ID3v1 tag.

  UINT32 offset = sizeof(ID3v1_TAG);

  ID3v1_TAG id3v1tag;

  input.seekg(file_length - offset);
  input.read(reinterpret_cast<char *>(&id3v1tag), sizeof(ID3v1_TAG));

  const string id3magic(id3v1tag._magic, 0, 3);

  if (id3magic == ID3V1_MAGIC) {
    Info("file contains an id3v1 tag at " +
         decstr(file_length - offset) + "\n");
  } else {
    offset = 0;
  }

  // read footer
  APE_HEADER_FOOTER ape;
  input.seekg(file_length - offset - sizeof(APE_HEADER_FOOTER));
  input.read(reinterpret_cast<char *>(&ape), sizeof(APE_HEADER_FOOTER));

  const string magic(ape._magic, 0, 8);

  if (magic != APE_MAGIC) {
    Info("file does not contain ape tag\n");
    return new TAG(file_length, file_length - offset, 0, 0);
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
    return new TAG(file_length, 0, 0, 0);
  }

  // read header if any
  BOOL have_header = 0;

  UINT32 tagflags = 0;

  if (file_length >= (length + sizeof(APE_HEADER_FOOTER) + offset)) {
    input.seekg(-(INT32)(length + sizeof(APE_HEADER_FOOTER) +
                offset), ios::end);
    APE_HEADER_FOOTER ape2;

    input.read((char *)&ape2, sizeof(APE_HEADER_FOOTER));

    const string tag2(ape2._magic, 0, 8);
    if (tag2 == APE_MAGIC) {
      have_header = 1;

      const UINT32 version2 = ReadLittleEndianUint32(ape2._version);
      const UINT32 length2 = ReadLittleEndianUint32(ape2._length);
      const UINT32 items2 = ReadLittleEndianUint32(ape2._items);
      const UINT32 flags2 = ReadLittleEndianUint32(ape2._flags);

      tagflags = flags2;
      tagflags &= ~APE_FLAG_HAVE_HEADER;
      tagflags &= ~APE_FLAG_IS_HEADER;

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
      file_length - length - offset - have_header * sizeof(APE_HEADER_FOOTER),
      items, tagflags);

  // read and process tag data

  input.seekg(-(INT32)(length + offset), ios::end);

  INT32 items_length = length - sizeof(APE_HEADER_FOOTER);

  const char *tag_items = new char[items_length];
  input.read((char *)tag_items, items_length);

  for (UINT32 i = 0; i < items; i++) {
    const UINT32 l = ReadLittleEndianUint32(tag_items);
    tag_items += 4;
    const UINT32 f = ReadLittleEndianUint32(tag_items);
    tag_items += 4;

    UINT32 flags = f;

    string key(tag_items);

    tag_items += 1 + key.length();

    string value(tag_items, l);

    tag_items += l;

    Info("tag " + decstr(i) + ":  len: " + decstr(l) + "  flags: " + hexstr(f) +
         "  key: " + key + " value: " +
         (flags == APE_TAG_ITEM_FLAG_BINARY ? "<Embedded Binary>" : value) +
         "\n");
    tag->UpdateItem(new ITEM(key, value, flags));

    items_length -= 4 + 4 + 1 + key.length() + value.length();
  }

  if (items_length != 0) {
    Warning("items size mismatch\n");
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
                  "read", "specify mode (read, update, overwrite, erase or "
                  "setro/setrw)");

SWITCH SwitchRo("ro", "general", SWITCH_TYPE_STRING, SWITCH_MODE_ACCUMULATE,
                  "$none$", "specify ape tag to set read only");

SWITCH SwitchRw("rw", "general", SWITCH_TYPE_STRING, SWITCH_MODE_ACCUMULATE,
                  "$none$", "specify ape tag to set read write");

SWITCH SwitchFile("file", "general", SWITCH_TYPE_STRING, SWITCH_MODE_OVERWRITE,
                  "", "specify pathname for ape-tagged import file");

// ========================================================================
int Usage() {
  cout << "APETAG version: " << ExtractVersion()
       << "  (C) Robert Muth 2003 and onwards";
  cout << R"STR(
Web: http://www.muth.org/Robert/Apetag

Usage: apetag -i input-file -m mode  {[-p|-f|-r] tag=value}*
Or: apetag -i input-file -m {update} {[-rw|-ro] tag}
Or: apetag -i input-file -m {overwrite} {-file import-file}

change or create APE tag for file input-file

apetag operates in one of three modes:
Mode read (default):
    read APE tag if present
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
    tags set read only will remain unchanged

Mode overwrite:
    Overwrite all the tags with items specified by the -p, -f, or -r options
    tags not listed with the -p, -f, or -r option will be removed
    replace the tags with items from an ape-tagged file with the -file option
    tags set read only will remain unchanged
    this mode is also used to create ape tags initially

Mode erase:
    Remove the APE tag from file input-file

Mode setro|setrw:
    Set the APE tag read only or read write
        e.g.: setro
    set the ape tag read only

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

BOOL ValidKey (const string &key) {
    // The APEv2 specification does not allow the following for keys
    if (key == "ID3" || key == "TAG" || key == "OggS" ||
        key == "MP+") {
      Warning("key \"" + key + "\" is not an allowed key\n");
      return false;
    }

    return true;
}

void HandleModeRead(TAG *tag) {
  map<string, string> items;

  cout << "Found APE tag at offset " + decstr(tag->TagOffset()) + "\n";
  if ((tag->Flags() & APE_FLAG_READONLY) == APE_FLAG_READONLY) {
    cout << "TAG IS SET READ ONLY\n";
  }
  cout << "Items:\n";
  cout << "Type\tRO/RW\tField\tValue\n";
  for (ITEM_SET::const_iterator it = tag->Items().begin();
       it != tag->Items().end(); ++it) {
    const ITEM *item = *it;

    const string &key = item->Key();
    const string &value = item->Value();
    const UINT32 &flags = item->Flags();

    items[key] = value;

    string dumpitem;
    string lockflag;
    if ((~flags & APE_FLAG_READONLY) == APE_FLAG_READONLY) {
      lockflag = "RW\t";
    } else {
      lockflag = "RO\t";
    }

    if ((flags & APE_TAG_ITEM_FLAG_EXTERNAL_RESOURCE) == APE_TAG_ITEM_FLAG_EXTERNAL_RESOURCE) {
        dumpitem += "RSC\t" + lockflag;
        dumpitem += key + "\t" + value;
    } else if ((flags & APE_TAG_ITEM_FLAG_BINARY) == APE_TAG_ITEM_FLAG_BINARY) {
        dumpitem += "BIN\t" + lockflag;
        dumpitem += key;
    } else if ((flags & APE_TAG_ITEM_FLAG_TEXT) == APE_TAG_ITEM_FLAG_TEXT) {
        dumpitem += "TXT\t" + lockflag;
        dumpitem += key + "\t" + value;
    }

    cout << dumpitem + "\n";
  }

  const UINT32 num_file_items = SwitchFilePair.ValueNumber();

  // there is no switch 0, start at switch 1
  for (UINT32 i = 1; i < num_file_items; i++) {
    const pair<string, string> pair =
        ParsedPair(SwitchFilePair.ValueString(i));

    const string &key = pair.first;
    const string &val = pair.second;

    if (items.count(key)) {
      const string &value = items[key];

      if (ifstream(val.c_str()).good()) {
        Error("output file exists: " + val + "\n");
      }

      ofstream file(val.c_str());

      if (!file.is_open())
        Error("could not open file: " + val + "\n");

      UINT32 pos = value.find('\0') + 1;
      file << value.substr(pos);

      file.close();
    } else {
      Error("item \"" + key + "\" not found\n");
    }
  }
}

void HandleModeUpdate(TAG *tag) {
  const UINT32 num_rw_items = SwitchRw.ValueNumber();

  // there is no switch 0, start at switch 1
  for (UINT32 i = 1; i < num_rw_items; i++) {
    const string &key = SwitchRw.ValueString(i);

    Debug("setting (" + key + ") read write\n");

    const ITEM *item = tag->FindItem(key);
    if (item->Value().size()) {
      UINT32 flags = item->Flags();
      flags &= ~APE_FLAG_READONLY;
      tag->UpdateItem(new ITEM(item->Key(), item->Value(), flags));
    } else {
      Warning("item \"" + key + "\" not found\n");
    }
  }

  const UINT32 num_utf8_items = SwitchPair.ValueNumber();

  // there is no switch 0, start at switch 1
  for (UINT32 i = 1; i < num_utf8_items; i++) {
    const pair<string, string> pair = ParsedPair(SwitchPair.ValueString(i));

    const string &key = pair.first;
    const string &val = pair.second;

    if (!ValidKey(key)) {
        Warning("skipping invalid key \"" + key + "\"\n");
        continue;
    }

    Debug("adding (" + key + "," + val + ")\n");

    tag->UpdateItem(new ITEM(key, val, APE_TAG_ITEM_FLAG_TEXT));
  }

  const UINT32 num_resource_items = SwitchResourcePair.ValueNumber();

  // there is no switch 0, start at switch 1
  for (UINT32 i = 1; i < num_resource_items; i++) {
    const pair<string, string> pair =
        ParsedPair(SwitchResourcePair.ValueString(i));

    const string &key = pair.first;
    const string &val = pair.second;

    if (!ValidKey(key)) {
        Warning("skipping invalid key \"" + key + "\"\n");
        continue;
    }

    Debug("adding (" + key + "," + val + ")\n");

    tag->UpdateItem(new ITEM(key, val, APE_TAG_ITEM_FLAG_EXTERNAL_RESOURCE));
  }

  const UINT32 num_file_items = SwitchFilePair.ValueNumber();

  // there is no switch 0, start at switch 1
  for (UINT32 i = 1; i < num_file_items; i++) {
    pair<string, string> pair = ParsedPair(SwitchFilePair.ValueString(i));

    const string &key = pair.first;
    string val = pair.second;

    if (!ValidKey(key)) {
        Warning("skipping invalid key \"" + key + "\"\n");
        continue;
    }

    if (val.length()) {
      ifstream file(val.c_str());

      if (!file.is_open())
        Error("could not open file: " + val + "\n");

      istreambuf_iterator<char> begin(file), end;

      val = '\0';
      val += string(begin, end);

      file.close();
    }

    Debug("adding (" + key + "," + " <Embedded Binary>)\n");

    tag->UpdateItem(new ITEM(key, val, APE_TAG_ITEM_FLAG_BINARY));
  }

  const UINT32 num_ro_items = SwitchRo.ValueNumber();

  // there is no switch 0, start at switch 1
  for (UINT32 i = 1; i < num_ro_items; i++) {
    const string &key = SwitchRo.ValueString(i);

    Debug("setting (" + key + ") read only\n");

    const ITEM *item = tag->FindItem(key);
    if (item->Value().size()) {
      UINT32 flags = item->Flags();
      flags |= APE_FLAG_READONLY;
      tag->UpdateItem(new ITEM(item->Key(), item->Value(), flags));
    } else {
      Warning("item \"" + key + "\" not found\n");
    }
  }
}

void HandleRoRw(TAG *tag, const BOOL &ro) {
  if (ro) {
    Debug("setting tag read only\n");

    UINT32 flags = tag->Flags();
    flags |= APE_FLAG_READONLY;
    tag->SetFlags(flags);
  } else {
    Debug("setting tag read write\n");

    UINT32 flags = tag->Flags();
    flags &= ~APE_FLAG_READONLY;
    tag->SetFlags(flags);
  }
}

void HandleTagImport (fstream &input, TAG *tag) {
  const string &infile = SwitchFile.ValueString();
  fstream in(infile.c_str(), ios_base::in);

  if (!in.is_open()) {
    Error("could not open file: " + infile + "\n");
  } else {
    Info("successfully opened file " + infile + "\n");
  }

  TAG *intag = ReadAndProcessApeHeader(in);

  TAG *offsettag = new TAG(tag->FileLength(), tag->TagOffset(),
                           intag->ItemCount(), intag->Flags());

  ITEM_SET::const_iterator it = intag->Items().begin();
  while (it != intag->Items().end()) {
    offsettag->UpdateItem(*it);
    ++it;
  }

  WriteApeTag(input, offsettag);
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

  const BOOL change_file = (mode == "overwrite" || mode == "update"
                            || mode == "erase" || mode == "setro"
                            || mode == "setrw");

  fstream input(filename.c_str(),
                change_file ? (ios_base::in | ios_base::out) : ios_base::in);
  if (!input) {
    Error("could not open file\n");
  } else {
    Info("successfully opened file " + filename + "\n");
  }

  unique_ptr<TAG> tag(ReadAndProcessApeHeader(input));

  const UINT32 id3_offset = tag->FileLength() - sizeof(ID3v1_TAG);

  ID3v1_TAG id3v1tag;

  input.seekg(id3_offset);
  input.read(reinterpret_cast<char *>(&id3v1tag), sizeof(ID3v1_TAG));

  const string id3magic(id3v1tag._magic, 0, 3);

  const BOOL has_apetag = (!((tag->TagOffset() == id3_offset)
                       && (id3magic == ID3V1_MAGIC))
                       && (tag->TagOffset() != tag->FileLength()));

  if (mode == "read") {
    if (!has_apetag) {
      cout << "No valid APE tag found\n";
    } else {
      HandleModeRead(tag.get());
    }
  } else if (mode == "update") {
    if ((tag->Flags() & APE_FLAG_READONLY) == APE_FLAG_READONLY) {
      Error("tag is read only\n");
    } else {
      HandleModeUpdate(tag.get());
      WriteApeTag(input, tag.get());
      if (id3magic == ID3V1_MAGIC)
        WriteId3v1(input, id3v1tag);
      Truncate(input, tag.get(), filename);
    }
  } else if (mode == "overwrite") {
    if ((tag->Flags() & APE_FLAG_READONLY) == APE_FLAG_READONLY) {
      Error("tag is read only\n");
    } else {
      if (SwitchFile.ValueString().size()) {
        HandleTagImport(input, tag.get());
      } else {
        tag->DelAllItems();
        HandleModeUpdate(tag.get());
        WriteApeTag(input, tag.get());
      }
      if (id3magic == ID3V1_MAGIC)
        WriteId3v1(input, id3v1tag);
      Truncate(input, tag.get(), filename);
    }
  } else if (mode == "erase") {
    if (!has_apetag) {
      cout << "No valid APE tag found\n";
    } else {
      input.seekp(tag->TagOffset());
      if (id3magic == ID3V1_MAGIC)
        WriteId3v1(input, id3v1tag);
      Truncate(input, tag.get(), filename);
    }
  } else if (mode == "setro" || mode == "setrw") {
    if (!has_apetag) {
      cout << "No valid APE tag found\n";
    } else {
      HandleRoRw(tag.get(), (mode == "setro"));
      WriteApeTag(input, tag.get());
      if (id3magic == ID3V1_MAGIC)
        WriteId3v1(input, id3v1tag);
      Truncate(input, tag.get(), filename);
    }
  } else {
    Error("unknown mode\n");
  }

  return 0;
}

// ========================================================================

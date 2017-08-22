/*
 *      Copyright (C) 2017 Brent Murphy <bmurphy@bcmcs.net>
 *      https://github.com/fuzzard/pvr.hdhomerun
 *
 *      Copyright (C) 2015 Zoltan Csizmadia <zcsizmadia@gmail.com>
 *      https://github.com/zcsizmadia/pvr.hdhomerun
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "EPG_XML.h"
#include "HDHomeRunTuners.h"
#include "rapidxml/rapidxml.hpp"
#include "Utils.h"
#include "zlib.h"

using namespace ADDON;
using namespace rapidxml;
//using namespace EPG_XML;

EPG_XML::EPG_XML()
{
}

/*
 * This method uses zlib to decompress a gzipped file in memory.
 * Author: Andrew Lim Chong Liang
 * http://windrealm.org
 */
bool EPG_XML::GzipInflate(const String &compressedBytes, String &uncompressedBytes)
{

  #define HANDLE_CALL_ZLIB(status) {   \
    if(status != Z_OK) {        \
      free(uncomp);             \
      return false;             \
    }                           \
  }

  if ( compressedBytes.size() == 0 )
  {
    uncompressedBytes = compressedBytes ;
    return true ;
  }

  uncompressedBytes.clear() ;

  unsigned full_length = compressedBytes.size() ;
  unsigned half_length = compressedBytes.size() / 2;

  unsigned uncompLength = full_length ;
  char* uncomp = (char*) calloc( sizeof(char), uncompLength );

  z_stream strm;
  strm.next_in = (Bytef *) compressedBytes.c_str();
  strm.avail_in = compressedBytes.size() ;
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false ;

  HANDLE_CALL_ZLIB(inflateInit2(&strm, (16+MAX_WBITS)));

  while (!done)
  {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength )
    {
      // Increase size of output buffer
      uncomp = (char *) realloc(uncomp, uncompLength + half_length);
      if (uncomp == NULL)
        return false;
      uncompLength += half_length ;
    }

    strm.next_out = (Bytef *) (uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate (&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK)
    {
      break;
    }
  }

  HANDLE_CALL_ZLIB(inflateEnd (&strm));

  for ( size_t i=0; i<strm.total_out; ++i )
  {
    uncompressedBytes += uncomp[ i ];
  }

  free( uncomp );
  return true ;
}

bool EPG_XML::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, String xmltvlocation)
{
  String strXMLlocation, strXMLdata, decompressed;
  char *buffer;

  KODI_LOG(LOG_DEBUG, "xmltvlocation '%s", xmltvlocation.c_str());

  if (GetFileContents(xmltvlocation, strXMLdata))
  {
    KODI_LOG(LOG_DEBUG, "Read file: %s", xmltvlocation.c_str());
    // gzip packed
    if (strXMLdata[0] == '\x1F' && strXMLdata[1] == '\x8B' && strXMLdata[2] == '\x08')
    {
      if (!EPG_XML::GzipInflate(strXMLdata, decompressed))
      {
        KODI_LOG(LOG_DEBUG, "Invalid EPG file '%s': unable to decompress file.", xmltvlocation.c_str());
        return false;
      }
      KODI_LOG(LOG_DEBUG, "Decompressed file: %s", xmltvlocation.c_str());
      buffer = &(decompressed[0]);
    
    }
    else
    {
      KODI_LOG(LOG_DEBUG, "File not gz compressed.");
      buffer = &(strXMLdata[0]);
    }
    if (buffer[0] != '\x3C' || buffer[1] != '\x3F' || buffer[2] != '\x78' ||
      buffer[3] != '\x6D' || buffer[4] != '\x6C')
    {
    // check for BOM
      if (buffer[0] != '\xEF' || buffer[1] != '\xBB' || buffer[2] != '\xBF')
      {
        // check for tar archive
        if (strcmp(buffer + 0x101, "ustar") || strcmp(buffer + 0x101, "GNUtar"))
          buffer += 0x200; // RECORDSIZE = 512
        else
        {
          KODI_LOG(LOG_DEBUG, "Invalid EPG file '%s': unable to parse file.", xmltvlocation.c_str());
          return false;
        }
      }
    }
    else
    {
      KODI_LOG(LOG_DEBUG, "Invalid EPG file '%s': unable to parse file.",  xmltvlocation.c_str());
      return false;
    }
    EPG_XML::_xmlparse(buffer);
  }
  return true;
}

bool EPG_XML::_xmlparse(char *xmlbuffer)
{
  xml_document<> xmlDoc;
  try
  {
    xmlDoc.parse<0>(xmlbuffer);
  }
  catch(parse_error p)
  {
    KODI_LOG(LOG_DEBUG, "Unable parse EPG XML: %s", p.what());
    return false;
  }

  xml_node<> *pRootElement = xmlDoc.first_node("tv");
  KODI_LOG(LOG_DEBUG, "Name of my first node is: %s", xmlDoc.first_node()->name());
  if (!pRootElement)
  {
    KODI_LOG(LOG_ERROR, "Invalid EPG XML: no <tv> tag found");
    return false;
  }
  return true;
}

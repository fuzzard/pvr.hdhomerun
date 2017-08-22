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

#include <json/json.h>
#include <string.h>

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

/*
 * Next two methods pulled from iptvsimple
 * Author: Anton Fedchin
 * Original: http://github.com/afedchin/xbmc-addon-iptvsimple/
 */

template<class Ch>
inline bool GetNodeValue(const xml_node<Ch> * pRootNode, const char* strTag, std::string& strStringValue)
{
  xml_node<Ch> *pChildNode = pRootNode->first_node(strTag);
  if (pChildNode == NULL)
  {
    return false;
  }
  strStringValue = pChildNode->value();
  return true;
}

template<class Ch>
inline bool GetAttributeValue(const xml_node<Ch> * pNode, const char* strAttributeName, std::string& strStringValue)
{
  xml_attribute<Ch> *pAttribute = pNode->first_attribute(strAttributeName);
  if (pAttribute == NULL)
  {
    return false;
  }
  strStringValue = pAttribute->value();
  return true;
}

bool EPG_XML::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, String xmltvlocation)
{
  String strXMLlocation, strXMLdata, decompressed;

  if (GetFileContents(xmltvlocation, strXMLdata))
  {
    // gzip packed
    if (strXMLdata.substr(0,3) == "\x1F\x8B\x08")
    {
      if (!EPG_XML::GzipInflate(strXMLdata, decompressed))
      {
        KODI_LOG(LOG_DEBUG, "Invalid EPG file '%s': unable to decompress file.", xmltvlocation.c_str());
        return false;
      }
      strXMLdata = decompressed;
    }
    if (!(strXMLdata.substr(0,5) == "<?xml"))
    {
      KODI_LOG(LOG_DEBUG, "Invalid EPG file: %s",  xmltvlocation.c_str());
      return false;
    }
    char *xmlbuffer = new char[strXMLdata.size() + 1];
    strcpy(xmlbuffer, strXMLdata.c_str());
    //EPG_XML::_xmlparse(pTuner, xmlbuffer);
  }
  return true;
}

bool EPG_XML::_xmlparse(HDHomeRunTuners::Tuner *pTuner, char *xmlbuffer)
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
  if (!pRootElement)
  {
    KODI_LOG(LOG_DEBUG, "Invalid EPG XML: no <tv> tag found");
    return false;
  }
  
  EPG_XML::_xmlparseelement(pTuner, pRootElement, "channel");

  /*pRootElement = xmlDoc.first_node("programme");
  if (!pRootElement)
  {
    KODI_LOG(LOG_DEBUG, "Invalid EPG XML: no <programme> tag found");
    return false;
  }
  EPG_XML::_xmlparseelement(pTuner, pRootElement, "programme");
*/
  return true;
}

bool EPG_XML::_xmlparseelement(HDHomeRunTuners::Tuner *pTuner,const xml_node<> *pRootNode, const char *strElement)
{
  Json::Value::ArrayIndex nIndex, nCount, nGuideIndex;
  xml_node<> *pChannelNode = NULL;

  for(pChannelNode = pRootNode->first_node(strElement); pChannelNode; pChannelNode = pChannelNode->next_sibling(strElement))
  {
    std::string strName;
    std::string strId;

    if (strcmp(strElement, "channel") == 0)
    {
      if(!GetAttributeValue(pChannelNode, "id", strId))
        continue;
      GetNodeValue(pChannelNode, "display-name", strName);
      KODI_LOG(LOG_DEBUG, "ID: %s", strId.c_str());
      KODI_LOG(LOG_DEBUG, "DisplayName: %s", strName.c_str());
      xml_node<> *pChannelLCN = NULL;

      for(pChannelLCN = pChannelNode->first_node("LCN"); pChannelLCN; pChannelLCN = pChannelLCN->next_sibling("LCN"))
      {
        KODI_LOG(LOG_DEBUG, "Channel Name: %s LCN: %s", strName.c_str(), pChannelLCN->value());
      }
    }
    else if (strcmp(strElement, "programme") == 0)
    {
    }
    else
    {
      KODI_LOG(LOG_DEBUG, "element parse fail");
      return false;
    }
  }
  return true;
}

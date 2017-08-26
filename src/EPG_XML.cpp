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

#include "EPG.h"
#include "EPG_XML.h"
#include "HDHomeRunTuners.h"
#include "rapidxml/rapidxml.hpp"
#include "Utils.h"
#include "zlib.h"

#include <json/json.h>
#include <string.h>
#include <ctime>
#include <vector>

using namespace ADDON;
using namespace rapidxml;
//using namespace EPG_XML;

REGISTER_CLASS("XML", EPG_XML);

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
 * Author: Anton Fedchin http://github.com/afedchin/xbmc-addon-iptvsimple/
 * Author: Pulse-Eight http://www.pulse-eight.com/
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

/*
 *  UpdateGuide()
 *  Accepts a string pointing to an xml or gz compacted xml file. Can be local or remote
 *  Parses xmltv formated file and populates pTuner->Guide with data.
 *
 */
bool EPG_XML::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, String xmltvlocation)
{
  String strXMLlocation, strXMLdata, decompressed;

  if (pTuner->Guide.size() < 1)
  {
    EPG_XML::_prepareGuide(pTuner);
  }

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
    // data starts as an expected xml file
    // Potentially look at using another xml library to run a proper verification on the
    // file. Not important to this development at this stage however.
    if (!(strXMLdata.substr(0,5) == "<?xml"))
    {
      KODI_LOG(LOG_DEBUG, "Invalid EPG file: %s",  xmltvlocation.c_str());
      return false;
    }
    char *xmlbuffer = new char[strXMLdata.size() + 1];
    strcpy(xmlbuffer, strXMLdata.c_str());
    if (!EPG_XML::_xmlparse(pTuner, xmlbuffer))
      return false;
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
  
  if (!EPG_XML::_xmlparseelement(pTuner, pRootElement, "channel"))
    return false;
  if (!EPG_XML::_xmlparseelement(pTuner, pRootElement, "programme"))
    return false;

  return true;
}

bool EPG_XML::_xmlparseelement(HDHomeRunTuners::Tuner *pTuner, const xml_node<> *pRootNode, const char *strElement)
{
  Json::Value::ArrayIndex nCount = 0;
  xml_node<> *pChannelNode = NULL;
  String strPreviousId = "";

  for(pChannelNode = pRootNode->first_node(strElement); pChannelNode; pChannelNode = pChannelNode->next_sibling(strElement))
  {
    std::string strName, strTitle, strSynopsis;
    std::string strId, strStartTime, strEndTime;
    int tempSeriesId = 0;
    if (strcmp(strElement, "channel") == 0)
    {
      if(!GetAttributeValue(pChannelNode, "id", strId))
        continue;
      GetNodeValue(pChannelNode, "display-name", strName);

      xml_node<> *pChannelLCN = NULL;
      std::vector<Json::Value*> vGuide;
      for(pChannelLCN = pChannelNode->first_node("LCN"); pChannelLCN; pChannelLCN = pChannelLCN->next_sibling("LCN"))
      {
        Json::Value& jsonChannel = EPG_XML::findJsonValue(pTuner->Guide, "GuideNumber", pChannelLCN->value());
        Json::Value* jsonChannelPointer = &jsonChannel;
        vGuide.push_back(jsonChannelPointer);
      }
      channelMap[strId] = vGuide;
    }
    else if (strcmp(strElement, "programme") == 0)
    {
      // ToDo: Check other xml providers to see if any other data can be mapped
      //<episode-num system="xmltv_ns">4.14.</episode-num>
      if(!GetAttributeValue(pChannelNode, "channel", strId))
        continue;
      std::vector<Json::Value*> vGuide = channelMap[strId];
      for(std::vector<Json::Value*>::iterator it = vGuide.begin(); it != vGuide.end(); ++it) {
        Json::Value* jsonChannelPointer = *it;
        Json::Value& jsonChannel = *jsonChannelPointer;

        GetAttributeValue(pChannelNode, "start", strStartTime);
        GetAttributeValue(pChannelNode, "stop", strEndTime);
        GetNodeValue(pChannelNode, "title", strTitle);
        GetNodeValue(pChannelNode, "desc", strSynopsis);
        jsonChannel["Guide"][nCount]["StartTime"] = EPG_XML::ParseDateTime(strStartTime);
        jsonChannel["Guide"][nCount]["EndTime"] = EPG_XML::ParseDateTime(strEndTime);
        jsonChannel["Guide"][nCount]["Title"] = strTitle;
        jsonChannel["Guide"][nCount]["Synopsis"] = strSynopsis;
        jsonChannel["Guide"][nCount]["OriginalAirdate"] = 0;
        jsonChannel["Guide"][nCount]["ImageURL"] = "";
        jsonChannel["Guide"][nCount]["SeriesID"] = tempSeriesId;

        // Look at alternative for an actual series id
        // maybe other xml providers supply this as well
        tempSeriesId++;
      }
      // ToDo: Add filter that contains genres
      //jsonChannel["Guide"][nCount]["Filter"] = Json::Value(Json::arrayValue);
    }
    else
    {
      KODI_LOG(LOG_DEBUG, "element parse fail");
      return false;
    }
  }
  return true;
}

/*
 *  Create basic layout of Guide json for each channel listed by LineUp
 *
 */
void EPG_XML::_prepareGuide(HDHomeRunTuners::Tuner *pTuner)
{
  Json::Value::ArrayIndex nIndex, nCount;
  for (nIndex = 0, nCount = 0; nIndex < pTuner->LineUp.size(); nIndex++)
  {
    pTuner->Guide[nIndex]["GuideNumber"] = pTuner->LineUp[nIndex]["GuideNumber"];
    pTuner->Guide[nIndex]["GuideName"] = pTuner->LineUp[nIndex]["GuideName"];
    pTuner->Guide[nIndex]["ImageURL"] = "";
    pTuner->Guide[nIndex]["Guide"] = Json::Value(Json::arrayValue);
  }
}

Json::Value& EPG_XML::findJsonValue(Json::Value &Guide, String jsonElement, String searchData)
{
  Json::Value::ArrayIndex nIndex, nCount, nGuideIndex;

  for (nIndex = 0, nCount = 0; nIndex < Guide.size(); nIndex++)
  {
    Json::Value& jsonGuide = Guide[nIndex];
    if (strcmp(jsonGuide[jsonElement].asString().c_str(), searchData.c_str()) == 0)
    {
      return jsonGuide;
    }
  }

// Need to work out appropriate return type
 // return (const&)Json::Value(Json::arrayValue);
}

/*
 * ParseDateTime pulled from and iptvsimple and adapted slightly
 * Author: Anton Fedchin
 * Author: Pulse-Eight http://www.pulse-eight.com/
 * Original: http://github.com/afedchin/xbmc-addon-iptvsimple/
 */

int EPG_XML::ParseDateTime(std::string& strDate)
{
  struct tm timeinfo;
  memset(&timeinfo, 0, sizeof(tm));
  char sign = '+';
  int hours = 0;
  int minutes = 0;

  sscanf(strDate.c_str(), "%04d%02d%02d%02d%02d%02d %c%02d%02d", &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec, &sign, &hours, &minutes);

  timeinfo.tm_mon  -= 1;
  timeinfo.tm_year -= 1900;
  timeinfo.tm_isdst = -1;

  time_t current_time;
  time(&current_time);
  long offset = 0;
#ifndef TARGET_WINDOWS
  offset = -localtime(&current_time)->tm_gmtoff;
#else
  _get_timezone(&offset);
#endif // TARGET_WINDOWS

  long offset_of_date = (hours * 60 * 60) + (minutes * 60);
  if (sign == '-')
  {
    offset_of_date = -offset_of_date;
  }

  return mktime(&timeinfo) - offset_of_date - offset;
}

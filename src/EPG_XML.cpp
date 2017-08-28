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
#include "zlib.h"

#include <ctime>

using namespace ADDON;
using namespace rapidxml;

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
  Json::Value::ArrayIndex nIndex;
  KODI_LOG(LOG_DEBUG, "Starting XMLTV Guide Update: %s", xmltvlocation.c_str());
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
    // ToDo: Potentially look at using another xml library to run a proper verification
    //       on the file. Not important to this development at this stage however.
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

  EPG_XML::_duplicateChannelCheck(pTuner);
  for (nIndex = 0; nIndex < pTuner->Guide.size(); nIndex++)
    EPGBase::addguideinfo(pTuner->Guide[nIndex]["Guide"]);

  KODI_LOG(LOG_DEBUG, "Finished XMLTV Guide Update");
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
  KODI_LOG(LOG_DEBUG, "Parsing EPG XML: <channel>");
  if (!EPG_XML::_xmlparseelement(pTuner, pRootElement, "channel"))
    return false;
  KODI_LOG(LOG_DEBUG, "Parsing EPG XML: <programme>");
  if (!EPG_XML::_xmlparseelement(pTuner, pRootElement, "programme"))
    return false;

  return true;
}

void EPG_XML::_duplicateChannelCheck(HDHomeRunTuners::Tuner *pTuner)
{
  Json::Value::ArrayIndex nIndex = 0, nIndex2 = 0;
  for (nIndex = 0; nIndex < pTuner->Guide.size(); nIndex++)
  {
    Json::Value& jsonGuideName = pTuner->Guide[nIndex]["GuideName"];
    if (pTuner->Guide[nIndex]["Guide"].size() == 0)
    {
      for (nIndex2 = 0; nIndex2 < pTuner->Guide.size(); nIndex2++)
      {
        if (nIndex2 == nIndex)
          continue;
        if (pTuner->Guide[nIndex2]["Guide"].size() == 0)
          continue;
        if (strcmp(jsonGuideName.asString().c_str(),pTuner->Guide[nIndex2]["GuideName"].asString().c_str()) == 0)
        {
          Json::Value& jsonGuideCurrent = pTuner->Guide[nIndex]["Guide"];
          Json::Value jsonGuideCopy = pTuner->Guide[nIndex2]["Guide"];
          jsonGuideCurrent = jsonGuideCopy;
        }
      }
    }
  }
}

bool EPG_XML::_xmlparseelement(HDHomeRunTuners::Tuner *pTuner, const xml_node<> *pRootNode, const char *strElement)
{
  Json::Value::ArrayIndex nCount = 0;
  xml_node<> *pChannelNode = NULL;
  String strPreviousId = "";
  int iStartTime = 0, iEndTime = 0;

  for(pChannelNode = pRootNode->first_node(strElement); pChannelNode; pChannelNode = pChannelNode->next_sibling(strElement))
  {
    std::string strName, strTitle, strSynopsis;
    std::string strId, strStartTime, strEndTime;
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
        if (jsonChannel.isNull())
          return false;
        Json::Value* jsonChannelPointer = &jsonChannel;
        vGuide.push_back(jsonChannelPointer);
      }
      channelMap[strId] = vGuide;
    }
    else if (strcmp(strElement, "programme") == 0)
    {
      if(!GetAttributeValue(pChannelNode, "channel", strId))
        continue;

      int iYear = 0;
      String strEpData = "", strEpName = "", strCategory = "", strActors = "", strDirectors = "", strYear = "", strEpNumSystem = "";
      Json::Value jsonFilter = Json::Value(Json::arrayValue);
      xml_node<> *pProgrammeCategory = NULL, *pProgrammeActors = NULL, *pProgrammeCredits = NULL, *pProgrammeDirectors = NULL;

      if (strPreviousId.empty())
      {
        strPreviousId = strId;
      }
      std::vector<Json::Value*> vGuide = channelMap[strId];

      GetAttributeValue(pChannelNode, "start", strStartTime);
      GetAttributeValue(pChannelNode, "stop", strEndTime);
      iStartTime = EPG_XML::ParseDateTime(strStartTime);
      iEndTime = EPG_XML::ParseDateTime(strEndTime);
      GetNodeValue(pChannelNode, "title", strTitle);
      GetNodeValue(pChannelNode, "desc", strSynopsis);
      if(pChannelNode->first_node("episode-num"))
        // only support xmltv_ns numbering scheme at this stage
        GetAttributeValue(pChannelNode->first_node("episode-num"), "system", strEpNumSystem);
        if (strcmp(strEpNumSystem.c_str(), "xmltv_ns") == 0)
          GetNodeValue(pChannelNode, "episode-num", strEpData);
      if(pChannelNode->first_node("sub-title"))
        GetNodeValue(pChannelNode, "sub-title", strEpName);
      if(pChannelNode->first_node("date"))
      {
        GetNodeValue(pChannelNode, "date", strYear);
        iYear = std::stoi(strYear);
      }

      if(pChannelNode->first_node("category"))
      {
        for(pProgrammeCategory = pChannelNode->first_node("category"); pProgrammeCategory; pProgrammeCategory = pProgrammeCategory->next_sibling("category"))
        {
          strCategory = pProgrammeCategory->value();
          jsonFilter.append(strCategory);
        }
      }
      if(pChannelNode->first_node("credits"))
      {
        pProgrammeCredits = pChannelNode->first_node("credits");
        for(pProgrammeActors = pProgrammeCredits->first_node("actor"); pProgrammeActors; pProgrammeActors = pProgrammeActors->next_sibling("actor"))
        {
          if (strActors.empty())
          {
            strActors = strActors + pProgrammeActors->value();
          }
          else
          {
            strActors = strActors + ", " + pProgrammeActors->value();
          }
        }
        for(pProgrammeDirectors = pChannelNode->first_node("credits")->first_node("director"); pProgrammeDirectors; pProgrammeDirectors = pProgrammeDirectors->next_sibling("director"))
        {
          if (strDirectors.empty())
          {
            strDirectors = strDirectors + pProgrammeDirectors->value();
          }
          else
          {
            strDirectors = strDirectors + ", " + pProgrammeDirectors->value();
          }
        }
      }

      for(std::vector<Json::Value*>::iterator it = vGuide.begin(); it != vGuide.end(); ++it) {
        Json::Value* jsonChannelPointer = *it;
        Json::Value& jsonChannel = *jsonChannelPointer;
        // ToDo: imageurl
        //       SeriesID

        jsonChannel["Guide"][nCount]["StartTime"] = iStartTime;
        jsonChannel["Guide"][nCount]["EndTime"] = iEndTime;
        jsonChannel["Guide"][nCount]["Title"] = strTitle;
        if (!strEpName.empty())
          jsonChannel["Guide"][nCount]["EpisodeTitle"] = strEpName;
        if (!strEpData.empty())
          jsonChannel["Guide"][nCount]["EpisodeNumber"] = strEpData;
        jsonChannel["Guide"][nCount]["Synopsis"] = strSynopsis;
        jsonChannel["Guide"][nCount]["OriginalAirdate"] = 0; // not implemented
        jsonChannel["Guide"][nCount]["ImageURL"] = "";  // not implemented
        jsonChannel["Guide"][nCount]["SeriesID"] = tempSeriesId; // not implemented
        jsonChannel["Guide"][nCount]["Filter"] = jsonFilter;
        if (!(iYear == 0))
          jsonChannel["Guide"][nCount]["Year"] = iYear;
        if (!strActors.empty())
          jsonChannel["Guide"][nCount]["Cast"] = strActors;
        if (!strDirectors.empty())
          jsonChannel["Guide"][nCount]["Director"] = strDirectors;
        // Look at alternative for an actual series id
        // maybe other xml providers supply this as well
        tempSeriesId++;
      }
      if (strPreviousId ==  strId)
      {
        nCount++;
      }
      else
      {
        nCount = 0;
        strPreviousId = strId;
      }
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
  Json::Value::ArrayIndex nIndex;
  for (nIndex = 0; nIndex < pTuner->LineUp.size(); nIndex++)
  {
    pTuner->Guide[nIndex]["GuideNumber"] = pTuner->LineUp[nIndex]["GuideNumber"];
    pTuner->Guide[nIndex]["GuideName"] = pTuner->LineUp[nIndex]["GuideName"];
    pTuner->Guide[nIndex]["ImageURL"] = "";
    pTuner->Guide[nIndex]["Guide"] = Json::Value(Json::arrayValue);
  }
}

Json::Value& EPG_XML::findJsonValue(Json::Value &Guide, String jsonElement, String searchData)
{
  Json::Value::ArrayIndex nIndex;

  for (nIndex = 0; nIndex < Guide.size(); nIndex++)
  {
    Json::Value& jsonGuide = Guide[nIndex];
    if (strcmp(jsonGuide[jsonElement].asString().c_str(), searchData.c_str()) == 0)
    {
      return jsonGuide;
    }
  }
  return jsonNull;
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

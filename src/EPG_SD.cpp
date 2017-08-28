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

#include "EPG_SD.h"

#include "client.h"
#include "Utils.h"

using namespace ADDON;

REGISTER_CLASS("SD", EPG_SD);

bool EPG_SD::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, String advancedguide)
{
  String strUrl;

  strUrl = StringUtils::Format("http://my.hdhomerun.com/api/guide.php?DeviceAuth=%s", EncodeURL(pTuner->Device.device_auth).c_str());

  if (advancedguide == "AG")
  {
  // No guide Exists, so pull basic guide data to populate epg guide initially
    if (pTuner->Guide.size() < 1)
    {
      EPG_SD::_UpdateBasicGuide(pTuner, strUrl);
      return true;
    }
    else
    {
      if (EPG_SD::_UpdateAdvancedGuide(pTuner, strUrl))
      {
        return true;
      }
    }
  }
  else
  {
    if (EPG_SD::_UpdateBasicGuide(pTuner, strUrl))
    {
      return true;
    }
  }
  return false;
}

bool EPG_SD::_UpdateAdvancedGuide(HDHomeRunTuners::Tuner *pTuner, String strUrl)
{
  Json::Value::ArrayIndex nIndex;
  String strJson, strUrlExtended, strJsonExtended;
  Json::Reader jsonReader;
  bool exitExtend;
  unsigned long long endTime;
  Json::Value tempGuide;

  KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun Extended guide: %s", strUrl.c_str());

  if (pTuner->Guide.type() == Json::arrayValue)
  {
    //Loop each Guide
    for (nIndex = 0; nIndex < pTuner->Guide.size(); nIndex++)
    {
      exitExtend = false;
      Json::Value& jsonGuide = pTuner->Guide[nIndex]["Guide"];
      endTime = EPG_SD::_getEndTime(jsonGuide);

      do 
      {
        strUrlExtended = StringUtils::Format("%s&Channel=%s&Start=%llu", strUrl.c_str() , pTuner->Guide[nIndex]["GuideNumber"].asString().c_str(), endTime);
        GetFileContents(strUrlExtended.c_str(), strJsonExtended);
        if (strJsonExtended.substr(0,4) == "null" ) 
        {
          exitExtend = true;
        }
        else
        {
          if (jsonReader.parse(strJsonExtended, tempGuide) &&
            tempGuide.type() == Json::arrayValue)
          {
            EPG_SD::_insert_guide_data(pTuner->Guide[nIndex]["Guide"], tempGuide);
            endTime = EPG_SD::_getEndTime(pTuner->Guide[nIndex]["Guide"]);
          }
        }
      } while (!exitExtend);
      KODI_LOG(LOG_DEBUG, "Guide Complete for Channel: %s", pTuner->Guide[nIndex]["GuideNumber"].asString().c_str());
      EPGBase::addguideinfo(jsonGuide);
    }
  } 
  return true;
}

unsigned long long EPG_SD::_getEndTime(Json::Value jsonGuide)
{
  Json::Value& jsonGuideItem = jsonGuide[jsonGuide.size() - 1];
  if (jsonGuideItem["EndTime"].asUInt() > 0)
  {
    return jsonGuideItem["EndTime"].asUInt();
  }
  else
  {
    return 0;
  }
}


bool EPG_SD::_insert_guide_data(Json::Value &Guide, Json::Value strInsertdata)
{
  Json::Value::ArrayIndex i = 0;

  for (Json::Value::ArrayIndex j = 0; j < strInsertdata[i]["Guide"].size(); j++)
  {
    Guide.append(strInsertdata[i]["Guide"][j]);
  }

  return true;
}

bool EPG_SD::_UpdateBasicGuide(HDHomeRunTuners::Tuner *pTuner, String strUrl)
{
  Json::Value::ArrayIndex nIndex;
  String strJson;
  Json::Reader jsonReader;

  KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun Basic guide: %s", strUrl.c_str());

  if (GetFileContents(strUrl.c_str(), strJson))
  {
    if (jsonReader.parse(strJson, pTuner->Guide) &&
      pTuner->Guide.type() == Json::arrayValue)
    {
      for (nIndex = 0; nIndex < pTuner->Guide.size(); nIndex++)
      {
        Json::Value& jsonGuide = pTuner->Guide[nIndex]["Guide"];

        if (jsonGuide.type() != Json::arrayValue)
          continue;

        EPGBase::addguideinfo(jsonGuide);
      }
    }
    else
    {
      KODI_LOG(LOG_ERROR, "Failed to parse guide", strUrl.c_str());
      return false;
    }
  }
  else
  {
    return false;
  }
  return true;
}

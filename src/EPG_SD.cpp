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

REGISTER_CLASS("SD", CEpg_SD);

bool CEpg_SD::UpdateGuide(HDHomeRunTuners::Tuner *pTuner, String advancedguide)
{
  String strUrl;

  strUrl = StringUtils::Format(CEpgBase::SD_GUIDEURL.c_str(), EncodeURL(pTuner->Device.device_auth).c_str());

  if (advancedguide == "AG")
  {
  // No guide Exists, so pull basic guide data to populate epg guide initially
    if (pTuner->Guide.size() < 1)
    {
      CEpg_SD::UpdateBasicGuide(pTuner, strUrl);
      return true;
    }
    else
    {
      if (CEpg_SD::UpdateAdvancedGuide(pTuner, strUrl))
      {
        return true;
      }
    }
  }
  else
  {
    if (CEpg_SD::UpdateBasicGuide(pTuner, strUrl))
    {
      return true;
    }
  }
  return false;
}

bool CEpg_SD::UpdateAdvancedGuide(HDHomeRunTuners::Tuner *pTuner, String strUrl)
{
  Json::Value::ArrayIndex nIndex;
  String strJson, strUrlExtended, strJsonExtended, jsonReaderError;
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());
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
      endTime = CEpg_SD::GetEndTime(jsonGuide);

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
          if (reader->parse(strJsonExtended.c_str(), strJsonExtended.c_str() + strJsonExtended.size(), &tempGuide, &jsonReaderError) &&
            tempGuide.type() == Json::arrayValue)
          {
            CEpg_SD::InsertGuideData(pTuner->Guide[nIndex]["Guide"], tempGuide);
            endTime = CEpg_SD::GetEndTime(pTuner->Guide[nIndex]["Guide"]);
          }
        }
      } while (!exitExtend);
      KODI_LOG(LOG_DEBUG, "Guide Complete for Channel: %s", pTuner->Guide[nIndex]["GuideNumber"].asString().c_str());
      CEpgBase::AddGuideInfo(jsonGuide);
    }
  } 
  return true;
}

unsigned long long CEpg_SD::GetEndTime(Json::Value jsonGuide)
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


bool CEpg_SD::InsertGuideData(Json::Value &Guide, Json::Value strInsertdata)
{
  Json::Value::ArrayIndex i = 0;

  for (Json::Value::ArrayIndex j = 0; j < strInsertdata[i]["Guide"].size(); j++)
  {
    Guide.append(strInsertdata[i]["Guide"][j]);
  }

  return true;
}

bool CEpg_SD::UpdateBasicGuide(HDHomeRunTuners::Tuner *pTuner, String strUrl)
{
  Json::Value::ArrayIndex nIndex;
  String strJson, jsonReaderError;
  Json::CharReaderBuilder jsonReaderBuilder;
  std::unique_ptr<Json::CharReader> const reader(jsonReaderBuilder.newCharReader());

  KODI_LOG(LOG_DEBUG, "Requesting HDHomeRun Basic guide: %s", strUrl.c_str());

  if (GetFileContents(strUrl.c_str(), strJson))
  {
    if (reader->parse(strJson.c_str(), strJson.c_str() + strJson.size(), &pTuner->Guide, &jsonReaderError) &&
      pTuner->Guide.type() == Json::arrayValue)
    {
      for (nIndex = 0; nIndex < pTuner->Guide.size(); nIndex++)
      {
        Json::Value& jsonGuide = pTuner->Guide[nIndex]["Guide"];

        if (jsonGuide.type() != Json::arrayValue)
          continue;

        CEpgBase::AddGuideInfo(jsonGuide);
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

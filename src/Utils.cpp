/*
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

#include "client.h"
#include "Utils.h"

#if defined(USE_DBG_CONSOLE) && defined(TARGET_WINDOWS)
int DbgPrintf(const char* szFormat, ...)
{
  static bool g_bDebugConsole = false;
  char szBuffer[4096];
  int nLen;
  va_list args;
  DWORD dwWritten;

  if (!g_bDebugConsole)
  {
    ::AllocConsole();
    g_bDebugConsole = true;
  }

  va_start(args, szFormat);
  nLen = vsnprintf(szBuffer, sizeof(szBuffer) - 1, szFormat, args);
  ::WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), szBuffer, nLen, &dwWritten, 0);
  va_end(args);

  return nLen;
}
#endif

bool GetFileContents(const String& url, String& strContent)
{
  char buffer[1024];
  void* fileHandle;

  strContent.clear();

  fileHandle = g.XBMC->OpenFile(url.c_str(), 0);

  if (fileHandle == NULL)
  {
    KODI_LOG(0, "GetFileContents: %s failed\n", url.c_str());
    return false;
  }

  for (;;)
  {
    ssize_t bytesRead = g.XBMC->ReadFile(fileHandle, buffer, sizeof(buffer));
    if (bytesRead <= 0)
      break;
    strContent.append(buffer, bytesRead);
  }

  g.XBMC->CloseFile(fileHandle);

  return true;
}

String EncodeURL(const String& strUrl)
{
  String str, strEsc;

  for (String::const_iterator iter = strUrl.begin(); iter != strUrl.end(); iter++)
  {
    char c = *iter;

    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
      str += c;
    else
    {
      String strPercent = StringUtils::Format("%%%02X", (int)c);
      str += strPercent;
    }
  }

  return str;
}

// return value set to 0 forces a curl fail
// still provides the info we are after (status protocol response
size_t header_callback(char* b, size_t size, size_t nitems, void* userdata)
{
  size_t numbytes = size * nitems;
  g.strCurlHeader.append(b, numbytes);
  return 0;
}

// use our own curl inclusion rather than depend on XBMC
// xbmc curl does not allow us to pull header protocol response as a 503 fails the CURLOpen
// and therefore we cannot use GetPropertyValue
bool CheckTunerAvailable(const String& url)
{
  g.strCurlHeader.clear();

  CURLcode urlReq;
  long responseCode;
  CURL* curlHandle = curl_easy_init();
  curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, header_callback);
  urlReq = curl_easy_perform(curlHandle);
  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &responseCode);
  if (urlReq != CURLE_OK)
  {
    curl_easy_cleanup(curlHandle);
    switch((int)responseCode)
    {
      case 200 :
        // Tuner available
        // Call hdhomerun_device_selector_choose_and_lock() 
        return true;
      case 503 :
        return false;
      default :
        KODI_LOG(0, "Unsupported Response Code: %d", (int)responseCode);
        return false;
    }
  }
  // should never reach here due to header_callback return being set to 0
  curl_easy_cleanup(curlHandle);
  return true;
}
/*
// Working but not ideal as we dont receive protocol status, we only get a failure for
// any status >= 400 from xbmc's curl implementation
bool CheckTunerAvailable2(const String& url)
{
  void* fileHandle;

  fileHandle = g.XBMC->CURLCreate(url.c_str());

  if (fileHandle == NULL)
  {
    return false;
  }

  // 503 error spits out here
  // GetFilePropertyValue does not work as the CURLOpen has failed
  if (!g.XBMC->CURLOpen(fileHandle, XFILE::READ_NO_CACHE))
  {
    return false;
  }

  g.XBMC->CloseFile(fileHandle);

  return true;
}
*/


project(pvr.hdhomerun)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(ZLIB REQUIRED)
find_package(hdhomerun REQUIRED)
if (WIN32)
  find_package(CURL)
else()
  find_package(CURL REQUIRED)
endif(WIN32)

IF(NOT CURL_FOUND)
  if (WIN32)
    message(STATUS "WIN - Curl Not Found - Download and compile CURL")
    include(DownloadProject.cmake)
    set(CURL_FLAGS "-DCURL_STATICLIB=ON -DCURL_STATIC_CRT=ON -DENABLE_DEBUG=OFF -DCMAKE_USE_OPENSSL=OFF -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF -DCURL_ZLIB=OFF -DHTTP_ONLY=ON-DENABLE_MANUAL=OFF")
    download_project(PROJ curl
                     URL https://github.com/curl/curl/archive/697271fc980331ffb53f12850f82c80ed182a375.tar.gz
                     CMAKE_ARGS ${CURL_FLAGS}
    )
    set(CURL_LIBRARY "-L C:/Program Files (x86)/CURL/bin")
    find_package(CURL REQUIRED)
  endif(WIN32)
endif(NOT CURL_FOUND)

if (NOT WIN32)
  include_directories(${kodiplatform_INCLUDE_DIRS}
                      ${p8-platform_INCLUDE_DIRS}
                      ${KODI_INCLUDE_DIR}
                      ${JSONCPP_INCLUDE_DIRS}
                      ${ZLIB_INCLUDE_DIRS}
                      ${HDHOMERUN_INCLUDE_DIRS}
                      ${CURL_INCLUDE_DIRS}
                      ${PROJECT_SOURCE_DIR}/lib
  )
else ()
  set(WIN32_CURL_INCLUDE "C:/Program Files (x86)/CURL/include")
  include_directories(${kodiplatform_INCLUDE_DIRS}
                      ${p8-platform_INCLUDE_DIRS}
                      ${KODI_INCLUDE_DIR}
                      ${JSONCPP_INCLUDE_DIRS}
                      ${ZLIB_INCLUDE_DIRS}
                      ${HDHOMERUN_INCLUDE_DIRS}
                      ${WIN32_CURL_INCLUDE}
                      ${PROJECT_SOURCE_DIR}/lib
  )
endif(NOT WIN32)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${ZLIB_LIBRARIES}
            ${CURL_LIBRARIES}
            ${HDHOMERUN_LIBRARIES}
)

set(PVRHDHOMERUN_SOURCES src/client.cpp
                         src/HDHomeRunTuners.cpp
                         src/Utils.cpp
                         src/EPG.cpp
                         src/EPG_SD.cpp
                         src/EPG_XML.cpp
)

set(PVRHDHOMERUN_HEADERS src/client.h
                         src/HDHomeRunTuners.h
                         src/Utils.h
                         src/EPG.h
                         src/EPG_SD.h
                         src/EPG_XML.h
)

if(WIN32)
#  IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
#    set(WIN32_CURL_BIN "C:/Program Files (x86)/CURL/bin/libcurl-d.dll")
#  ELSE()
#    set(WIN32_CURL_BIN "C:/Program Files (x86)/CURL/bin/libcurl.dll")
#  ENDIF()
  list(APPEND DEPLIBS ws2_32)
  list(APPEND DEPLIBS iphlpapi)
#  list(APPEND DEPLIBS ${WIN32_CURL_BIN})
  SET( PATCH_BATCH_FILE patch.bat )
  SET( PATCH_EXECUTABLE patch.exe )
  SET( RAPIDXML_DIR ${PROJECT_SOURCE_DIR}/lib/rapidxml )
  FILE( WRITE ${RAPIDXML_DIR}/${PATCH_BATCH_FILE} "${PATCH_EXECUTABLE} -p0 -i ${RAPIDXML_DIR}/0001-Win-compile.patch" )
  EXECUTE_PROCESS( COMMAND cmake -E chdir ${RAPIDXML_DIR} ${PATCH_BATCH_FILE} RESULT_VARIABLE RESULT_PATCH )
  MESSAGE( STATUS "Patch result: ${RESULT_PATCH}, PROF SOURCE DIR: ${PROJECT_SOURCE_DIR}" )
  FILE( REMOVE ${PATCH_BATCH_FILE} )
endif()

build_addon(pvr.hdhomerun PVRHDHOMERUN DEPLIBS)

include(CPack)

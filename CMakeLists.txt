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

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${JSONCPP_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                    ${HDHOMERUN_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR}/lib)

set(DEPLIBS ${kodiplatform_LIBRARIES}
            ${p8-platform_LIBRARIES}
            ${JSONCPP_LIBRARIES}
            ${ZLIB_LIBRARIES}
            ${HDHOMERUN_LIBRARIES})

set(PVRHDHOMERUN_SOURCES src/client.cpp
                         src/HDHomeRunTuners.cpp
                         src/Utils.cpp
                         src/EPG.cpp
                         src/EPG_SD.cpp
                         src/EPG_XML.cpp)

set(PVRHDHOMERUN_HEADERS src/client.h
                         src/HDHomeRunTuners.h
                         src/Utils.h
                         src/EPG.h
                         src/EPG_SD.h
                         src/EPG_XML.h)

if(WIN32)
  list(APPEND DEPLIBS ws2_32)
  list(APPEND DEPLIBS iphlpapi)
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

# - Try to find KdeWebKit
# Once done this will define
#
#  KDEWEBKIT_FOUND - system has KdeWebKit
#  KDEWEBKIT_INCLUDE_DIR - the KdeWebKit include directory
#  KDEWEBKIT_LIBRARIES - Link these to use KdeWebKit
#  KDEWEBKIT_DEFINITIONS - Compiler switches required for using KdeWebKit
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( KDEWEBKIT_INCLUDE_DIR AND KDEWEBKIT_LIBRARIES )
   # in cache already
   SET(KdeWebKit_FIND_QUIETLY TRUE)
endif ( KDEWEBKIT_INCLUDE_DIR AND KDEWEBKIT_LIBRARIES )

# Little trick I found in FindKDE4Interal... If we're building KdeWebKit, set the variables to point to the build directory.
if(kdewebkit_SOURCE_DIR)
    set(KDEWEBKIT_LIBRARIES kdewebkit)
    set(KDEWEBKIT_INCLUDE_DIR ${CMAKE_SOURCE_DIR})
endif(kdewebkit_SOURCE_DIR)

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  INCLUDE(UsePkgConfig)

  PKGCONFIG(kdewebkit _KdeWebKitIncDir _KdeWebKitLinkDir _KdeWebKitLinkFlags _KdeWebKitCflags)

  SET(KDEWEBKIT_DEFINITIONS ${_KdeWebKitCflags})
endif( NOT WIN32 )

FIND_PATH(KDEWEBKIT_INCLUDE_DIR NAMES webkitpart.h
  PATHS
  ${_KdeWebKitIncDir}
  ${KDE4_INCLUDE_INSTALL_DIR}
  PATH_SUFFIXES kdewebkit
)

FIND_LIBRARY(KDEWEBKIT_LIBRARIES NAMES kdewebkit
  PATHS
  ${_KdeWebKitLinkDir}
  ${KDE4_LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(KdeWebKit DEFAULT_MSG KDEWEBKIT_INCLUDE_DIR KDEWEBKIT_LIBRARIES )

# show the KDEWEBKIT_INCLUDE_DIR and KDEWEBKIT_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KDEWEBKIT_INCLUDE_DIR KDEWEBKIT_LIBRARIES)


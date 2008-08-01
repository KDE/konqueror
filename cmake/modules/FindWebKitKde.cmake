# - Try to find WebKitKde
# Once done this will define
#
#  WEBKITKDE_FOUND - system has WebKitKde
#  WEBKITKDE_INCLUDE_DIR - the WebKitKde include directory
#  WEBKITKDE_LIBRARIES - Link these to use WebKitKde
#  WEBKITKDE_DEFINITIONS - Compiler switches required for using WebKitKde
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( WEBKITKDE_INCLUDE_DIR AND WEBKITKDE_LIBRARIES )
   # in cache already
   SET(WebKitKde_FIND_QUIETLY TRUE)
endif ( WEBKITKDE_INCLUDE_DIR AND WEBKITKDE_LIBRARIES )

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  INCLUDE(UsePkgConfig)

  PKGCONFIG(webkitkde _WebKitKdeIncDir _WebKitKdeLinkDir _WebKitKdeLinkFlags _WebKitKdeCflags)

  SET(WEBKITKDE_DEFINITIONS ${_WebKitKdeCflags})
endif( NOT WIN32 )

FIND_PATH(WEBKITKDE_INCLUDE_DIR NAMES webkitpart.h
  PATHS
  ${_WebKitKdeIncDir}
  ${KDE4_INCLUDE_INSTALL_DIR}
  PATH_SUFFIXES webkitkde
)

FIND_LIBRARY(WEBKITKDE_LIBRARIES NAMES webkitkde
  PATHS
  ${_WebKitKdeLinkDir}
  ${KDE4_LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WebKitKde DEFAULT_MSG WEBKITKDE_INCLUDE_DIR WEBKITKDE_LIBRARIES )

# show the WEBKITKDE_INCLUDE_DIR and WEBKITKDE_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(WEBKITKDE_INCLUDE_DIR WEBKITKDE_LIBRARIES )


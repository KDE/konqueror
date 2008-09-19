# - Try to find KdeNetwork
# Once done this will define
#
#  KDENETWORK_FOUND - system has KdeNetwork
#  KDENETWORK_INCLUDE_DIR - the KdeNetwork include directory
#  KDENETWORK_LIBRARIES - Link these to use KdeNetwork
#  KDENETWORK_DEFINITIONS - Compiler switches required for using KdeNetwork
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( KDENETWORK_INCLUDE_DIR AND KDENETWORK_LIBRARIES )
   # in cache already
   SET(KdeNetwork_FIND_QUIETLY TRUE)
endif ( KDENETWORK_INCLUDE_DIR AND KDENETWORK_LIBRARIES )

# Little trick I found in FindKDE4Interal... If we're building KdeNetwork, set the variables to point to the build directory.
if(kdenetwork_SOURCE_DIR)
    set(KDENETWORK_LIBRARIES kdenetwork)
    get_directory_property(KDENETWORK_INCLUDE_DIR DIRECTORY "${kdenetwork_SOURCE_DIR}" PARENT_DIRECTORY)
endif(kdenetwork_SOURCE_DIR)

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  INCLUDE(UsePkgConfig)

  PKGCONFIG(kdenetwork _KdeNetworkIncDir _KdeNetworkLinkDir _KdeNetworkLinkFlags _KdeNetworkCflags)

  SET(KDENETWORK_DEFINITIONS ${_KdeNetworkCflags})
endif( NOT WIN32 )

FIND_PATH(KDENETWORK_INCLUDE_DIR NAMES knetworkaccessmanager.h
  PATHS
  ${_KdeNetworkIncDir}
  ${KDE4_INCLUDE_INSTALL_DIR}
  PATH_SUFFIXES kdenetwork
)

FIND_LIBRARY(KDENETWORK_LIBRARIES NAMES kdenetwork
  PATHS
  ${_KdeNetworkLinkDir}
  ${KDE4_LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(KdeNetwork DEFAULT_MSG KDENETWORK_INCLUDE_DIR KDENETWORK_LIBRARIES )

# show the KDENETWORK_INCLUDE_DIR and KDENETWORK_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KDENETWORK_INCLUDE_DIR KDENETWORK_LIBRARIES)


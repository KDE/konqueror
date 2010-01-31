# - Try to find KWebKitPart
# Once done this will define
#
#  KWEBKITPART_FOUND - system has KWebKitPart
#  KWEBKITPART_INCLUDE_DIR - the KWebKitPart include directory
#  KWEBKITPART_LIBRARIES - Link these to use KWebKitPart
#  KWEBKITPART_DEFINITIONS - Compiler switches required for using KWebKitPart
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if ( KWEBKITPART_INCLUDE_DIR AND KWEBKITPART_LIBRARIES )
   # in cache already
   SET(KWebKitPart_FIND_QUIETLY TRUE)
endif ( KWEBKITPART_INCLUDE_DIR AND KWEBKITPART_LIBRARIES )

# Little trick I found in FindKDE4Interal... If we're building KWebKitPart, set the variables to point to the build directory.
if(kwebkitpart_SOURCE_DIR)
    set(KWEBKITPART_LIBRARIES kwebkitpart)
    set(KWEBKITPART_INCLUDE_DIR ${CMAKE_SOURCE_DIR})
endif(kwebkitpart_SOURCE_DIR)

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  find_package(PkgConfig)
  pkg_check_modules(PC_KWEBKITPART kwebkitpart)
  set(PCRE_DEFINITIONS ${PC_KWEBKITPART_CFLAGS_OTHER})
endif( NOT WIN32 )

FIND_PATH(KWEBKITPART_INCLUDE_DIR NAMES kwebkitpart.h
  PATHS
  ${PC_KWEBKITPART_INCLUDEDIR} 
  ${PC_KWEBKITPART_INCLUDE_DIRS}
  ${KDE4_INCLUDE_INSTALL_DIR}
)

FIND_LIBRARY(KWEBKITPART_LIBRARIES NAMES kwebkit
  PATHS
  ${PC_KWEBKITPART_LIBDIR} 
  ${PC_KWEBKITPART_LIBRARY_DIRS}
  ${KDE4_LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(KWebKitPart DEFAULT_MSG KWEBKITPART_INCLUDE_DIR KWEBKITPART_LIBRARIES )

# show the KWEBKITPART_INCLUDE_DIR and KWEBKITPART_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KWEBKITPART_INCLUDE_DIR KWEBKITPART_LIBRARIES)

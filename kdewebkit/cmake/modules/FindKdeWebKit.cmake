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
    get_directory_property(KDEWEBKIT_INCLUDE_DIR DIRECTORY "${kdewebkit_SOURCE_DIR}" PARENT_DIRECTORY)
endif(kdewebkit_SOURCE_DIR)

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  find_package(PkgConfig)

  pkg_check_modules(PC_KDEWEBKIT kdewebkit)

  set(KDEWEBKIT_DEFINITIONS ${PC_KDEWEBKIT_CFLAGS_OTHER})

endif( NOT WIN32 )

FIND_PATH(KDEWEBKIT_INCLUDE_DIR NAMES kwebview.h
  PATHS
  ${PC_KDEWEBKIT_INCLUDEDIR}
  ${PC_KDEWEBKIT_INCLUDE_DIRS}
  ${KDE4_INCLUDE_INSTALL_DIR}
  PATH_SUFFIXES kdewebkit
)

FIND_LIBRARY(KDEWEBKIT_LIBRARIES NAMES kdewebkit
  PATHS
  ${PC_KDEWEBKIT_LIBDIR}
  ${PC_KDEWEBKIT_LIBRARY_DIRS}
  ${KDE4_LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(KdeWebKit DEFAULT_MSG KDEWEBKIT_INCLUDE_DIR KDEWEBKIT_LIBRARIES )

# show the KDEWEBKIT_INCLUDE_DIR and KDEWEBKIT_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KDEWEBKIT_INCLUDE_DIR KDEWEBKIT_LIBRARIES)


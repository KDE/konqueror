# Try to find the HTML Tidy lib
# Once done this will define:
#
#  LIBTIDY_FOUND - system has LIBTIDY
#  LIBTIDY_INCLUDE_DIR - the LIBTIDY include directory
#  LIBTIDY_LIBRARIES - The libraries needed to use LIBTIDY
#  LIBTIDY_ULONG_VERSION_FOUND - To deal with source incompatible versions
#
# Copyright (c) 2007, Paulo Moura Guedes, <moura@kdewebdev.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (LIBTIDY_INCLUDE_DIR)
  # Already in cache, be silent
  set(LibTidy_FIND_QUIETLY TRUE)
endif (LIBTIDY_INCLUDE_DIR)

FIND_PATH(LIBTIDY_INCLUDE_DIR tidy.h)

if( NOT LIBTIDY_INCLUDE_DIR )
    find_path(LIBTIDY_INCLUDE_DIR tidy.h PATH_SUFFIXES tidy)
    #now tidy.h was inside a tidy subdirectory so we need to
    #add that to the include dir
    set(LIBTIDY_INCLUDE_DIR ${LIBTIDY_INCLUDE_DIR}/tidy CACHE PATH "Libtidy include directory")
endif( NOT LIBTIDY_INCLUDE_DIR )



FIND_LIBRARY(LIBTIDY_LIBRARIES NAMES tidy)

if (LIBTIDY_INCLUDE_DIR AND LIBTIDY_LIBRARIES)
   set(LIBTIDY_FOUND TRUE)
endif (LIBTIDY_INCLUDE_DIR AND LIBTIDY_LIBRARIES)


if (LIBTIDY_FOUND)
   if (NOT LibTidy_FIND_QUIETLY)
      message(STATUS "Found Tidy: ${LIBTIDY_LIBRARIES}")
   endif (NOT LibTidy_FIND_QUIETLY)

    SET(CHECK_TIDY_ULONG_SOURCE_CODE "
#include <${LIBTIDY_INCLUDE_DIR}/tidy.h>

int main()
{
    ulong l;
    TidyInputSource s;
    s.sourceData = l;
}
")

    CHECK_CXX_SOURCE_COMPILES("${CHECK_TIDY_ULONG_SOURCE_CODE}" TIDY_ULONG_VERSION)
    if(TIDY_ULONG_VERSION)
        SET(LIBTIDY_ULONG_VERSION_FOUND TRUE)
    else(TIDY_ULONG_VERSION)
        SET(LIBTIDY_ULONG_VERSION_FOUND FALSE)
    endif(TIDY_ULONG_VERSION)

    macro_bool_to_01(TIDY_ULONG_VERSION HAVE_TIDY_ULONG_VERSION)

else (LIBTIDY_FOUND)
    if (LibTidy_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find LIBTIDY")
    else (LibTidy_FIND_REQUIRED)
      message(STATUS "Could NOT find LIBTIDY")
    endif (LibTidy_FIND_REQUIRED)
endif (LIBTIDY_FOUND)

MARK_AS_ADVANCED(LIBTIDY_INCLUDE_DIR LIBTIDY_LIBRARIES)


include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckStructMember)
include(CheckTypeSize)
include(CheckCXXSourceCompiles)

check_function_exists(getpeereid  HAVE_GETPEEREID) # kdesu
check_function_exists(statvfs HAVE_STATVFS) # kinfocenter

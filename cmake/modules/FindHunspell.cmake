# This file is part of the KDE project.
#
# SPDX-FileCopyrightText: 2021 Stefano Crocco <posta@stefanocrocco.it>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#[=======================================================================[.rst:
FindHunspell
-------

Finds the Hunspell program and its availlable dictionaries

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Hunspell_FOUND``
  True if the system has the hunspell executable.
``Hunspell_EXECUTABLE``
  The path to the hunspell executable
``Hunspell_DICTIONARIES``
  A list of all dictionaries availlable to hunspell, as reported by hunspell -D
``Hunspell_UNIQUE_DICTIONARIES``
  A list of dictionaries availlable to hunspell without dictionaries with the same name. If more than one dictionary with the same name exists, the first one reported by hunspell -D will be included
#]=======================================================================]

find_program(Hunspell_EXECUTABLE hunspell)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hunspell
  FOUND_VAR Hunspell_FOUND
  REQUIRED_VARS Hunspell_EXECUTABLE
)

if (Hunspell_FOUND)
  #Call hunspell -D to find list of available dictionaries
  execute_process(COMMAND ${Hunspell_EXECUTABLE} "-D" OUTPUT_VARIABLE Hunspell_OUTPUT ERROR_VARIABLE Hunspell_OUTPUT)
  #Convert output to a list of lines
  string(REPLACE "\n" ";" OUTPUT_AS_LIST ${Hunspell_OUTPUT})
  #NOTE: don't rely on the exact text of the first lines, as it can be translated
  #Remove the first 3 lines, as they aren't part of the dictionary list
  list(SUBLIST OUTPUT_AS_LIST 3 -1 OUTPUT_AS_LIST)

  set(Hunspell_DICTIONARIES "")
  set(Hunspell_UNIQUE_DICTIONARIES "")
  set(Hunspell_unique_dict_names "")

  #There may be more than one dictionary with the same name. We need a list
  #of unique dictionaries (as QtWebEngine only takes into account their name
  #and not their path)
  foreach(D ${OUTPUT_AS_LIST})
  #The content of Hunspell_DICTIONARIES isn't just the list of dictionaries:
  #the last entries are the line LOADED DICTIONARIES followed by a list of loaded
  #dictionary files which we aren't interested in. As soon as we find a line
  #not containing the path separator (/) we know that we have reached the LOADED DICTIONARIES
  #line and we can exit the loop
    string(FIND ${D} "/" separator_idx)
    #This means that / wasn't in the string
    if (separator_idx EQUAL -1)
      break()
    endif()
    list(APPEND Hunspell_DICTIONARIES ${D})
    get_filename_component(base ${D} NAME)
    list(FIND Hunspell_unique_dict_names ${base} dict_found)
    if (${dict_found} EQUAL -1)
      list(APPEND Hunspell_unique_dict_names ${base})
      list(APPEND Hunspell_UNIQUE_DICTIONARIES ${D})
    endif()
  endforeach()
endif()
list(JOIN Hunspell_unique_dict_names ", " detected_dictionaries)
add_feature_info(Hunspell Hunspell_FOUND "with dictionaries ${detected_dictionaries}")

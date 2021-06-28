# This file is part of the KDE project.
#
# Copyright 2021  Stefano Crocco <posta@stefanocrocco.it>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  #Find which hunspell dictionaries are availlable
  execute_process(COMMAND ${Hunspell_EXECUTABLE} "-D" OUTPUT_VARIABLE Hunspell_OUTPUT ERROR_VARIABLE Hunspell_OUTPUT)
  #Remove useless output
  string(REGEX MATCH "AVAILABLE DICTIONARIES [^\n]*\n(.*)" Hunspell_DICTIONARIES ${Hunspell_OUTPUT})
  #Convert string into list - CMAKE_MATCH_1 comes from the REGEX MATCH,
  #although the value has **also** been stored as Hunspell_DICTIONARIES
  string(REGEX REPLACE "\n" ";" Hunspell_DICTIONARIES "${CMAKE_MATCH_1}")
  set(Hunspell_UNIQUE_DICTIONARIES "")
  set(Hunspell_unique_dict_names "")

  #
  foreach(D ${Hunspell_DICTIONARIES})
    get_filename_component(base ${D} NAME)
    list(FIND Hunspell_unique_dict_names ${base} dict_found)
    if (${dict_found} EQUAL -1)
      list(APPEND Hunspell_unique_dict_names ${base})
      list(APPEND Hunspell_UNIQUE_DICTIONARIES ${D})
    endif()
  endforeach()
endif()

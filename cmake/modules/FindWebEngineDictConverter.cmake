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
FindWebEngineDictConverter
-------

Finds the qwebengine_convert_dict tool

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``WebEngineDictConverter_FOUND``
  True if the system has the qwebengine_convert_dict executable.
``WebEngineDictConverter_EXECUTABLE``
  The path to the qwebengine_convert_dict executable
#]=======================================================================]
include(ECMQueryQmake)
query_qmake(QT_BINARIES_DIR QT_INSTALL_BINS)
find_program(WebEngineDictConverter_EXECUTABLE qwebengine_convert_dict HINTS ${QT_BINARIES_DIR})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WebEngineDictConverter
  FOUND_VAR WebEngineDictConverter_FOUND
  REQUIRED_VARS WebEngineDictConverter_EXECUTABLE
)

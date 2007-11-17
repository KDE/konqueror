/*  This file is part of the KDE project
    Copyright (C) 2007 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KONQPRIVATE_EXPORT_H
#define KONQPRIVATE_EXPORT_H

/* needed for KDE_EXPORT and KDE_IMPORT macros */
#include <kdemacros.h>

/* Classes from the Konqueror application, which are exported only for unit tests */
#ifdef COMPILING_TESTS
# ifndef KONQ_TESTS_EXPORT
#  if defined(MAKE_KDEINIT_KONQUEROR_LIB) || defined(MAKE_KDED_KONQY_PRELOADER_LIB)
    /* We are building this library */
#   define KONQ_TESTS_EXPORT KDE_EXPORT
#  else
    /* We are using this library */
#   define KONQ_TESTS_EXPORT KDE_IMPORT
#  endif
# endif
#else /* not compiling tests */
# define KONQ_TESTS_EXPORT
#endif

/* Classes from the libkonquerorprivate library */
#ifndef KONQUERORPRIVATE_EXPORT
# if defined(MAKE_KONQUERORPRIVATE_LIB)
   /* We are building this library */
#  define KONQUERORPRIVATE_EXPORT KDE_EXPORT
# else
   /* We are using this library */
#  define KONQUERORPRIVATE_EXPORT KDE_IMPORT
# endif
#endif

#ifndef KONQUERORPRIVATE_EXPORT_DEPRECATED
# define KONQUERORPRIVATE_EXPORT_DEPRECATED KDE_DEPRECATED KONQUERORPRIVATE_EXPORT
#endif

#endif

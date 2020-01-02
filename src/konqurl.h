/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2020 Stefano Crocco <stefano.crocco@alice.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
*/

#ifndef KONQURL_H
#define KONQURL_H

#include <QLatin1String>
#include <QString>
#include <QUrl>

namespace KonqUrl {
    
    enum class Type{
        NoPath,
        Blank,
        Konqueror,
        Launch,
        Specs,
        Intro,
        Tips,
        Plugins
    };
    
    QLatin1String scheme();

    QLatin1String string(Type type = Type::NoPath);
    
    QUrl url(Type type = Type::NoPath);
    
    bool hasKonqScheme(const QUrl &url);
    
    bool canBeKonqUrl(const QString &url);
    
    bool hasKnownPathRoot(const QString &url);
    
    bool isValidNotBlank(const QString &url);
    
    bool isValidNotBlank(const QUrl &url);
    
    bool isKonqBlank(const QString &url);
    
    bool isKonqBlank(const QUrl &url);
  
};

#endif // KONQURL_H

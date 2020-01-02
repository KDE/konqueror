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

#include "konqurl.h"

namespace KonqUrl {
  
    QLatin1String string(KonqUrl::Type type)
    {
      static QLatin1String s_konqUrls[] = {
        QLatin1String("konq:"),
        QLatin1String("konq:blank"),
        QLatin1String("konq:konqueror"),
        QLatin1String("konq:konqueror/launch"),
        QLatin1String("konq:konqueror/specs"),
        QLatin1String("konq:konqueror/intro"),
        QLatin1String("konq:konqueror/tips"),
        QLatin1String("konq:plugins"),
      };
      return s_konqUrls[static_cast<int>(type)];
    }
    
    QLatin1String scheme()
    {
        static QLatin1String s_konqScheme = QLatin1String("konq");
        return s_konqScheme;
    }

    
    QUrl url(KonqUrl::Type type) {
        return QUrl(string(type));
    }
    
    bool hasKonqScheme(const QUrl& url)
    {
      return url.scheme() == scheme();
    }


    bool canBeKonqUrl(const QString &url) {
        return url.startsWith(string(Type::NoPath));
    }
  
    bool hasKnownPathRoot(const QString &url) {
        return url == string(Type::Blank) || url == string(Type::Plugins) || url.startsWith(string(Type::Konqueror));
    }
    
    bool isValidNotBlank(const QString &url) {
        return url == string(Type::NoPath) || url == string(Type::Plugins) || url.startsWith(string(Type::Konqueror));
    }
    
    bool isValidNotBlank(const QUrl &url) {
        return isValidNotBlank(url.url());
    }
    
    bool isKonqBlank(const QString &url) {
        return url == string(Type::Blank);
    }
    
    bool isKonqBlank(const QUrl &url) {
        return url.url() == string(Type::Blank);
    }
  
}

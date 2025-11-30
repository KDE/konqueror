/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
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
        QLatin1String("konq:speeddial")
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
        return url == string(Type::Blank) || url == string(Type::Plugins) || url.startsWith(string(Type::Konqueror)) || url.startsWith(string(Type::SpeedDial));
    }
    
    bool isValidNotBlank(const QString &url) {
        return url == string(Type::NoPath) || url == string(Type::Plugins) || url.startsWith(string(Type::Konqueror)) || url == string(Type::SpeedDial);
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

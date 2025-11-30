/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
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
        Plugins,
        SpeedDial
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

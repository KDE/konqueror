/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef WEBKITPART_H
#define WEBKITPART_H

#include "kwebkit_export.h"

#include <KDE/KParts/ReadOnlyPart>

namespace KParts {
  class BrowserExtension;
}

class QDataStream;
class QNetworkReply;
class QWebView;
class QWebFrame;
class QWebHistoryItem;
class KWebKitPartPrivate;

/**
 *
 *
 *
 */
class KWEBKIT_EXPORT KWebKitPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    explicit KWebKitPart(QWidget *parentWidget = 0, QObject *parent = 0, const QStringList &/*args*/ = QStringList());
    ~KWebKitPart();

    virtual bool openUrl(const KUrl &);

    virtual bool closeUrl();

    /**
     * A reference to the widget used to display web pages.
     */
    virtual QWebView *view();

protected:
    virtual void guiActivateEvent(KParts::GUIActivateEvent *);
    virtual bool openFile();

    void initAction();
    bool handleError(const KUrl &, QWebFrame *frame);

private:
    friend class KWebKitPartPrivate;
    KWebKitPartPrivate * const d;    
};

#endif // WEBKITPART_H

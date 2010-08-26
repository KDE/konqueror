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
#ifndef KWEBKITPART_H
#define KWEBKITPART_H

#include "kwebkit_export.h"

#include <KDE/KParts/ReadOnlyPart>

namespace KParts {
  class BrowserExtension;
}

class QWebView;
class KWebKitPartPrivate;

/**
 * A KPart wrapper for the QtWebKit's browser rendering engine.
 *
 * This class attempts to provide the same type of integration into KPart
 * plugin applications, such as Konqueror, in much the same way as KHTML.
 *
 * Unlink the KHTML part however, access into the internals of the rendering
 * engine are provided through existing QtWebKit class ; @see QWebView.
 *
 */
class KWEBKIT_EXPORT KWebKitPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY( bool modified READ isModified )
public:
    explicit KWebKitPart(QObject *parent = 0, QWidget *parentWidget = 0,
                         const QString& historyFile = QString(),
                         const QStringList &/*args*/ = QStringList());
    ~KWebKitPart();

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openUrl
     */
    virtual bool openUrl(const KUrl &);

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::closeUrl
     */
    virtual bool closeUrl();

    /**
     * Returns a pointer to the render widget used to display a web page.
     *
     * @see QWebView.
     */
    virtual QWebView *view();

    /**
     * Checks whether the page contains unsubmitted form changes.
     *
     * @return @p true if form changes exist.
     */
    bool isModified() const;

protected:
    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::guiActivateEvent
     */
    virtual void guiActivateEvent(KParts::GUIActivateEvent *);

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openFile
     */
    virtual bool openFile();

private:
    friend class KWebKitPartPrivate;
    KWebKitPartPrivate * const d;
};

#endif // WEBKITPART_H

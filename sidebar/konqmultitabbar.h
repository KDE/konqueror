/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KONQMULTITABBAR_H
#define KONQMULTITABBAR_H

#include <QUrl>
#include <kmultitabbar.h>

class KonqMultiTabBar : public KMultiTabBar
{
    Q_OBJECT

public:
    explicit KonqMultiTabBar(QWidget *parent);

Q_SIGNALS:
    void urlsDropped(const QList<QUrl> &urls);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

#endif /* KONQMULTITABBAR_H */


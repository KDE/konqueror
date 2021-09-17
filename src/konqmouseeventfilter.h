/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.bg>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KONQMOUSEEVENTFILTER_H
#define KONQMOUSEEVENTFILTER_H

#include <QObject>

class KonqMouseEventFilter : public QObject
{
    Q_OBJECT

public:
    static KonqMouseEventFilter *self();

    void reparseConfiguration();

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

private:
    explicit KonqMouseEventFilter();
    friend class KonqMouseEventFilterSingleton;

    bool m_bBackRightClick;
};

#endif /* KONQMOUSEEVENTFILTER_H */


/**
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KCOOKIESPOLICYSELECTIONDLG_H
#define KCOOKIESPOLICYSELECTIONDLG_H

#include <kdialog.h>
#include "ui_kcookiespolicyselectiondlg.h"

class QWidget;

class KCookieAdvice
{
public:
    enum Value {Dunno = 0, Accept, AcceptForSession, Reject, Ask};

    static const char* adviceToStr (const int& advice) {
        switch (advice) {
        case KCookieAdvice::Accept:
            return I18N_NOOP ("Accept");
        case KCookieAdvice::AcceptForSession:
            return I18N_NOOP ("AcceptForSession");
        case KCookieAdvice::Reject:
            return I18N_NOOP ("Reject");
        case KCookieAdvice::Ask:
            return I18N_NOOP ("Ask");
        default:
            return I18N_NOOP ("Dunno");
        }
    }

    static KCookieAdvice::Value strToAdvice (const QString& _str) {
        if (_str.isEmpty())
            return KCookieAdvice::Dunno;

        QString advice = _str.toLower();

        if (advice == QLatin1String ("accept"))
            return KCookieAdvice::Accept;
        else if (advice == QLatin1String ("acceptforsession"))
            return KCookieAdvice::AcceptForSession;
        else if (advice == QLatin1String ("reject"))
            return KCookieAdvice::Reject;
        else if (advice == QLatin1String ("ask"))
            return KCookieAdvice::Ask;

        return KCookieAdvice::Dunno;
    }
};

class KCookiesPolicySelectionDlg : public KDialog
{
    Q_OBJECT

public:
    explicit KCookiesPolicySelectionDlg (QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~KCookiesPolicySelectionDlg () {}

    int advice() const;
    QString domain() const;

    void setEnableHostEdit (bool, const QString& host = QString());
    void setPolicy (int policy);

protected Q_SLOTS:
    void slotTextChanged (const QString&);

private:
    Ui::KCookiesPolicySelectionDlgUI mUi;
};
#endif

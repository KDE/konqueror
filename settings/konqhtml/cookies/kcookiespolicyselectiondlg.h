/*
    SPDX-FileCopyrightText: 2000 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCOOKIESPOLICYSELECTIONDLG_H
#define KCOOKIESPOLICYSELECTIONDLG_H

#include "ui_kcookiespolicyselectiondlg.h"
#include "interfaces/cookiejar.h"

#include <KLazyLocalizedString>

#include <QDialog>

class QWidget;
class QDialogButtonBox;

class KCookieAdvice
{
public:

    static const char *adviceToStr(KonqInterfaces::CookieJar::CookieAdvice advice)
    {
        switch (advice) {
        case KonqInterfaces::CookieJar::CookieAdvice::Accept:
            return kli18n("Accept").untranslatedText();
        case KonqInterfaces::CookieJar::CookieAdvice::AcceptForSession:
            return kli18n("Accept For Session").untranslatedText();
        case KonqInterfaces::CookieJar::CookieAdvice::Reject:
            return kli18n("Reject").untranslatedText();
        case KonqInterfaces::CookieJar::CookieAdvice::Ask:
            return kli18n("Ask").untranslatedText();
        default:
            return kli18n("Do Not Know").untranslatedText();
        }
    }

    static KonqInterfaces::CookieJar::CookieAdvice strToAdvice(const QString &_str)
    {
        if (_str.isEmpty()) {
            return KonqInterfaces::CookieJar::CookieAdvice::Unknown;
        }

        QString advice = _str.toLower().remove(QLatin1Char(' '));

        if (advice == QLatin1String("accept")) {
            return KonqInterfaces::CookieJar::CookieAdvice::Accept;
        } else if (advice == QLatin1String("acceptforsession")) {
            return KonqInterfaces::CookieJar::CookieAdvice::AcceptForSession;
        } else if (advice == QLatin1String("reject")) {
            return KonqInterfaces::CookieJar::CookieAdvice::Reject;
        } else if (advice == QLatin1String("ask")) {
            return KonqInterfaces::CookieJar::CookieAdvice::Ask;
        } else {
            return KonqInterfaces::CookieJar::CookieAdvice::Unknown;
        }
    }
};

class KCookiesPolicySelectionDlg : public QDialog
{
    Q_OBJECT

public:
    explicit KCookiesPolicySelectionDlg(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~KCookiesPolicySelectionDlg() override
    {
    }

    KonqInterfaces::CookieJar::CookieAdvice advice() const;
    QString domain() const;

    void setEnableHostEdit(bool, const QString &host = QString());
    void setPolicy(KonqInterfaces::CookieJar::CookieAdvice policy);

protected Q_SLOTS:
    void slotTextChanged(const QString &);
    void slotPolicyChanged(const QString &);

private:
    KonqInterfaces::CookieJar::CookieAdvice mOldPolicy = KonqInterfaces::CookieJar::CookieAdvice::Accept;
    Ui::KCookiesPolicySelectionDlgUI mUi;
    QDialogButtonBox *mButtonBox;
};
#endif

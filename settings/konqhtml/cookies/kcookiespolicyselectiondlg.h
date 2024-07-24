/*
    SPDX-FileCopyrightText: 2000 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCOOKIESPOLICYSELECTIONDLG_H
#define KCOOKIESPOLICYSELECTIONDLG_H

#include "ui_kcookiespolicyselectiondlg.h"

#include "konqsettings.h"

#include <KLazyLocalizedString>

#include <QDialog>

class QWidget;
class QDialogButtonBox;

class KCookieAdvice
{
public:

    static const char *adviceToStr(Konq::Settings::CookieAdvice advice)
    {
        switch (advice) {
        case Konq::Settings::CookieAdvice::Accept:
            return kli18n("Accept").untranslatedText();
        case Konq::Settings::CookieAdvice::AcceptForSession:
            return kli18n("Accept For Session").untranslatedText();
        case Konq::Settings::CookieAdvice::Reject:
            return kli18n("Reject").untranslatedText();
        case Konq::Settings::CookieAdvice::Ask:
            return kli18n("Ask").untranslatedText();
        default:
            return kli18n("Do Not Know").untranslatedText();
        }
    }

    static Konq::Settings::CookieAdvice strToAdvice(const QString &_str)
    {
        if (_str.isEmpty()) {
            return Konq::Settings::CookieAdvice::Unknown;
        }

        QString advice = _str.toLower().remove(QLatin1Char(' '));

        if (advice == QLatin1String("accept")) {
            return Konq::Settings::CookieAdvice::Accept;
        } else if (advice == QLatin1String("acceptforsession")) {
            return Konq::Settings::CookieAdvice::AcceptForSession;
        } else if (advice == QLatin1String("reject")) {
            return Konq::Settings::CookieAdvice::Reject;
        } else if (advice == QLatin1String("ask")) {
            return Konq::Settings::CookieAdvice::Ask;
        } else {
            return Konq::Settings::CookieAdvice::Unknown;
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

    Konq::Settings::CookieAdvice advice() const;
    QString domain() const;

    void setEnableHostEdit(bool, const QString &host = QString());
    void setPolicy(Konq::Settings::CookieAdvice policy);

protected Q_SLOTS:
    void slotTextChanged(const QString &);
    void slotPolicyChanged(const QString &);

private:
    Konq::Settings::CookieAdvice mOldPolicy = Konq::Settings::CookieAdvice::Accept;
    Ui::KCookiesPolicySelectionDlgUI mUi;
    QDialogButtonBox *mButtonBox;
};
#endif

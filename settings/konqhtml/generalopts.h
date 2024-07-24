/* General options (for both fm and web modes) konqueror options

    SPDX-FileCopyrightText: 1998 Sven Radej
    SPDX-FileCopyrightText: 1998 David Faure
    SPDX-FileCopyrightText: 2007 Nick Shaforostoff

*/

#ifndef GENERALOPTS_H
#define GENERALOPTS_H

#include <QCheckBox>

#include <KCModule>

class QComboBox;
class QLineEdit;
class QVBoxLayout;
class KMessageWidget;
class QCheckBox;

class KKonqGeneralOptions : public KCModule
{
    Q_OBJECT

public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    KKonqGeneralOptions(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KKonqGeneralOptions() override;
    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void slotChanged();
    void displayEmpytStartPageWarningIfNeeded();

private:
    void addHomeUrlWidgets(QVBoxLayout *);

    QComboBox *m_startCombo;
    QLineEdit *homeURL;
    QLineEdit *startURL;
    QComboBox *m_webEngineCombo;
    QComboBox *m_splitBehaviour;
    KMessageWidget *m_emptyStartUrlWarning;
    QCheckBox *m_restoreLastState;
};

#endif // GENERALOPTS_H

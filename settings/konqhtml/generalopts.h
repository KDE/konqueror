/* General options (for both fm and web modes) konqueror options
 *
 * Copyright (c) Sven Radej 1998
 * Copyright (c) David Faure 1998
 * Copyright (c) Nick Shaforostoff 2007
 *
 */

#ifndef GENERALOPTS_H
#define GENERALOPTS_H

#include <QCheckBox>

#include <kcmodule.h>
#include <ksharedconfig.h>

class QComboBox;
class QLineEdit;
class Ui_advancedTabOptions;
class QVBoxLayout;

class KKonqGeneralOptions : public KCModule
{
    Q_OBJECT

public:
    KKonqGeneralOptions(QWidget *parent, const QVariantList &);
    ~KKonqGeneralOptions() override;
    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void slotChanged();

private:
    void addHomeUrlWidgets(QVBoxLayout *);

    KSharedConfig::Ptr m_pConfig;

    QComboBox *m_startCombo;
    QLineEdit *homeURL;
    QLineEdit *startURL;
    QComboBox *m_webEngineCombo;
    Ui_advancedTabOptions *tabOptions;
};

#endif // GENERALOPTS_H

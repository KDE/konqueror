//
//
// General options (for both fm and web modes) konqueror options
//
// (c) Sven Radej 1998
// (c) David Faure 1998
// (c) Nick Shaforostoff 2007

#ifndef GENERALOPTS_H
#define GENERALOPTS_H

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>

#include <kcmodule.h>
#include <ksharedconfig.h>

class Ui_advancedTabOptions;

class KKonqGeneralOptions : public KCModule
{
    Q_OBJECT

public:
    KKonqGeneralOptions( QWidget *parent, const QVariantList& );
    ~KKonqGeneralOptions();
    virtual void load();
    virtual void save();
    virtual void defaults();

private Q_SLOTS:
    void slotChanged();

private:
    KSharedConfig::Ptr m_pConfig;

    Ui_advancedTabOptions* tabOptions;
};

#endif // GENERALOPTS_H

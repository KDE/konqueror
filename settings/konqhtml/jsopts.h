/*
    SPDX-FileCopyrightText: 1996 Martin R. Jones

    SPDX-FileCopyrightText: 1998 Torben Weis
    KControl port & modifications

    SPDX-FileCopyrightText: 2024 Stefano Crocco
    Use this as toplevel javascript KCM
*/


#ifndef JSOPTS_H
#define JSOPTS_H

#include <kcmodule.h>
#include <KSharedConfig>
#include "domainlistview.h"
#include "jspolicies.h"

class QCheckBox;

class PolicyDialog;

class KJavaScriptOptions : public KCModule
{
    Q_OBJECT
public:
    KJavaScriptOptions(QObject *parent, const KPluginMetaData &md={});

    void load() override;
    void save() override;
    void defaults() override;

    bool _removeJavaScriptDomainAdvice;

private Q_SLOTS:
    void slotChangeJSEnabled();

private:

    KSharedConfig::Ptr m_pConfig;
    const QString m_groupname = QStringLiteral("Java/JavaScript Settings");
    JSPolicies js_global_policies;
    QCheckBox *enableJavaScriptGloballyCB;
    JSPoliciesFrame *js_policies_frame;

    DomainListView *domainSpecific;

    friend class DomainListView;
};

#endif      // JSOPTS_H


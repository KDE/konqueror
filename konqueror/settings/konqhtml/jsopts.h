//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef JSOPTS_H
#define JSOPTS_H

#include <kcmodule.h>
#include <KComponentData>
#include <KSharedConfig>
#include "domainlistview.h"
#include "jspolicies.h"

class QCheckBox;

class PolicyDialog;

class KJavaScriptOptions;

/** JavaScript-specific enhancements to the domain list view
  */
class JSDomainListView : public DomainListView
{
    Q_OBJECT
public:
    JSDomainListView(KSharedConfig::Ptr config, const QString &group, KJavaScriptOptions *opt,
                     QWidget *parent);
    virtual ~JSDomainListView();

    /** remnant for importing pre KDE 3.2 settings
      */
    void updateDomainListLegacy(const QStringList &domainConfig);

protected:
    JSPolicies *createPolicies() Q_DECL_OVERRIDE;
    JSPolicies *copyPolicies(Policies *pol) Q_DECL_OVERRIDE;
    void setupPolicyDlg(PushButton trigger, PolicyDialog &pDlg,
                                Policies *copy) Q_DECL_OVERRIDE;

private:
    QString group;
    KJavaScriptOptions *options;
};

class KJavaScriptOptions : public KCModule
{
    Q_OBJECT
public:
    KJavaScriptOptions(KSharedConfig::Ptr config, const QString &group, QWidget *parent);

    void load() Q_DECL_OVERRIDE;
    void save() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

    bool _removeJavaScriptDomainAdvice;

private Q_SLOTS:
    void slotChangeJSEnabled();

private:

    KSharedConfig::Ptr m_pConfig;
    QString m_groupname;
    JSPolicies js_global_policies;
    QCheckBox *enableJavaScriptGloballyCB;
    QCheckBox *reportErrorsCB;
    QCheckBox *jsDebugWindow;
    JSPoliciesFrame *js_policies_frame;
    bool _removeECMADomainSettings;

    JSDomainListView *domainSpecific;

    friend class JSDomainListView;
};

#endif      // JSOPTS_H


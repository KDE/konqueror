/*
    SPDX-FileCopyrightText: 1996 Martin R. Jones
    HTML Options
    SPDX-FileCopyrightText: 1998 Torben Weis
    Port to KControl
    SPDX-FileCopyrightText: 2000 Daniel Molkentin
    Redesign and cleanup

*/

#ifndef JAVAOPTS_H
#define JAVAOPTS_H

#include <kcmodule.h>

#include "domainlistview.h"
#include "policies.h"
#include <KSharedConfig>

class KUrlRequester;

class QCheckBox;
class QLineEdit;
class KPluralHandlingSpinBox;

class KJavaOptions;

/** policies with java-specific constructor
  */
class JavaPolicies : public Policies
{
public:
    /**
     * constructor
     * @param config configuration to initialize this instance from
     * @param group config group to use if this instance contains the global
     *    policies (global == true)
     * @param global true if this instance contains the global policy settings,
     *    false if this instance contains policies specific for a domain.
     * @param domain name of the domain this instance is used to configure the
     *    policies for (case insensitive, ignored if global == true)
     */
    JavaPolicies(const KSharedConfig::Ptr &config, const QString &group, bool global,
                 const QString &domain = QString());

    /** empty constructor to make QMap happy
     * don't use for constructing a policies instance.
     * @internal
     */
    //JavaPolicies();

    ~JavaPolicies() override;
};

/** Java-specific enhancements to the domain list view
  */
class JavaDomainListView : public DomainListView
{
    Q_OBJECT
public:
    JavaDomainListView(KSharedConfig::Ptr config, const QString &group, KJavaOptions *opt,
                       QWidget *parent);
    ~JavaDomainListView() override;

    /** remnant for importing pre KDE 3.2 settings
      */
    void updateDomainListLegacy(const QStringList &domainConfig);

protected:
    JavaPolicies *createPolicies() override;
    JavaPolicies *copyPolicies(Policies *pol) override;
    void setupPolicyDlg(PushButton trigger, PolicyDialog &pDlg,
                                Policies *copy) override;

private:
    QString group;
    KJavaOptions *options;
};

class KJavaOptions : public KCModule
{
    Q_OBJECT

public:
    KJavaOptions(const KSharedConfig::Ptr &config, const QString &group, QWidget *parent);

    void load() override;
    void save() override;
    void defaults() override;

    bool _removeJavaScriptDomainAdvice;

#if QT_VERSION_MAJOR < 6
    void setNeedsSave(bool needs) {emit changed(needs);}
#endif

private Q_SLOTS:
    void slotChanged();
    void toggleJavaControls();

private:

    KSharedConfig::Ptr m_pConfig;
    QString  m_groupname;
    JavaPolicies java_global_policies;

    QCheckBox     *enableJavaGloballyCB;
    QCheckBox     *javaSecurityManagerCB;
    QCheckBox     *useKioCB;
    QCheckBox     *enableShutdownCB;
    KPluralHandlingSpinBox *serverTimeoutSB;
    QLineEdit     *addArgED;
    KUrlRequester *pathED;
    bool           _removeJavaDomainSettings;

    JavaDomainListView *domainSpecific;

    friend class JavaDomainListView;
};

#endif      // HTML_OPTIONS_H


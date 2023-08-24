/*
    SPDX-FileCopyrightText: 1996 Martin R. Jones
    SPDX-FileCopyrightText: 1998 Bernd Wuebben

    SPDX-FileCopyrightText: 1998 Torben Weis
    KControl port & modifications

    SPDX-FileCopyrightText: 1998 David Faure
    End of the KControl port, added 'kfmclient configure' call.

    SPDX-FileCopyrightText: 2000 Kalle Dalheimer
    New configuration scheme for Java/JavaScript

    SPDX-FileCopyrightText: 2000 Daniel Molkentin
    Redesign and cleanup

    SPDX-FileCopyrightText: 2002-2003 Leo Savernik
    Big changes to accommodate per-domain settings

*/

// Own
#include "javaopts.h"

// Qt
#include <QFormLayout>
#include <QGroupBox>
#include <QTreeWidget>

// KDE
#include <kurlrequester.h>
#include <klineedit.h>
#include <KLocalizedString>
#include <QHBoxLayout>
#include <kconfiggroup.h>
#include <kpluralhandlingspinbox.h>

// Local
#include "htmlopts.h"
#include "policydlg.h"
#include <htmlextension.h>
#include <htmlsettingsinterface.h>

// == class JavaPolicies =====

JavaPolicies::JavaPolicies(const KSharedConfig::Ptr &config, const QString &group, bool global,
                           const QString &domain) :
    Policies(config, group, global, domain, QStringLiteral("java."), QStringLiteral("EnableJava"))
{
}

/*
JavaPolicies::JavaPolicies() : Policies(0,QString(),false,
    QString(),QString(),QString()) {
}
*/

JavaPolicies::~JavaPolicies()
{
}

// == class KJavaOptions =====

KJavaOptions::KJavaOptions(const KSharedConfig::Ptr &config, const QString &group,
                           QWidget *parent)
    : KCModule(parent),
      _removeJavaScriptDomainAdvice(false),
      m_pConfig(config),
      m_groupname(group),
      java_global_policies(config, group, true),
      _removeJavaDomainSettings(false)
{
    QVBoxLayout *toplevel = new QVBoxLayout(widget());

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    enableJavaGloballyCB = new QCheckBox(i18n("Enable Ja&va globally"), widget());
    connect(enableJavaGloballyCB, &QAbstractButton::clicked, this, &KJavaOptions::slotChanged);
    connect(enableJavaGloballyCB, &QAbstractButton::clicked, this, &KJavaOptions::toggleJavaControls);
    toplevel->addWidget(enableJavaGloballyCB);

    /***************************************************************************
     ***************** Domain Specific Settings ********************************
     **************************************************************************/
    domainSpecific = new JavaDomainListView(m_pConfig, m_groupname, this, widget());
    connect(domainSpecific, &DomainListView::changed, this, &KJavaOptions::slotChanged);
    toplevel->addWidget(domainSpecific, 2);

    /***************************************************************************
     ***************** Java Runtime Settings ***********************************
     **************************************************************************/
    QGroupBox *javartGB = new QGroupBox(i18n("Java Runtime Settings"), widget());
    QFormLayout *laygroup1 = new QFormLayout(javartGB);
    toplevel->addWidget(javartGB);

    javaSecurityManagerCB = new QCheckBox(i18n("&Use security manager"), widget());
    laygroup1->addRow(javaSecurityManagerCB);
    connect(javaSecurityManagerCB, &QAbstractButton::toggled, this, &KJavaOptions::slotChanged);

    useKioCB = new QCheckBox(i18n("Use &KIO"), widget());
    laygroup1->addRow(useKioCB);
    connect(useKioCB, &QAbstractButton::toggled, this, &KJavaOptions::slotChanged);

    enableShutdownCB = new QCheckBox(i18n("Shu&tdown applet server when inactive for more than"), widget());
    connect(enableShutdownCB, &QAbstractButton::toggled, this, &KJavaOptions::slotChanged);
    connect(enableShutdownCB, &QAbstractButton::clicked, this, &KJavaOptions::toggleJavaControls);
    QWidget *secondsHB = new QWidget(javartGB);
    QHBoxLayout *secondsHBHBoxLayout = new QHBoxLayout(secondsHB);
    secondsHBHBoxLayout->setContentsMargins(0, 0, 0, 0);
    laygroup1->addWidget(secondsHB);
    serverTimeoutSB = new KPluralHandlingSpinBox(secondsHB);
    serverTimeoutSB->setSizePolicy(QSizePolicy::MinimumExpanding, serverTimeoutSB->sizePolicy().verticalPolicy());
    secondsHBHBoxLayout->addWidget(serverTimeoutSB);
    serverTimeoutSB->setSingleStep(5);
    serverTimeoutSB->setRange(0, 1000);
    serverTimeoutSB->setSuffix(ki18np(" second", " seconds"));
    connect(serverTimeoutSB, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](){ slotChanged(); });
    laygroup1->addRow(enableShutdownCB, serverTimeoutSB);

    pathED = new  KUrlRequester(widget());
    connect(pathED, &KUrlRequester::textChanged, this, &KJavaOptions::slotChanged);
    laygroup1->addRow(i18n("&Path to Java executable, or 'java':"), pathED);

    addArgED = new QLineEdit(widget());
    connect(addArgED, &QLineEdit::textChanged, this, &KJavaOptions::slotChanged);
    laygroup1->addRow(i18n("Additional Java a&rguments:"), addArgED);

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    enableJavaGloballyCB->setToolTip(i18n("Enables the execution of scripts written in Java "
                                            "that can be contained in HTML pages. "
                                            "Note that, as with any browser, enabling active contents can be a security problem."));
    QString wtstr = i18n("<p>This box contains the domains and hosts you have set "
                         "a specific Java policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling Java applets on pages sent by these "
                         "domains or hosts.</p><p>Select a policy and use the controls on "
                         "the right to modify it.</p>");
    domainSpecific->listView()->setToolTip(wtstr);
#if 0
    domainSpecific->importButton()->setToolTip(i18n("Click this button to choose the file that contains "
            "the Java policies. These policies will be merged "
            "with the existing ones. Duplicate entries are ignored."));
    domainSpecific->exportButton()->setToolTip(i18n("Click this button to save the Java policy to a zipped "
            "file. The file, named <b>java_policy.tgz</b>, will be "
            "saved to a location of your choice."));
#endif
    domainSpecific->setToolTip(i18n("Here you can set specific Java policies for any particular "
                                      "host or domain. To add a new policy, simply click the <i>New...</i> "
                                      "button and supply the necessary information requested by the "
                                      "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                      "button and choose the new policy from the policy dialog box. Clicking "
                                      "on the <i>Delete</i> button will remove the selected policy, causing the default "
                                      "policy setting to be used for that domain."));
#if 0
    "The <i>Import</i> and <i>Export</i> "
    "button allows you to easily share your policies with other people by allowing "
    "you to save and retrieve them from a zipped file."));
#endif

    javaSecurityManagerCB->setToolTip(i18n("Enabling the security manager will cause the jvm to run with a Security "
                                        "Manager in place. This will keep applets from being able to read and "
                                        "write to your file system, creating arbitrary sockets, and other actions "
                                        "which could be used to compromise your system. Disable this option at your "
                                        "own risk. You can modify your $HOME/.java.policy file with the Java "
                                        "policytool utility to give code downloaded from certain sites more "
                                        "permissions."));

    useKioCB->setToolTip(i18n("Enabling this will cause the jvm to use KIO for network transport "));

    pathED->setToolTip(i18n("Enter the path to the java executable. If you want to use the jre in "
                              "your path, simply leave it as 'java'. If you need to use a different jre, "
                              "enter the path to the java executable (e.g. /usr/lib/jdk/bin/java), "
                              "or the path to the directory that contains 'bin/java' (e.g. /opt/IBMJava2-13)."));

    addArgED->setToolTip(i18n("If you want special arguments to be passed to the virtual machine, enter them here."));

    QString shutdown = i18n("When all the applets have been destroyed, the applet server should shut down. "
                            "However, starting the jvm takes a lot of time. If you would like to "
                            "keep the java process running while you are "
                            "browsing, you can set the timeout value to whatever you like. To keep "
                            "the java process running for the whole time that the konqueror process is, "
                            "leave the Shutdown Applet Server checkbox unchecked.");
    serverTimeoutSB->setToolTip(shutdown);
    enableShutdownCB->setToolTip(shutdown);
}

void KJavaOptions::load()
{
    // *** load ***
    java_global_policies.load();
    bool bJavaGlobal      = java_global_policies.isFeatureEnabled();
    bool bSecurityManager = m_pConfig->group(m_groupname).readEntry("UseSecurityManager", true);
    bool bUseKio = m_pConfig->group(m_groupname).readEntry("UseKio", false);
    bool bServerShutdown  = m_pConfig->group(m_groupname).readEntry("ShutdownAppletServer", true);
    int  serverTimeout    = m_pConfig->group(m_groupname).readEntry("AppletServerTimeout", 60);
    QString sJavaPath     = m_pConfig->group(m_groupname).readPathEntry("JavaPath", QStringLiteral("java"));

    if (sJavaPath == QLatin1String("/usr/lib/jdk")) {
        sJavaPath = QStringLiteral("java");
    }

    if (m_pConfig->group(m_groupname).hasKey("JavaDomains")) {
        domainSpecific->initialize(m_pConfig->group(m_groupname).readEntry("JavaDomains", QStringList()));
    } else if (m_pConfig->group(m_groupname).hasKey("JavaDomainSettings")) {
        domainSpecific->updateDomainListLegacy(m_pConfig->group(m_groupname).readEntry("JavaDomainSettings", QStringList()));
        _removeJavaDomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy(m_pConfig->group(m_groupname).readEntry("JavaScriptDomainAdvice", QStringList()));
        _removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    enableJavaGloballyCB->setChecked(bJavaGlobal);
    javaSecurityManagerCB->setChecked(bSecurityManager);
    useKioCB->setChecked(bUseKio);

    addArgED->setText(m_pConfig->group(m_groupname).readEntry("JavaArgs"));
    pathED->lineEdit()->setText(sJavaPath);

    enableShutdownCB->setChecked(bServerShutdown);
    serverTimeoutSB->setValue(serverTimeout);

    toggleJavaControls();
    setNeedsSave(false);
}

void KJavaOptions::defaults()
{
    java_global_policies.defaults();
    enableJavaGloballyCB->setChecked(false);
    javaSecurityManagerCB->setChecked(true);
    useKioCB->setChecked(false);
    pathED->lineEdit()->setText(QStringLiteral("java"));
    addArgED->setText(QLatin1String(""));
    enableShutdownCB->setChecked(true);
    serverTimeoutSB->setValue(60);
    toggleJavaControls();
    setNeedsSave(true);
}

void KJavaOptions::save()
{
    java_global_policies.save();
    m_pConfig->group(m_groupname).writeEntry("JavaArgs", addArgED->text());
    m_pConfig->group(m_groupname).writePathEntry("JavaPath", pathED->lineEdit()->text());
    m_pConfig->group(m_groupname).writeEntry("UseSecurityManager", javaSecurityManagerCB->isChecked());
    m_pConfig->group(m_groupname).writeEntry("UseKio", useKioCB->isChecked());
    m_pConfig->group(m_groupname).writeEntry("ShutdownAppletServer", enableShutdownCB->isChecked());
    m_pConfig->group(m_groupname).writeEntry("AppletServerTimeout", serverTimeoutSB->value());

    domainSpecific->save(m_groupname, QStringLiteral("JavaDomains"));

    if (_removeJavaDomainSettings) {
        m_pConfig->group(m_groupname).deleteEntry("JavaDomainSettings");
        _removeJavaDomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
    setNeedsSave(false);
}

void KJavaOptions::slotChanged()
{
    setNeedsSave(true);
}

void KJavaOptions::toggleJavaControls()
{
    bool isEnabled = true; //enableJavaGloballyCB->isChecked();

    java_global_policies.setFeatureEnabled(enableJavaGloballyCB->isChecked());
    javaSecurityManagerCB->setEnabled(isEnabled);
    useKioCB->setEnabled(isEnabled);
    addArgED->setEnabled(isEnabled);
    pathED->setEnabled(isEnabled);
    enableShutdownCB->setEnabled(isEnabled);

    serverTimeoutSB->setEnabled(enableShutdownCB->isChecked() && isEnabled);
}

// == class JavaDomainListView =====

JavaDomainListView::JavaDomainListView(KSharedConfig::Ptr config, const QString &group,
                                       KJavaOptions *options, QWidget *parent)
    : DomainListView(config, i18nc("@title:group", "Doma&in-Specific"), parent),
      group(group), options(options)
{
}

JavaDomainListView::~JavaDomainListView()
{
}

void JavaDomainListView::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JavaPolicies pol(config, group, false);
    pol.defaults();
    const QStringList::ConstIterator itEnd = domainConfig.end();
    for (QStringList::ConstIterator it = domainConfig.begin(); it != itEnd; ++it) {
        QString domain;
        HtmlSettingsInterface::JavaScriptAdvice javaAdvice;
        HtmlSettingsInterface::JavaScriptAdvice javaScriptAdvice;
        HtmlSettingsInterface::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        if (javaAdvice != HtmlSettingsInterface::JavaScriptDunno) {
            QTreeWidgetItem *index = new QTreeWidgetItem(domainSpecificLV, QStringList() << domain <<
                    i18n(HtmlSettingsInterface::javascriptAdviceToText(javaAdvice)));
            pol.setDomain(domain);
            pol.setFeatureEnabled(javaAdvice != HtmlSettingsInterface::JavaScriptReject);
            domainPolicies[index] = new JavaPolicies(pol);
        }
    }
}

void JavaDomainListView::setupPolicyDlg(PushButton trigger, PolicyDialog &pDlg,
                                        Policies *pol)
{
    QString caption;
    switch (trigger) {
    case AddButton: caption = i18nc("@title:window", "New Java Policy");
        pol->setFeatureEnabled(!options->enableJavaGloballyCB->isChecked());
        break;
    case ChangeButton: caption = i18nc("@title:window", "Change Java Policy"); break;
    default:; // inhibit gcc warning
    }/*end switch*/
    pDlg.setWindowTitle(caption);
    pDlg.setFeatureEnabledLabel(i18n("&Java policy:"));
    pDlg.setFeatureEnabledWhatsThis(i18n("Select a Java policy for "
                                         "the above host or domain."));
    pDlg.refresh();
}

JavaPolicies *JavaDomainListView::createPolicies()
{
    return new JavaPolicies(config, group, false);
}

JavaPolicies *JavaDomainListView::copyPolicies(Policies *pol)
{
    return new JavaPolicies(*static_cast<JavaPolicies *>(pol));
}


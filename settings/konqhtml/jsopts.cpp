/*
    SPDX-FileCopyrightText: 1996 Martin R. Jones
    SPDX-FileCopyrightText: 1998 Bernd Wuebben

    SPDX-FileCopyrightText: 1998 Torben Weis
    KControl port & modifications

    SPDX-FileCopyrightText: 1998 David Faure
    End of the KControl port, added 'kfmclient configure' call.

    SPDX-FileCopyrightText: 2000 Kalle Dalheimer
    New configuration scheme for JavaScript

    SPDX-FileCopyrightText: 2000 Daniel Molkentin
    Major cleanup & Java/JS settings split

    SPDX-FileCopyrightText: 2002-2003 Leo Savernik
    Big changes to accommodate per-domain settings

    SPDX-FileCopyrightText: 2024 Stefano Crocco
    Use this as toplevel javascript KCM
*/

// Own
#include "jsopts.h"

#include "konqsettings.h"

// Qt
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDBusConnection>
#include <QDBusMessage>

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kurlrequester.h>
#include <KLocalizedString>

// Local
#include "htmlopts.h"
#include "policydlg.h"
#include <htmlextension.h>

using namespace Konq;

// == class KJavaScriptOptions =====
KJavaScriptOptions::KJavaScriptOptions(QObject *parent, const KPluginMetaData &md) :
    KCModule(parent, md),
    m_pConfig(KSharedConfig::openConfig()),
    js_global_policies(m_pConfig, m_groupname, true, QString())
{
    QVBoxLayout *toplevel = new QVBoxLayout(widget());

    enableJavaScriptGloballyCB = new QCheckBox(i18n("Ena&ble JavaScript globally"));
    enableJavaScriptGloballyCB->setToolTip(i18n("Enables the execution of scripts written in ECMA-Script "
            "(also known as JavaScript) that can be contained in HTML pages. "
            "Note that, as with any browser, enabling scripting languages can be a security problem."));
    connect(enableJavaScriptGloballyCB, &QAbstractButton::clicked, this, &KJavaScriptOptions::markAsChanged);
    connect(enableJavaScriptGloballyCB, &QAbstractButton::clicked, this, &KJavaScriptOptions::slotChangeJSEnabled);
    toplevel->addWidget(enableJavaScriptGloballyCB);

    // the domain-specific listview
    domainSpecific = new DomainListView(m_pConfig, m_groupname, this, widget());
    connect(domainSpecific, &DomainListView::changed, this, [this](bool changed){setNeedsSave(changed);});
    toplevel->addWidget(domainSpecific, 2);

    domainSpecific->setToolTip(i18n("Here you can set specific JavaScript policies for any particular "
                                      "host or domain. To add a new policy, simply click the <i>New...</i> "
                                      "button and supply the necessary information requested by the "
                                      "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                      "button and choose the new policy from the policy dialog box. Clicking "
                                      "on the <i>Delete</i> button will remove the selected policy causing the default "
                                      "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                      "button allows you to easily share your policies with other people by allowing "
                                      "you to save and retrieve them from a zipped file."));

    QString wtstr = i18n("<p>This box contains the domains and hosts you have set "
                         "a specific JavaScript policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                         "domains or hosts.</p><p>Select a policy and use the controls on "
                         "the right to modify it.</p>");
    domainSpecific->listView()->setToolTip(wtstr);

    domainSpecific->importButton()->setToolTip(i18n("Click this button to choose the file that contains "
            "the JavaScript policies. These policies will be merged "
            "with the existing ones. Duplicate entries are ignored."));
    domainSpecific->exportButton()->setToolTip(i18n("Click this button to save the JavaScript policy to a zipped "
            "file. The file, named <b>javascript_policy.tgz</b>, will be "
            "saved to a location of your choice."));

    // the frame containing the JavaScript policies settings
    js_policies_frame = new JSPoliciesFrame(&js_global_policies,
                                            i18n("Global JavaScript Policies"), widget());
    toplevel->addWidget(js_policies_frame);
    connect(js_policies_frame, &JSPoliciesFrame::changed, this, [this](){setNeedsSave(true);});

}

void KJavaScriptOptions::load()
{
    // *** load ***
    KConfigGroup cg(m_pConfig, m_groupname);
    domainSpecific->initialize(Settings::ecmaDomains());

    //TODO Settings: currently, the settings for each domain are stored in a separate group, which makes it
    //impossible to use KConfigXT to manage them. See whether it's possible to store the settings in a Json document
    // *** apply to GUI ***
    js_policies_frame->load();
    enableJavaScriptGloballyCB->setChecked(js_global_policies.isFeatureEnabled());
    KCModule::load();
}

void KJavaScriptOptions::defaults()
{
    js_policies_frame->defaults();
    enableJavaScriptGloballyCB->setChecked(js_global_policies.isFeatureEnabled());
    setNeedsSave(true);
    setRepresentsDefaults(true);
    KCModule::defaults();
}

void KJavaScriptOptions::save()
{
    KConfigGroup cg(m_pConfig, m_groupname);

    domainSpecific->save(m_groupname, QStringLiteral("ECMADomains"));
    js_policies_frame->save();


    // Send signal to konqueror
    // Warning. In case something is added/changed here, keep kfmclient in sync
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
    KCModule::save();
}

void KJavaScriptOptions::slotChangeJSEnabled()
{
    js_global_policies.setFeatureEnabled(enableJavaScriptGloballyCB->isChecked());
}

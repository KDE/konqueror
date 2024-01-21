/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configdialog.h"

#include <KCModule>

#include <QTabWidget>

using namespace Konq;

Konq::ConfigDialog::ConfigDialog(QWidget* parent) : KCMultiDialog(parent)
{
    setObjectName(QStringLiteral("configureDialog"));
    setFaceType(KPageDialog::Tree);

    QMap<Module, QString> modules {
        {GeneralModule, "konqueror_kcms/khtml_general"},
        {PerformanceModule, "konqueror_kcms/kcm_performance"},
        {TabsModule, "konqueror_kcms/khtml_tabs"},
        {BookmarksModule, "konqueror_kcms/kcm_bookmarks"},
        {KonqModule, "konqueror_kcms/kcm_konq"},
        {DolphinGeneralModule, "dolphin/kcms/kcm_dolphingeneral"},
#if QT_VERSION_MAJOR < 6
        {DolphinNavigationModule, "dolphin/kcms/kcm_dolphinnavigation"},
#endif
        {DolphinViewModesModule, "dolphin/kcms/kcm_dolphinviewmodes"},
        {TrashModule, "kcm_trash"},
        {FileTypesModule, "plasma/kcms/systemsettings_qwidgets/kcm_filetypes"},
        {HtmlBehaviorModule, "konqueror_kcms/khtml_behavior"},
        {HtmlAppearanceModule, "konqueror_kcms/khtml_appearance"},
        {AdBlockModule, "konqueror_kcms/khtml_filter"},
        {HtmlCacheModule, "konqueror_kcms/khtml_cache"},
        {WebShortcutsModule, "kcm_webshortcuts"},
        {ProxyModule, "kcm_proxy"},
        {HistoryModule, "konqueror_kcms/kcm_history"},
        {CookiesModule, "konqueror_kcms/khtml_cookies"},
        {JavaModule, "konqueror_kcms/khtml_java_js"},
        {UserAgentModule, "konqueror_kcms/khtml_useragent"}
    };

#ifdef Q_OS_WIN
    modules.remove(PerformanceModule);
#endif
    for (auto modIt = modules.constBegin(); modIt != modules.constEnd(); ++modIt) {
        KPageWidgetItem *it = addModule(KPluginMetaData(modIt.value()));
        //Attempt to remove the Behavior tab from the Dolphin general module, as it only applies to dolphin
        if (modIt.key() == DolphinGeneralModule) {
            QTabWidget *tw = it->widget()->findChild<QTabWidget*>();
            if(tw && tw->count() > 0) {
                tw->removeTab(0);
            }
        }
        m_pages[modIt.key()] = it;
    }
}

Konq::ConfigDialog::~ConfigDialog() noexcept
{
}

QSize Konq::ConfigDialog::computeSizeHint() const
{
    QSize size{KCMultiDialog::sizeHint()};
    for (auto it = m_pages.constBegin(); it != m_pages.constEnd(); ++it) {
        QSize s = it.value()->widget()->sizeHint();
        if (size.width() < s.width()) {
            size.setWidth(s.width());
        }
        if (size.height() < s.height()) {
            size.setHeight(s.height());
        }
    }
    // QList<KCModule*> modules = findChildren<KCModule*>();
    // for (auto m : modules) {
    //     QSize s = m->widget()->sizeHint();
    //     if (size.width() < s.width()) {
    //         size.setWidth(s.width());
    //     }
    //     if (size.height() < s.height()) {
    //         size.setHeight(s.height());
    //     }
    // }
    return size;
}

QSize Konq::ConfigDialog::sizeHint() const
{
    if (!m_sizeHint.isValid()) {
        m_sizeHint = computeSizeHint();
    }
    return m_sizeHint;
}

void Konq::ConfigDialog::setCurrentPage(Module module)
{
    KPageWidgetItem *page = m_pages.value(module);
    if (page) {
        KCMultiDialog::setCurrentPage(page);
    }
}

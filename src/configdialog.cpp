/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configdialog.h"

#include <KCModule>
#include <KLocalizedString>

#include <QTabWidget>

using namespace Konq;

QStringList ConfigDialog::s_extraModules;

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
        {DolphinViewModesModule, "dolphin/kcms/kcm_dolphinviewmodes"},
        {TrashModule, "kcm_trash"},
        {FileTypesModule, "plasma/kcms/systemsettings_qwidgets/kcm_filetypes"},
        {HtmlBehaviorModule, "konqueror_kcms/khtml_behavior"},
        {HtmlAppearanceModule, "konqueror_kcms/khtml_appearance"},
        {AdBlockModule, "konqueror_kcms/khtml_filter"},
        {HtmlCacheModule, "konqueror_kcms/khtml_cache"},
        {WebShortcutsModule, "plasma/kcms/systemsettings_qwidgets/kcm_webshortcuts"},
        {ProxyModule, "plasma/kcms/systemsettings_qwidgets/kcm_proxy"},
        {HistoryModule, "konqueror_kcms/kcm_history"},
        {CookiesModule, "konqueror_kcms/khtml_cookies"},
        {JavaModule, "konqueror_kcms/khtml_java_js"},
        {UserAgentModule, "konqueror_kcms/khtml_useragent"},
        {SpeedDialModule, "konqueror_kcms/kcm_speeddial"}
    };

#ifdef Q_OS_WIN
    modules.remove(PerformanceModule);
#endif
    for (auto modIt = modules.constBegin(); modIt != modules.constEnd(); ++modIt) {
        addPageForModule(modIt.value(), modIt.key());

    }

    for (const QString &mod : s_extraModules) {
        addPageForModule(mod);
    }
}

Konq::ConfigDialog::~ConfigDialog() noexcept
{
}

void ConfigDialog::registerExtraModule(const QString& path)
{
    s_extraModules.append(path);
}

void Konq::ConfigDialog::addPageForModule(const QString& fileName, std::optional<Module> type)
{
    KPluginMetaData md(fileName);
    //In KF6, the web shortcuts and the proxy KCMs are provided by kio-extras, which Konqueror doesn't require,
    //so avoid displaying an invalid entry if they can't be loaded for any reason
    if (!md.isValid() && (type == WebShortcutsModule || type == ProxyModule)) {
        return;
    }

    KPageWidgetItem *it = addModule(md);

    //Attempt to remove the Behavior tab from the Dolphin general module, as it only applies to dolphin
    if (type == DolphinGeneralModule) {
        fixDolphinGeneralPage(it);
    }

    m_pages.insert(type ? static_cast<int>(*type) : m_pages.count(), it);
    if (!type) {
        m_extraPages.insert(fileName, it);
    }
}

void Konq::ConfigDialog::fixDolphinGeneralPage(KPageWidgetItem *it)
{
    QTabWidget *tw = it->widget()->findChild<QTabWidget*>();
    if(tw && tw->count() > 0) {
        tw->removeTab(0);
    }
    QString name = i18nc("@title:tab The file manager behavior tab. Also used in the list of tabs for consistency", "File Manager Behavior");
    it->setName(name);
    it->setHeader(name);
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

void Konq::ConfigDialog::setCurrentPage(const QString& module)
{
    KPageWidgetItem *page = m_extraPages.value(module);
    if (page) {
        KCMultiDialog::setCurrentPage(page);
    }
}


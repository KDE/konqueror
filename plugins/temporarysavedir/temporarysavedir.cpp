/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "temporarysavedir.h"
#include "interfaces/browser.h"

#include <KPluginFactory>
#include <KPluginMetaData>
#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KParts/ReadOnlyPart>
#include <KUrlRequester>
#include <KUrlRequesterDialog>

#include <QMenu>
#include <QApplication>

K_PLUGIN_CLASS_WITH_JSON(TemporarySaveDirPlugin, "temporarysavedir.json")

TemporarySaveDirCore::TemporarySaveDirCore(QWidget *window) : QObject(window), m_window(window)
{
}

TemporarySaveDirCore::~TemporarySaveDirCore() noexcept
{
}

void TemporarySaveDirCore::setTemporarySaveDir(const QString& dir)
{
    m_temporarySaveDir = dir;
    if (m_window) {
        KonqInterfaces::Browser::browser(qApp)->setSaveDirForWindow(m_temporarySaveDir, m_window);
    }
}

QString TemporarySaveDirCore::temporarySaveDir() const
{
    return m_temporarySaveDir;
}

bool TemporarySaveDirCore::useTemporarySaveDir() const
{
    return !m_temporarySaveDir.isEmpty();
}

TemporarySaveDirPlugin::TemporarySaveDirPlugin(QObject *parent, const KPluginMetaData& metaData, const QVariantList &) : KonqParts::Plugin(parent)
{
    setMetaData(metaData);

    m_menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("document-save-as")), i18n("Temporary Download Directory"), actionCollection());
    actionCollection()->addAction(QStringLiteral("temporarysavedir_action_menu"), m_menu);
    m_menu->setPopupMode(QToolButton::InstantPopup);
    connect(m_menu->menu(), &QMenu::aboutToShow, this, &TemporarySaveDirPlugin::updateMenu);

    m_chooseTemporarySaveDir = actionCollection()->addAction(QStringLiteral("chooseTemporarySaveDir"));
    m_chooseTemporarySaveDir->setText(i18n("Choose Custom Download Directory..."));
    m_chooseTemporarySaveDir->setToolTip(i18n("Choose a custom directory to use when downloading URLs from this window, temporarily overriding the default one"));
    m_menu->addAction(m_chooseTemporarySaveDir);
    connect(m_chooseTemporarySaveDir , &QAction::triggered, this, &TemporarySaveDirPlugin::chooseTemporarySaveDir);

    m_clearTemporarySaveDirectory = actionCollection()->addAction(QStringLiteral("clearTemporarySaveDir"));
    m_clearTemporarySaveDirectory->setText(i18nc("@action:inmenu stop using a temporary download directory", "Stop Using Temporary Download Directory"));
    m_clearTemporarySaveDirectory->setToolTip(i18n("Use the default download directory instead of a custom one (only for this window)"));
    m_menu->addAction(m_clearTemporarySaveDirectory);
    m_clearTemporarySaveDirectory->setEnabled(false);
    connect(m_clearTemporarySaveDirectory, &KToggleAction::triggered, this, &TemporarySaveDirPlugin::clearTemporarySaveDir);

    QWidget *w = window();
    static constexpr const char* s_temporarySaveDirProperty = "temporarySaveDirCore";
    m_core = w->property(s_temporarySaveDirProperty).value<TemporarySaveDirCore*>();
    if (!m_core) {
        m_core = new TemporarySaveDirCore(w);
        w->setProperty(s_temporarySaveDirProperty, QVariant::fromValue(m_core));
    } else {
        m_clearTemporarySaveDirectory->setEnabled(m_core->useTemporarySaveDir());
    }
}

TemporarySaveDirPlugin::~TemporarySaveDirPlugin()
{
}

QWidget* TemporarySaveDirPlugin::window() const
{
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart*>(parent());
    if (!part || !part->widget()) {
        return nullptr;
    }
    return part->widget()->window();
}

void TemporarySaveDirPlugin::clearTemporarySaveDir()
{
    if (m_core) {
        m_core->setTemporarySaveDir({});
    }
}

void TemporarySaveDirPlugin::chooseTemporarySaveDir()
{
    if (!m_core) {
        return;
    }
    QWidget *w = window();
    QUrl startUrl;
    if (!m_core->temporarySaveDir().isEmpty()) {
        startUrl = QUrl::fromLocalFile(m_core->temporarySaveDir());
    }
    KUrlRequesterDialog dlg({}, i18n("Temporary save directory"), w);
    dlg.setWindowTitle(i18n("Choose Temporary Save Directory"));
    dlg.urlRequester()->setMode(KFile::Directory|KFile::LocalOnly);
    if (dlg.exec() == QDialog::Rejected) {
        return;
    }
    QString temporarySaveDir = dlg.selectedUrl().path();
    m_core->setTemporarySaveDir(temporarySaveDir);
}

void TemporarySaveDirPlugin::updateMenu()
{
    if (!m_core) {
        return;
    }
    m_clearTemporarySaveDirectory->setEnabled(m_core->useTemporarySaveDir());
}

#include "temporarysavedir.moc"

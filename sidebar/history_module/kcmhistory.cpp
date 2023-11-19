/*
    kcmhistory.cpp
    SPDX-FileCopyrightText: 2000, 2001 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2002 Stephan Binner <binner@kde.org>

    based on kcmtaskbar.cpp
    SPDX-FileCopyrightText: 2000 Kurt Granroth <granroth@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "kcmhistory.h"

// Qt
#include <QCheckBox>
#include <QPushButton>
#include <QIcon>
#include <QFontDialog>

// KDE
#include <KConfig>
#include <KConfigGroup>
// #include <KFontChooser>
#include <KLocalizedString>
#include <kmessagebox.h>
#include <KPluginFactory>

#include <konq_historyprovider.h>
#include <konqhistorysettings.h>

// Local

K_PLUGIN_CLASS_WITH_JSON(HistorySidebarConfig, "kcmhistory.json")

HistorySidebarConfig::HistorySidebarConfig(QObject *parent, const KPluginMetaData& md, const QVariantList &list)
    : KCModule(parent, md)
{
    m_settings = KonqHistorySettings::self();

    if (!HistoryProvider::exists()) {
        new KonqHistoryProvider(this);
    }

    QVBoxLayout *topLayout = new QVBoxLayout(widget());
    dialog = new KonqSidebarHistoryDlg(widget());

    dialog->comboDefaultAction->addItem(i18nc("Automatically decide which action to perform", "Auto"));
    dialog->comboDefaultAction->addItem(i18nc("Open URL in new tab", "Open in new tab"));
    dialog->comboDefaultAction->addItem(i18nc("Open URL in current tab", "Open in current tab"));
    dialog->comboDefaultAction->addItem(i18nc("Open URL in new window", "Open in new window"));

    QString defaultActionToolTip = i18n("Action to carry out when opening an URL from history:<dl>"
        "<dl><dt>Auto:</dt><dd>opens the URL in a new tab unless the current tab is empty or an intro page</dd>"
        "<dt>Open URL in a new tab:</dt><dd>always opens the URL in a new tab</dd>"
        "<dt>Open URL in current tab:</dt><dd>always opens the URL in the current tab</dd>"
        "<dt>Open URL in new window:</dt><dd>always opens the URL in a new window</dd>"
        "</dl>"
    );
    dialog->setToolTip(defaultActionToolTip);

    dialog->spinEntries->setRange(0, INT_MAX);
    dialog->spinExpire->setRange(0, INT_MAX);
    dialog->spinExpire->setSuffix(i18n("days"));

    dialog->spinNewer->setRange(0, INT_MAX);
    dialog->spinOlder->setRange(0, INT_MAX);

    dialog->comboNewer->insertItem(KonqHistorySettings::MINUTES,
                                   i18np("Minute", "Minutes", 0));
    dialog->comboNewer->insertItem(KonqHistorySettings::DAYS,
                                   i18np("Day", "Days", 0));

    dialog->comboOlder->insertItem(KonqHistorySettings::MINUTES,
                                   i18np("Minute", "Minutes", 0));
    dialog->comboOlder->insertItem(KonqHistorySettings::DAYS,
                                   i18np("Day", "Days", 0));

    connect(dialog->comboDefaultAction, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HistorySidebarConfig::configChanged);

    connect(dialog->cbExpire, &QAbstractButton::toggled, dialog->spinExpire, &QWidget::setEnabled);
    connect(dialog->spinExpire, QOverload<int>::of(&QSpinBox::valueChanged), this, &HistorySidebarConfig::slotExpireChanged);

    connect(dialog->spinNewer, QOverload<int>::of(&QSpinBox::valueChanged), this, &HistorySidebarConfig::slotNewerChanged);
    connect(dialog->spinOlder, QOverload<int>::of(&QSpinBox::valueChanged), this, &HistorySidebarConfig::slotOlderChanged);

    connect(dialog->btnFontNewer, &QAbstractButton::clicked, this, &HistorySidebarConfig::slotGetFontNewer);
    connect(dialog->btnFontOlder, &QAbstractButton::clicked, this, &HistorySidebarConfig::slotGetFontOlder);
    connect(dialog->btnClearHistory, &QAbstractButton::clicked, this, &HistorySidebarConfig::slotClearHistory);

    connect(dialog->cbDetailedTips, &QAbstractButton::toggled, this, &HistorySidebarConfig::configChanged);
    connect(dialog->cbExpire, &QAbstractButton::toggled, this, &HistorySidebarConfig::configChanged);
    connect(dialog->spinEntries, QOverload<int>::of(&QSpinBox::valueChanged), this, &HistorySidebarConfig::configChanged);
    connect(dialog->comboNewer, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HistorySidebarConfig::configChanged);
    connect(dialog->comboOlder, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HistorySidebarConfig::configChanged);

    dialog->show();
    topLayout->addWidget(dialog);
    load();
}

void HistorySidebarConfig::configChanged()
{
    setNeedsSave(true);
}

void HistorySidebarConfig::load()
{
    KConfig _config("konquerorrc");
    KConfigGroup config(&_config, "HistorySettings");

    dialog->comboDefaultAction->setCurrentIndex(config.readEntry("Default Action", 0));

    dialog->spinExpire->setValue(config.readEntry("Maximum age of History entries", 90));
    dialog->spinEntries->setValue(config.readEntry("Maximum of History entries", 500));
    dialog->cbExpire->setChecked(dialog->spinExpire->value() > 0);

    dialog->spinNewer->setValue(m_settings->m_valueYoungerThan);
    dialog->spinOlder->setValue(m_settings->m_valueOlderThan);

    dialog->comboNewer->setCurrentIndex(m_settings->m_metricYoungerThan);
    dialog->comboOlder->setCurrentIndex(m_settings->m_metricOlderThan);

    dialog->cbDetailedTips->setChecked(m_settings->m_detailedTips);

    m_fontNewer = m_settings->m_fontYoungerThan;
    m_fontOlder = m_settings->m_fontOlderThan;

    // enable/disable widgets
    dialog->spinExpire->setEnabled(dialog->cbExpire->isChecked());

    slotExpireChanged();
    slotNewerChanged(dialog->spinNewer->value());
    slotOlderChanged(dialog->spinOlder->value());

    setNeedsSave(false);
}

void HistorySidebarConfig::save()
{
    quint32 age   = dialog->cbExpire->isChecked() ? dialog->spinExpire->value() : 0;
    quint32 count = dialog->spinEntries->value();

    KonqHistoryProvider::self()->emitSetMaxAge(age);
    KonqHistoryProvider::self()->emitSetMaxCount(count);

    m_settings->m_defaultAction = static_cast<KonqHistorySettings::Action>(dialog->comboDefaultAction->currentIndex());

    m_settings->m_valueYoungerThan = dialog->spinNewer->value();
    m_settings->m_valueOlderThan   = dialog->spinOlder->value();

    m_settings->m_metricYoungerThan = dialog->comboNewer->currentIndex();
    m_settings->m_metricOlderThan   = dialog->comboOlder->currentIndex();

    m_settings->m_detailedTips = dialog->cbDetailedTips->isChecked();

    m_settings->m_fontYoungerThan = m_fontNewer;
    m_settings->m_fontOlderThan   = m_fontOlder;

    m_settings->applySettings();

    setNeedsSave(false);
}

void HistorySidebarConfig::defaults()
{
    dialog->comboDefaultAction->setCurrentIndex(0);

    dialog->spinEntries->setValue(500);
    dialog->cbExpire->setChecked(true);
    dialog->spinExpire->setValue(90);

    dialog->spinNewer->setValue(1);
    dialog->spinOlder->setValue(2);

    dialog->comboNewer->setCurrentIndex(KonqHistorySettings::DAYS);
    dialog->comboOlder->setCurrentIndex(KonqHistorySettings::DAYS);

    dialog->cbDetailedTips->setChecked(true);

    m_fontNewer = QFont();
    m_fontNewer.setItalic(true);
    m_fontOlder = QFont();
}

void HistorySidebarConfig::slotExpireChanged()
{
    configChanged();
}

// change hour to days, minute to minutes and the other way round,
// depending on the value of the spinbox, and synchronize the two spinBoxes
// to enforce newer <= older.
void HistorySidebarConfig::slotNewerChanged(int value)
{
    dialog->comboNewer->setItemText(KonqHistorySettings::DAYS,
                                    i18np("Day", "Days", value));
    dialog->comboNewer->setItemText(KonqHistorySettings::MINUTES,
                                    i18np("Minute", "Minutes", value));

    if (dialog->spinNewer->value() > dialog->spinOlder->value()) {
        dialog->spinOlder->setValue(dialog->spinNewer->value());
    }
    configChanged();
}

void HistorySidebarConfig::slotOlderChanged(int value)
{
    dialog->comboOlder->setItemText(KonqHistorySettings::DAYS,
                                    i18np("Day", "Days", value));
    dialog->comboOlder->setItemText(KonqHistorySettings::MINUTES,
                                    i18np("Minute", "Minutes", value));

    if (dialog->spinNewer->value() > dialog->spinOlder->value()) {
        dialog->spinNewer->setValue(dialog->spinOlder->value());
    }

    configChanged();
}

void HistorySidebarConfig::slotGetFontNewer()
{
    bool ok = false;
    m_fontNewer = QFontDialog::getFont(&ok, m_fontNewer, widget());
    if (ok) {
        configChanged();
    }
}

void HistorySidebarConfig::slotGetFontOlder()
{
    bool ok = false;
    m_fontOlder = QFontDialog::getFont(&ok, m_fontOlder, widget());
    if (ok) {
        configChanged();
    }
}

void HistorySidebarConfig::slotClearHistory()
{
    KGuiItem guiitem = KStandardGuiItem::clear();
    guiitem.setIcon(QIcon::fromTheme("edit-clear-history"));
    if (KMessageBox::warningContinueCancel(widget(),
                                           i18n("Do you really want to clear "
                                                   "the entire history?"),
                                           i18nc("@title:window", "Clear History?"), guiitem)
            == KMessageBox::Continue) {
        KonqHistoryProvider::self()->emitClear();
    }
}

#include "kcmhistory.moc"

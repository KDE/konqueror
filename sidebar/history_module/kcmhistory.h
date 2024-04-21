/*
    kcmhistory.h
    SPDX-FileCopyrightText: 2002 Stephan Binner <binner@kde.org>

    based on kcmtaskbar.h
    SPDX-FileCopyrightText: 2000 Kurt Granroth <granroth@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef __kcmhistory_h__
#define __kcmhistory_h__

#include <KCModule>
#include "ui_history_dlg.h"

class KonqHistoryManager;
class KonqHistorySettings;

class KonqSidebarHistoryDlg : public QWidget, public Ui::KonqSidebarHistoryDlg
{
public:
    KonqSidebarHistoryDlg(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
        layout()->setContentsMargins(0, 0, 0, 0);
    }
};

class HistorySidebarConfig : public KCModule
{
    Q_OBJECT

public:
    explicit HistorySidebarConfig(QObject *parent = nullptr, const KPluginMetaData& md={}, const QVariantList &list = {});

    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void configChanged();

    void slotGetFontNewer();
    void slotGetFontOlder();

    void slotExpireChanged();
    void slotNewerChanged(int);
    void slotOlderChanged(int);

    void slotClearHistory();

private:
    QFont m_fontNewer;
    QFont m_fontOlder;

    KonqSidebarHistoryDlg *dialog;
    KonqHistorySettings *m_settings;
    KonqHistoryManager *mgr;
};

#endif

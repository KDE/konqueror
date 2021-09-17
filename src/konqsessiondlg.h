/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#ifndef __konq_sessiondlg_h__
#define __konq_sessiondlg_h__

#include <QDialog>

#include <QMap>
#include <QString>
#include <QUrl>

class KonqViewManager;
class KonqMainWindow;

/**
 * This is the konqueror sessions administration dialog, which allows the user
 * to add, modify, rename and delete konqueror sessions.
 */
class KonqSessionDlg : public QDialog
{
    Q_OBJECT
public:
    explicit KonqSessionDlg(KonqViewManager *manager, QWidget *parent = nullptr);
    ~KonqSessionDlg() override;

protected Q_SLOTS:
    void slotOpen();
    void slotRename(QUrl dirpathTo = QUrl());
    void slotNew();
    void slotDelete();
    void slotSave();
    void slotSelectionChanged();

private:
    class KonqSessionDlgPrivate;
    KonqSessionDlgPrivate *const d;
    void loadAllSessions(const QString & = QString());
};

class KonqNewSessionDlg : public QDialog
{
    Q_OBJECT
public:
    enum Mode { NewFile, ReplaceFile };
    explicit KonqNewSessionDlg(QWidget *parent, KonqMainWindow *mainWindow, QString sessionName = QString(), Mode mode = NewFile);
    ~KonqNewSessionDlg() override;

protected Q_SLOTS:
    void slotAddSession();
    void slotTextChanged(const QString &text);
private:
    class KonqNewSessionDlgPrivate;
    KonqNewSessionDlgPrivate *const d;
};

#endif

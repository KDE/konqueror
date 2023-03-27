/*
    SPDX-FileCopyrightText: 2003 Antonio Larrosa <larrosa@kde.org>
    SPDX-FileCopyrightText: 2008 Matthias Grimrath <maps4711@gmx.de>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ARCHIVEDIALOG_H
#define ARCHIVEDIALOG_H

#include <kmainwindow.h>

#include <QUrl>
#include <QProcess>
#include <QPointer>

#include <kmessagewidget.h>


class QDialogButtonBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QTemporaryDir;
class QTemporaryFile;

class KUrlRequester;
class KJob;
class KPluralHandlingSpinBox;


class ArchiveDialog : public KMainWindow
{
    Q_OBJECT

public:
    ArchiveDialog(const QUrl &url, QWidget *parent = nullptr);
    ~ArchiveDialog() override;

protected:
    void saveSettings();
    void readSettings();

    bool queryClose() override;

protected slots:
    void slotArchiveTypeChanged(int idx);
    void slotSourceUrlChanged(const QString &text);
    void slotMessageLinkActivated(const QString &link);

private slots:
    void slotCreateButtonClicked();
    void slotCheckedDestination(KJob *job);
    void slotDeletedOldDestination(KJob *job);

    void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void slotCopiedArchive(KJob *job);
    void slotFinishedArchive(KJob *job);

private:
    void cleanup();
    void startDownloadProcess();
    void finishArchive();
    void setGuiEnabled(bool on);

    void showMessage(const QString &text, KMessageWidget::MessageType type = KMessageWidget::Information);
    void showMessageAndCleanup(const QString &text, KMessageWidget::MessageType type = KMessageWidget::Information);

private:
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_archiveButton;
    QPushButton *m_cancelButton;
    QWidget *m_guiWidget;
    KMessageWidget *m_messageWidget;

    KUrlRequester *m_pageUrlReq;
    KUrlRequester *m_saveUrlReq;

    QComboBox *m_typeCombo;
    KPluralHandlingSpinBox *m_waitTimeSpinbox;
    QCheckBox *m_noProxyCheck;
    QCheckBox *m_randomWaitCheck;
    QCheckBox *m_fixExtensionsCheck;
    QCheckBox *m_runInTerminalCheck;
    QCheckBox *m_closeWhenFinishedCheck;

    QUrl m_saveUrl;
    QString m_saveType;
    QString m_wgetProgram;
    QTemporaryDir *m_tempDir;
    QTemporaryFile *m_tempFile;

    QPointer<QProcess> m_archiveProcess;
};

#endif // ARCHIVEDIALOG_H

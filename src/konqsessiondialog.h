/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQSESSIONDIALOG_H
#define KONQSESSIONDIALOG_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QTreeWidget>
#include <QDialog>
#include <QFontMetrics>

#include <kconfig.h>
#include <konqprivate_export.h>
#include <config-konqueror.h>
#include <KX11Extras>
#include <KConfigGroup>

class KonqMainWindow;
class QDialogButtonBox;
class QTreeWidgetItem;

class SessionTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    SessionTreeWidget(QWidget* parent = nullptr);
    ~SessionTreeWidget();

    void fill(const QStringList &sessionFilePaths);

    static QString url(QTreeWidgetItem *item);
    static QString viewId(QTreeWidgetItem *item, int column = 0);
    QStringList discardedWindowList() const;

Q_SIGNALS:
    void sessionCountChanged(int count);

private Q_SLOTS:
    void slotItemChanged(QTreeWidgetItem *item, int column);

private:
    struct WindowId {
        QString session;
        QString window;
    };

    enum CustomRoles {IdRole = Qt::UserRole, UrlRole};
    enum ItemLevel {SessionLevel = 0, WindowLevel, ViewLevel};

    void fillSession(const QString &sessionFile);
    QTreeWidgetItem* createViewItem(QTreeWidgetItem *windowItem, const WindowId &windowId, const KConfigGroup &windowGroup, const QString &key);
    void updateCounts(QTreeWidgetItem *it, int column);
    ItemLevel itemLevel(QTreeWidgetItem *item);
    QString fullViewId(const QString &sessionFile, const QString &windowId, const QString &viewId);

private:
    QFontMetrics m_fontMetrics;
    int m_minWidth = 0;
    QString m_genericToolTip;
    int m_sessionItemsCount = 0;
    QHash<QTreeWidgetItem *, int> m_checkedSessionItems;
    static constexpr ItemLevel s_maxDisplayedLevel = WindowLevel;
    static constexpr ItemLevel s_lastLevel = ViewLevel;
    static constexpr double s_maxRelativeWidth = 0.5;
};

class SessionRestoreDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SessionRestoreDialog(const QStringList &sessionFilePaths, QWidget *parent = nullptr);
    ~SessionRestoreDialog() override;

    bool isEmpty() const;

    /**
     * Returns the list of session discarded/unselected by the user.
     */
    QStringList discardedWindowList() const;

    /**
     * Returns true if the don't show checkbox is checked.
     */
    bool isDontShowChecked() const;

    /**
     * Returns true if the corresponding session restore dialog should be shown.
     *
     * @param dontShowAgainName the name that identify the session restore dialog box.
     * @param result if not null, it will be set to the result that was chosen the last
     * time the dialog box was shown. This is only useful if the restore dialog box should
     * be shown.
     */
    static bool shouldBeShown(const QString &dontShowAgainName, int *result);

    /**
     * Save the fact that the session restore dialog should not be shown again.
     *
     * @param dontShowAgainName the name that identify the session restore dialog. If
     * empty, this method does nothing.
     * @param result the value (Yes or No) that should be used as the result
     * for the message box.
     */
    static void saveDontShow(const QString &dontShowAgainName, int result);

private Q_SLOTS:
    void slotClicked(bool);
    void showContextMenu(const QPoint &pos);
    void updateApplyButton(int sessionCount);

private:
    static QString sessionListToolTip();
    void createItemForSession(const QString& sessionFile);
    void createUi();

private:
    SessionTreeWidget *m_treeWidget;
    QDialogButtonBox *m_buttonBox;
    bool m_dontShowChecked = false;
};

#endif /* KONQSESSIONDIALOG_H */

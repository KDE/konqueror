/* This file is part of KDE
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQPOPUPMENUTEST_H
#define KONQPOPUPMENUTEST_H

#include <konq_popupmenu.h>
#include <QObject>
#include <KFileItem>
#include <KActionCollection>
class KNewFileMenu;

class KonqPopupMenuTest : public QObject
{
    Q_OBJECT
public:
    KonqPopupMenuTest();

private:

    /**
     * @brief Adjusts the expected actions to take into account different configurations
     *
     * Some menu entries are sensitive to a system configurations. This means that the same test
     * could pass on a system and fail on another depending on their configuration and/or installed
     * software. This function removes some entries from the list of expected actions depending on
     * whether they exist or not in the actual actions.
     *
     * @param expected the list of expected actions to be modified
     * @param actions the actual actions
     */
    static void adjustExpectedActions(QStringList &expected, const QStringList &actions);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testFile();
    void testFileInReadOnlyDirectory();
    void testFilePreviewSubMenu();
    void testSubDirectory();
    void testViewDirectory();
    void testViewReadOnlyDirectory();

    void testHtmlLink();
    void testHtmlPage();

private:
    KonqPopupMenu::Flags m_appFlags;

    KFileItem m_fileItem;
    KFileItem m_linkItem;
    KFileItem m_subDirItem;
    KFileItem m_thisDirectoryItem;
    QAction *m_cut;
    QAction *m_copy;
    QAction *m_paste;
    QAction *m_pasteTo;
    QAction *m_properties;
    QAction *m_rename;
    QAction *m_trash;
    QAction *m_newWindow;
    QAction *m_newTab;
    QAction *m_preview1;
    QAction *m_preview2;
    QActionGroup *m_tabHandlingActions;
    QActionGroup *m_previewActions;
    QActionGroup *m_htmlEditActions;
    QActionGroup *m_fileEditActions;
    QActionGroup *m_linkActions;
    QActionGroup *m_partActions;
    KNewFileMenu *m_newMenu;
    KActionCollection m_actionCollection;

    QString m_makefilePath;
    bool m_deleteMakefileDir;
};

#endif

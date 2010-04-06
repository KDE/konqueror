/* This file is part of the KDE libraries
 *  Copyright 2010 David Faure <faure@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License or ( at
 *  your option ) version 3 or, at the discretion of KDE e.V. ( which shall
 *  act as a proxy as in section 14 of the GPLv3 ), any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <kstandarddirs.h>
#include <globalbookmarkmanager.h>
#include <kdebug.h>
#include <qtest_kde.h>
#include <kbookmarkmanager.h>

#include "bookmarkmodel.h"
#include "commands.h"

// Return a list of bookmark addresses in KBookmarkManager.
class BookmarkLister : public KBookmarkGroupTraverser
{
public:
    BookmarkLister(const KBookmarkGroup& root) {
        traverse(root);
    }
    static QStringList list(KBookmarkManager* mgr) {
        BookmarkLister lister(mgr->root());
        return lister.m_list;
    }
    virtual void visit(const KBookmark& bk) {
        m_list.append(bk.address());
    }
    virtual void visitEnter(const KBookmarkGroup& /*group*/) {
        //m_list.append(group.address());
    }

private:
    QStringList m_list;
};

class KBookmarkModelTest : public QObject
{
    Q_OBJECT
public:
    KBookmarkModelTest() {}
private:

private Q_SLOTS:
    void initTestCase()
    {
        // TODO port away from that GlobalBookmarkManager singleton
        const QString filename = KStandardDirs::locateLocal("data", QLatin1String("konqueror/bookmarks.xml"));
        QFile::remove(filename);
        GlobalBookmarkManager::self()->createManager(filename, QString());
        m_bookmarkManager = GlobalBookmarkManager::self()->mgr();
        QCOMPARE(BookmarkLister::list(m_bookmarkManager), QStringList());
    }

    // The commands modify the model, so the test code uses the commands
    void testAddBookmark()
    {
        CreateCommand* cmd = new CreateCommand("/0", "test_bk", "www", KUrl("http://www.kde.org"));
        cmd->redo();
        QCOMPARE(BookmarkLister::list(m_bookmarkManager), QStringList() << "/0");
        cmd->undo();
        QCOMPARE(BookmarkLister::list(m_bookmarkManager), QStringList());
        delete cmd;
    }

    void testDeleteBookmark()
    {
        CreateCommand* cmd = new CreateCommand("/0", "test_bk", "www", KUrl("http://www.kde.org"));
        cmd->redo();
        QCOMPARE(BookmarkLister::list(m_bookmarkManager), QStringList() << "/0");
        DeleteCommand* deleteCmd = new DeleteCommand("/0");
        deleteCmd->redo();
        QCOMPARE(BookmarkLister::list(m_bookmarkManager), QStringList());
        deleteCmd->undo();
        QCOMPARE(BookmarkLister::list(m_bookmarkManager), QStringList() << "/0");

        delete cmd;
        delete deleteCmd;
    }

private:
    KBookmarkManager* m_bookmarkManager;
};

QTEST_KDEMAIN( KBookmarkModelTest, NoGUI )

#include "kmimefileparsertest.moc"




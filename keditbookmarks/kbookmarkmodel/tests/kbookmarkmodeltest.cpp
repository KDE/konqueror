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
#include <kdebug.h>
#include <qtest_kde.h>
#include <kbookmarkmanager.h>

#include "kbookmarkmodel/commandhistory.h"
#include "kbookmarkmodel/model.h"
#include "kbookmarkmodel/commands.h" // TODO provide public API in the model instead?

// Return a list of all bookmark addresses or urls in a KBookmarkManager.
class BookmarkLister : public KBookmarkGroupTraverser
{
public:
    BookmarkLister(const KBookmarkGroup& root) {
        traverse(root);
    }
    static QStringList addressList(KBookmarkManager* mgr) {
        BookmarkLister lister(mgr->root());
        return lister.m_addressList;
    }
    static QStringList urlList(KBookmarkManager* mgr) {
        BookmarkLister lister(mgr->root());
        return lister.m_urlList;
    }
    virtual void visit(const KBookmark& bk) {
        m_addressList.append(bk.address());
        m_urlList.append(bk.url().url());
    }
    virtual void visitEnter(const KBookmarkGroup& group) {
        m_addressList.append(group.address() + '/');
    }

private:
    QStringList m_addressList;
    QStringList m_urlList;
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
        const QString filename = KStandardDirs::locateLocal("data", QLatin1String("konqueror/bookmarks.xml"));
        QFile::remove(filename);
        m_bookmarkManager = KBookmarkManager::managerForFile(filename, QString());
        m_cmdHistory = new CommandHistory(this);
        m_cmdHistory->setBookmarkManager(m_bookmarkManager);
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList());
        m_model = new KBookmarkModel(m_bookmarkManager->root(), m_cmdHistory, this);
        QCOMPARE(m_model->rowCount(), 1); // the toplevel "Bookmarks" toplevel item
        m_rootIndex = m_model->index(0, 0);
        QVERIFY(m_rootIndex.isValid());
        QCOMPARE(m_model->rowCount(m_rootIndex), 0);
    }

    // The commands modify the model, so the test code uses the commands
    void testAddBookmark()
    {
        CreateCommand* cmd = new CreateCommand(m_model, "/0", "test_bk", "www", KUrl("http://www.kde.org"));
        cmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0");
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager), QStringList() << "http://www.kde.org");
        QCOMPARE(m_model->rowCount(m_rootIndex), 1);
        cmd->undo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList());
        QCOMPARE(m_model->rowCount(m_rootIndex), 0);
        delete cmd;
    }

    void testDeleteBookmark()
    {
        CreateCommand* cmd = new CreateCommand(m_model, "/0", "test_bk", "www", KUrl("http://www.kde.org"));
        cmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0");
        DeleteCommand* deleteCmd = new DeleteCommand(m_model, "/0");
        deleteCmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList());
        deleteCmd->undo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0");
        deleteCmd->redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList());

        delete cmd;
        delete deleteCmd;
    }

    void testCreateFolder()
    {
        CreateCommand folderCmd(m_model, "/0", "folder", "folder", true /*open*/);
        folderCmd.redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0/");
        QCOMPARE(m_model->rowCount(m_rootIndex), 1);

        const QString kde = "http://www.kde.org";
        CreateCommand cmd(m_model, "/0/0", "test_bk", "www", KUrl(kde));
        cmd.redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0/" << "/0/0");

        // Insert before this bookmark
        const QString first = "http://first.example.com";
        CreateCommand cmdFirstBk(m_model, "/0/0", "first_bk", "www", KUrl(first));
        cmdFirstBk.redo();
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0/" << "/0/0" << "/0/1");
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager), QStringList() << first << kde);

        // Move the kde bookmark before the first bookmark
        KBookmark kdeBookmark = m_bookmarkManager->findByAddress("/0/1");
        QCOMPARE(kdeBookmark.url().url(), kde);
        QModelIndex kdeIndex = m_model->indexForBookmark(kdeBookmark);
        QCOMPARE(kdeIndex.row(), 1);
        QCOMPARE(m_model->rowCount(kdeIndex.parent()), 2);

        QMimeData* mimeData = m_model->mimeData(QModelIndexList() << kdeIndex);
        bool ok = m_model->dropMimeData(mimeData, Qt::MoveAction, 0, 0, kdeIndex.parent());
        QVERIFY(ok);
        QCOMPARE(BookmarkLister::addressList(m_bookmarkManager), QStringList() << "/0/" << "/0/0" << "/0/1");
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager), QStringList() << kde << first);
        delete mimeData;

        // Move the kde bookmark after the bookmark called 'first'
        kdeBookmark = m_bookmarkManager->findByAddress("/0/0");
        kdeIndex = m_model->indexForBookmark(kdeBookmark);
        QCOMPARE(kdeIndex.row(), 0);
        mimeData = m_model->mimeData(QModelIndexList() << kdeIndex);
        ok = m_model->dropMimeData(mimeData, Qt::MoveAction, 2, 0, kdeIndex.parent());
        QVERIFY(ok);
        QCOMPARE(BookmarkLister::urlList(m_bookmarkManager), QStringList() << first << kde);
        delete mimeData;
    }

private:
    KBookmarkManager* m_bookmarkManager;
    KBookmarkModel* m_model;
    CommandHistory* m_cmdHistory;
    QModelIndex m_rootIndex; // the index of the "Bookmarks" root
};

QTEST_KDEMAIN( KBookmarkModelTest, NoGUI )

#include "kbookmarkmodeltest.moc"

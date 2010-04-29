/* This file is part of the KDE project
   Copyright (C) 2000, 2010 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef GLOBALBOOKMARKMANAGER_H
#define GLOBALBOOKMARKMANAGER_H

#include <kbookmark.h>
#include <QObject>

class CommandHistory;
class KBookmarkModel;
class KBookmark;
class KBookmarkManager;

class GlobalBookmarkManager : public QObject
{
    Q_OBJECT
public:
    typedef enum {HTMLExport, OperaExport, IEExport, MozillaExport, NetscapeExport} ExportType;

    // TODO port to K_GLOBAL_STATIC if we keep this class
    static GlobalBookmarkManager* self() { if (!s_mgr) { s_mgr = new GlobalBookmarkManager(); } return s_mgr; }
    ~GlobalBookmarkManager();
    KBookmarkGroup root();
    static KBookmark bookmarkAt(const QString & a);

    KBookmarkModel* model() const { return m_model; }
    KBookmarkManager* mgr() const { return m_mgr; }
    QString path() const;

    void createManager(const QString &filename, const QString &dbusObjectName, CommandHistory* commandHistory);
    void notifyManagers(const KBookmarkGroup& grp);
    void notifyManagers();
    bool managerSave();
    void saveAs(const QString &fileName);
    void doExport(ExportType type, const QString & path = QString());
    void setUpdate(bool update);

    void reloadConfig();

    // TODO move out
    static QString makeTimeStr(const QString &);
    static QString makeTimeStr(int);

private:
    GlobalBookmarkManager();
    KBookmarkManager *m_mgr;
    KBookmarkModel *m_model;
    static GlobalBookmarkManager *s_mgr;
};

#endif /* GLOBALBOOKMARKMANAGER_H */


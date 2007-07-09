/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef kfindpart__h
#define kfindpart__h

#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <kfileitem.h>
#include <kdebug.h>
#include <Qt3Support/Q3PtrList>

class KQuery;
class KAboutData;
class KAction;
class Kfind;

class KFindPart : public KParts::ReadOnlyPart
{
  friend class KFindPartBrowserExtension;
    Q_OBJECT
    Q_PROPERTY( bool showsResult READ showsResult )
public:
    KFindPart( QWidget * parentWidget, QObject *parent, const QStringList & /*args*/ );
    virtual ~KFindPart();

    static KAboutData *createAboutData();

    virtual bool doOpenURL( const KUrl &url );
    virtual bool doCloseURL() { return true; }
    virtual bool openFile() { return false; }

    bool showsResult() const { return m_bShowsResult; }

    virtual void saveState( QDataStream &stream );
    virtual void restoreState( QDataStream &stream );

  // "Cut" icons : disable those whose URL is in lst, enable the rest //added for konqdirpart
  //virtual void disableIcons( const KUrl::List & ){}
  //virtual const KFileItem * currentItem(){return 0;}

Q_SIGNALS:
    // Konqueror connects directly to those signals
    void started(); // started a search
    void clear(); // delete all items
    void newItems(const KFileItemList&); // found this/these item(s)
    void finished(); // finished searching
    void canceled(); // the user canceled the search
    void findClosed(); // close us
    void deleteItem( KFileItem *item);

protected Q_SLOTS:
    void slotStarted();
    void slotDestroyMe();
    void addFile(const KFileItem *item, const QString& matchingLine);
    /* An item has been removed, so update konqueror's view */
    void removeFile(KFileItem *item);
    void slotResult(int errorCode);
    void newFiles(const KFileItemList&);
#if 0
  // slots connected to the directory lister  //added for konqdirpart
//  virtual void slotStarted();
  virtual void slotCanceled(){}
  virtual void slotCompleted(){}
  virtual void slotNewItems( const KFileItemList& ){}
  virtual void slotDeleteItem( KFileItem * ){}
  virtual void slotRefreshItems( const KFileItemList& ){}
  virtual void slotClear(){}
  virtual void slotRedirection( const KUrl & ){}
#endif

private:
    Kfind * m_kfindWidget;
    KQuery *query;
    bool m_bShowsResult; // whether the dirpart shows the results of a search or not
    /**
     * The internal storage of file items
     */
    KFileItemList m_lstFileItems;
};

#endif

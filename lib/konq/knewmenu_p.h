/* This file is part of the KDE project
   Copyright (C) 1998-2006 David Faure <faure@kde.org>
                 2003      Sven Leiber <s.leiber@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KNEWMENU_P_H
#define KNEWMENU_P_H

#include <kdialog.h>
class KLineEdit;
class KUrlRequester;

/**
 * @internal
 * Dialog to ask for a filename and a URL, when creating a link to a URL.
 * Basically a merge of KLineEditDlg and KUrlRequesterDlg ;)
 * @author David Faure <faure@kde.org>
 */
class KUrlDesktopFileDlg : public KDialog
{
    Q_OBJECT
public:
    KUrlDesktopFileDlg( const QString& textFileName, const QString& textUrl, QWidget *parent = 0 );
    virtual ~KUrlDesktopFileDlg() {}

    /**
     * @return the filename the user entered (no path)
     */
    QString fileName() const;
    /**
     * @return the URL the user entered
     */
    KUrl url() const;

protected Q_SLOTS:
    void slotClear();
    void slotNameTextChanged( const QString& );
    void slotURLTextChanged( const QString& );
private:
    void initDialog( const QString& textFileName, const QString& defaultName, const QString& textUrl, const QString& defaultUrl );

    /**
     * The line edit widget for the fileName
     */
    KLineEdit *m_leFileName;
    /**
     * The URL requester for the URL :)
     */
    KUrlRequester *m_urlRequester;

    /**
     * True if the filename was manually edited.
     */
    bool m_fileNameEdited;
};

#endif /* KNEWMENU_P_H */

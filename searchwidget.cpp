/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "searchwidget.h"
#include <KMessageBox>
#include "searchwidget.moc"
#include <QWebPage>

SearchWidgetDialog::SearchWidgetDialog( QWidget *parent, QWebPage *page)
    : KDialog( parent ), m_page( page )
{
    setCaption( i18n( "Find Text" ) );
    setButtons( User1|Cancel );
    m_search = new SearchWidget( parent );
    setMainWidget( m_search );
    m_search->searchtext->setFocus();
    setButtonText( User1, i18n( "Find" ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotSearch() ) );
}

QString SearchWidgetDialog::searchText() const
{
    return m_search->searchtext->text();
}

bool SearchWidgetDialog::caseSensitive() const
{
    return m_search->sensitivecase->isChecked();
}

void SearchWidgetDialog::slotSearch()
{
    QWebPage::FindFlags flags;
    if ( caseSensitive() )
        flags |= QWebPage::FindCaseSensitively;
    bool status = m_page->findText( searchText(), flags );
    if ( !status )
    {
        KMessageBox::information( this, i18n( "No text found." ) );
    }
}

/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "bookmarkinfowidget.h"
#include "bookmarklistview.h"
#include "kbookmarkmodel/commands.h"
#include "kbookmarkmodel/commandhistory.h"
#include "globalbookmarkmanager.h"
#include "kbookmarkmodel/model.h"

#include <stdlib.h>

#include <QtCore/QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QFormLayout>

#include <klocale.h>
#include <kdebug.h>

#include <klineedit.h>

#include <kbookmark.h>

// SHUFFLE all these functions around, the order is just plain stupid
void BookmarkInfoWidget::showBookmark(const KBookmark &bk)
{
    // Fast exit if already shown, otherwise editing a title leads to a command after each keypress
    if (m_bk == bk)
        return;

    commitChanges();
    m_bk = bk;

    if (m_bk.isNull()) {
        // all read only and blank

        m_title_le->setReadOnly(true);
        m_title_le->setText(QString());

        m_url_le->setReadOnly(true);
        m_url_le->setText(QString());

        m_comment_le->setReadOnly(true);
        m_comment_le->setText(QString());

        m_visitdate_le->setReadOnly(true);
        m_visitdate_le->setText(QString());

        m_credate_le->setReadOnly(true);
        m_credate_le->setText(QString());

        m_visitcount_le->setReadOnly(true);
        m_visitcount_le->setText(QString());

        return;
    }

    // read/write fields
    m_title_le->setReadOnly( (bk.isSeparator()|| !bk.hasParent() )? true : false);
    if (bk.fullText() != m_title_le->text())
        m_title_le->setText(bk.fullText());

    m_url_le->setReadOnly(bk.isGroup() || bk.isSeparator());
    if (bk.isGroup()) {
         m_url_le->setText(QString());
    }
    else {
        // Update the text if and only if the text represents a different URL to that
        // of the current bookmark - the old method, "m_url_le->text() != bk.url().pathOrUrl()",
        // created difficulties due to the ambiguity of converting URLs to text. (#172647)
        if (KUrl(m_url_le->text()) != bk.url()) {
            const int cursorPosition = m_url_le->cursorPosition();
            m_url_le->setText(bk.url().pathOrUrl());
            m_url_le->setCursorPosition(cursorPosition);
        }
    }

    m_comment_le->setReadOnly((bk.isSeparator()|| !bk.hasParent()) ? true : false );
    QString commentText = bk.description();
    if (m_comment_le->text() != commentText) {
        const int cursorPosition = m_comment_le->cursorPosition();
        m_comment_le->setText(commentText);
        m_comment_le->setCursorPosition(cursorPosition);
    }

    // readonly fields
    updateStatus();

}

void BookmarkInfoWidget::updateStatus()
{
   //FIXME we don't want every metadata element, but only that with owner "http://www.kde.org"
   QString visitDate =
        GlobalBookmarkManager::makeTimeStr(m_bk.metaDataItem("time_visited"));
    m_visitdate_le->setReadOnly(true);
    m_visitdate_le->setText(visitDate);

    QString creationDate =
        GlobalBookmarkManager::makeTimeStr(m_bk.metaDataItem("time_added"));
    m_credate_le->setReadOnly(true);
    m_credate_le->setText(creationDate);

    // TODO - get the actual field name from the spec if it exists, else copy galeon
    m_visitcount_le->setReadOnly(true);
    m_visitcount_le->setText(m_bk.metaDataItem("visit_count"));
}

void BookmarkInfoWidget::commitChanges()
{
    commitTitle();
    commitURL();
    commitComment();
}

void BookmarkInfoWidget::commitTitle()
{
    // no more change compression
    titlecmd = 0;
}

void BookmarkInfoWidget::slotTextChangedTitle(const QString &str)
{
    if (m_bk.isNull() || !m_title_le->isModified())
        return;

    timer->start(1000);

    if(titlecmd)
    {
        titlecmd->modify(str);
        titlecmd->redo();
    }
    else
    {
        titlecmd = new EditCommand(m_model, m_bk.address(), 0, str);
        m_model->commandHistory()->addCommand(titlecmd);
    }
}

void BookmarkInfoWidget::commitURL()
{
    urlcmd = 0;
}

void BookmarkInfoWidget::slotTextChangedURL(const QString &str) {
    if (m_bk.isNull() || !m_url_le->isModified())
        return;

    timer->start(1000);

    if(urlcmd)
    {
        urlcmd->modify(str);
        urlcmd->redo();
    }
    else
    {
        urlcmd = new EditCommand(m_model, m_bk.address(), 1, str);
        m_model->commandHistory()->addCommand(urlcmd);
    }
}

void BookmarkInfoWidget::commitComment()
{
    commentcmd = 0;
}

void BookmarkInfoWidget::slotTextChangedComment(const QString &str) {
    if (m_bk.isNull() || !m_comment_le->isModified())
        return;

    timer->start(1000);

    if(commentcmd)
    {
        commentcmd->modify(str);
        commentcmd->redo();
    }
    else
    {
        commentcmd = new EditCommand(m_model, m_bk.address(), 2, str);
        m_model->commandHistory()->addCommand(commentcmd);
    }
}

void BookmarkInfoWidget::slotUpdate()
{
    const QModelIndexList & list = mBookmarkListView->selectionModel()->selectedRows();
    if( list.count() == 1)
    {
        QModelIndex index = *list.constBegin();
        showBookmark( mBookmarkListView->bookmarkModel()->bookmarkForIndex(index) );
    }
    else
        showBookmark( KBookmark() );
}

BookmarkInfoWidget::BookmarkInfoWidget(BookmarkListView * lv, KBookmarkModel* model, QWidget *parent)
    : QWidget(parent), m_model(model), mBookmarkListView(lv) {

    connect(mBookmarkListView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(slotUpdate()));

    connect(mBookmarkListView->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            SLOT(slotUpdate()));

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), SLOT(commitChanges()));

    titlecmd = 0;
    urlcmd = 0;
    commentcmd = 0;

    QHBoxLayout *layout = new QHBoxLayout(this);
    QFormLayout *form1 = new QFormLayout();
    QFormLayout *form2 = new QFormLayout();
    layout->addLayout(form1);
    layout->addLayout(form2);

    m_title_le = new KLineEdit(this);
    m_title_le->setClearButtonShown(true);
    form1->addRow(i18n("Name:"), m_title_le);

    connect(m_title_le, SIGNAL(textChanged(QString)),
                        SLOT(slotTextChangedTitle(QString)));
    connect(m_title_le, SIGNAL(editingFinished()), SLOT(commitTitle()));

    m_url_le = new KLineEdit(this);
    m_url_le->setClearButtonShown(true);
    form1->addRow(i18n("Location:"), m_url_le);

    connect(m_url_le, SIGNAL(textChanged(QString)),
                      SLOT(slotTextChangedURL(QString)));
    connect(m_url_le, SIGNAL(editingFinished()), SLOT(commitURL()));

    m_comment_le = new KLineEdit(this);
    m_comment_le->setClearButtonShown(true);
    form1->addRow(i18n("Comment:"), m_comment_le);

    connect(m_comment_le, SIGNAL(textChanged(QString)),
                          SLOT(slotTextChangedComment(QString)));
    connect(m_comment_le, SIGNAL(editingFinished()), SLOT(commitComment()));

    m_credate_le = new KLineEdit(this);
    form2->addRow(i18n("First viewed:"), m_credate_le);

    m_visitdate_le = new KLineEdit(this);
    form2->addRow(i18n("Viewed last:"), m_visitdate_le);

    m_visitcount_le = new KLineEdit(this);
    form2->addRow(i18n("Times visited:"), m_visitcount_le);

    showBookmark(KBookmark());
}

#include "bookmarkinfowidget.moc"


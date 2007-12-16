/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

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

// Own
#include "testlink.h"

// Qt
#include <QtCore/QTimer>
#include <QtGui/QPainter>

// KDE
#include <kdebug.h>

#include <kdatetime.h>
#include <kcharsets.h>
#include <kbookmarkmanager.h>

#include <kaction.h>
#include <klocale.h>

// Local
#include "toplevel.h"
#include "commands.h"
#include "bookmarkiterator.h"
#include "bookmarkmodel.h"

TestLinkItrHolder *TestLinkItrHolder::s_self = 0;

TestLinkItrHolder::TestLinkItrHolder()
    : BookmarkIteratorHolder() {
    // do stuff
}

void TestLinkItrHolder::doItrListChanged() {
    KEBApp::self()->setCancelTestsEnabled(count() > 0);
    if(count() == 0)
    {
        kDebug()<<"Notifing managers "<<m_affectedBookmark;
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(m_affectedBookmark).toGroup());
        m_affectedBookmark.clear();
    }
}

void TestLinkItrHolder::addAffectedBookmark( const QString & address )
{
    kDebug()<<"addAffectedBookmark "<<address;
    if(m_affectedBookmark.isNull())
        m_affectedBookmark = address;
    else
        m_affectedBookmark = KBookmark::commonParent(m_affectedBookmark, address);
    kDebug()<<" m_affectedBookmark is now "<<m_affectedBookmark;
}

/* -------------------------- */

TestLinkItr::TestLinkItr(QList<KBookmark> bks)
    : BookmarkIterator(bks) {
    m_job = 0;
}

TestLinkItr::~TestLinkItr() {
    //FIXME setStatus(m_oldStatus); if we didn't finish
    if (m_job) {
        // kDebug() << "JOB kill\n";
        m_job->disconnect();
        m_job->kill();
    }
}

void TestLinkItr::setStatus(const QString & text)
{
    EditCommand::setNodeText(curBk(), QStringList()<< "info" << "metadata" << "linkstate", text);
    CurrentMgr::self()->model()->emitDataChanged(curBk());
}

bool TestLinkItr::isApplicable(const KBookmark &bk) const {
    return (!bk.isGroup() && !bk.isSeparator());
}

void TestLinkItr::doAction() {
    kDebug()<<"TestLinkItr::doAction() "<<endl;
    m_job = KIO::get(curBk().url(), KIO::Reload, KIO::HideProgressInfo);
    m_job->addMetaData( QString("cookies"), QString("none") );
    m_job->addMetaData( QString("errorPage"), QString("false") );

    connect(m_job, SIGNAL( result( KJob *)),
            this, SLOT( slotJobResult(KJob *)));

    m_oldStatus = EditCommand::getNodeText(curBk(), QStringList()<< "info" << "metadata" << "linkstate");
    setStatus(i18n("Checking..."));
}

void TestLinkItr::slotJobResult(KJob *job) {
    kDebug()<<"TestLinkItr::slotJobResult()"<<endl;
    m_job = 0;

    KIO::TransferJob *transfer = (KIO::TransferJob *)job;
    QString modDate = transfer->queryMetaData("modified");

    if (transfer->error() || transfer->isErrorPage())
    {
        kDebug()<<"***********"<<transfer->error()<<"  "<<transfer->isErrorPage()<<endl;
        // can we assume that errorString will contain no entities?
        QString err = transfer->errorString();
        err.replace("\n", " ");
        setStatus(err);
    }
    else
    {
        if (!modDate.isEmpty())
            setStatus(modDate);
        else
            setStatus(i18n("OK"));
    }

    holder()->addAffectedBookmark(KBookmark::parentAddress(curBk().address()));
    delayedEmitNextOne();
    //FIXME check that we don't need to call kill()
}
#include "testlink.moc"

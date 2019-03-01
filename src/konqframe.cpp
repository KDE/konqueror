/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

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

// Own
#include "konqframe.h"

// Local
#include "konqtabs.h"
#include "konqview.h"
#include "konqviewmanager.h"
#include "konqframevisitor.h"
#include "konqframestatusbar.h"

// Qt
#include <QApplication>
#include <QEvent>
#include <QVBoxLayout>
#include <QUrl>

// KDE
#include <kactioncollection.h>
#include "konqdebug.h"
#include <kiconloader.h>
#include <KLocalizedString>
#include <ksqueezedtextlabel.h>
#include <konq_events.h>
#include <kconfiggroup.h>

KonqFrameBase::KonqFrameBase()
    : m_pParentContainer(nullptr)
{
}

QString KonqFrameBase::frameTypeToString(const KonqFrameBase::FrameType frameType)
{
    switch (frameType) {
    case View :
        return QStringLiteral("View");
    case Tabs :
        return QStringLiteral("Tabs");
    case ContainerBase :
        return QStringLiteral("ContainerBase");
    case Container :
        return QStringLiteral("Container");
    case MainWindow :
        return QStringLiteral("MainWindow");
    }
    Q_ASSERT(0);
    return QString();
}

KonqFrameBase::FrameType frameTypeFromString(const QString &str)
{
    if (str == QLatin1String("View")) {
        return KonqFrameBase::View;
    }
    if (str == QLatin1String("Tabs")) {
        return KonqFrameBase::Tabs;
    }
    if (str == QLatin1String("ContainerBase")) {
        return KonqFrameBase::ContainerBase;
    }
    if (str == QLatin1String("Container")) {
        return KonqFrameBase::Container;
    }
    if (str == QLatin1String("MainWindow")) {
        return KonqFrameBase::MainWindow;
    }
    Q_ASSERT(0);
    return KonqFrameBase::View;
}

KonqFrame::KonqFrame(QWidget *parent, KonqFrameContainerBase *parentContainer)
    : QWidget(parent)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrame::KonqFrame()";

    m_pLayout = nullptr;
    m_pView = nullptr;

    // the frame statusbar
    m_pStatusBar = new KonqFrameStatusBar(this);
    m_pStatusBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    connect(m_pStatusBar, &KonqFrameStatusBar::clicked, this, &KonqFrame::slotStatusBarClicked);
    connect(m_pStatusBar, &KonqFrameStatusBar::linkedViewClicked, this, &KonqFrame::slotLinkedViewClicked);
    m_separator = nullptr;
    m_pParentContainer = parentContainer;
}

KonqFrame::~KonqFrame()
{
    //qCDebug(KONQUEROR_LOG) << this;
}

bool KonqFrame::isActivePart()
{
    return (m_pView &&
            static_cast<KonqView *>(m_pView) == m_pView->mainWindow()->currentView());
}

void KonqFrame::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int /*id*/, int /*depth*/)
{
    if (m_pView) {
        m_pView->saveConfig(config, prefix, options);
    }
    //config.writeEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), statusbar()->isVisible() );
    if (this == docContainer) {
        config.writeEntry(QStringLiteral("docContainer").prepend(prefix), true);
    }
}

void KonqFrame::copyHistory(KonqFrameBase *other)
{
    Q_ASSERT(other->frameType() == KonqFrameBase::View);
    if (m_pView) {
        m_pView->copyHistory(static_cast<KonqFrame *>(other)->childView());
    }
}

KParts::ReadOnlyPart *KonqFrame::attach(const KonqViewFactory &viewFactory)
{
    KonqViewFactory factory(viewFactory);

    // Note that we set the parent to 0.
    // We don't want that deleting the widget deletes the part automatically
    // because we already have that taken care of in KParts...

    m_pPart = factory.create(this, nullptr);

    if (!m_pPart) {
        qCWarning(KONQUEROR_LOG) << "No part was created!";
        return nullptr;
    }
    if (!m_pPart->widget()) {
        qCWarning(KONQUEROR_LOG) << "The part" << m_pPart << "didn't create a widget!";
        delete m_pPart;
        m_pPart = nullptr;
        return nullptr;
    }

    attachWidget(m_pPart->widget());

    m_pStatusBar->slotConnectToNewView(nullptr, nullptr, m_pPart);

    return m_pPart;
}

void KonqFrame::attachWidget(QWidget *widget)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrame::attachInternal()";
    delete m_pLayout;

    m_pLayout = new QVBoxLayout(this);
    m_pLayout->setObjectName(QStringLiteral("KonqFrame's QVBoxLayout"));
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    m_pLayout->setSpacing(0);

    m_pLayout->addWidget(widget, 1);
    m_pLayout->addWidget(m_pStatusBar, 0);
    widget->show();

    m_pLayout->activate();

    installEventFilter(m_pView->mainWindow()); // for Ctrl+Tab
}

void KonqFrame::insertTopWidget(QWidget *widget)
{
    Q_ASSERT(m_pLayout);
    Q_ASSERT(widget);
    m_pLayout->insertWidget(0, widget);
    installEventFilter(m_pView->mainWindow()); // for Ctrl+Tab
}

void KonqFrame::setView(KonqView *child)
{
    m_pView = child;
    if (m_pView) {
        connect(m_pView, SIGNAL(sigPartChanged(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)),
                m_pStatusBar, SLOT(slotConnectToNewView(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)));
    }
}

void KonqFrame::setTitle(const QString &title, QWidget * /*sender*/)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrame::setTitle( " << title << " )";
    m_title = title;
    if (m_pParentContainer) {
        m_pParentContainer->setTitle(title, this);
    }
}

void KonqFrame::setTabIcon(const QUrl &url, QWidget * /*sender*/)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrame::setTabIcon( " << url << " )";
    if (m_pParentContainer) {
        m_pParentContainer->setTabIcon(url, this);
    }
}

void KonqFrame::slotStatusBarClicked()
{
    if (!isActivePart() && m_pView && !m_pView->isPassiveMode()) {
        m_pView->mainWindow()->viewManager()->setActivePart(part());
    }
}

void KonqFrame::slotLinkedViewClicked(bool mode)
{
    if (m_pView->mainWindow()->linkableViewsCount() == 2) {
        m_pView->mainWindow()->slotLinkView();
    } else {
        m_pView->setLinkedView(mode);
    }
}

void KonqFrame::slotRemoveView()
{
    m_pView->mainWindow()->viewManager()->removeView(m_pView);
}

void KonqFrame::activateChild()
{
    if (m_pView && !m_pView->isPassiveMode()) {
        m_pView->mainWindow()->viewManager()->setActivePart(part());

        if (!m_pView->isLoading() && (m_pView->url().isEmpty() || m_pView->url() == QUrl(QStringLiteral("about:blank")))) {
            //qCDebug(KONQUEROR_LOG) << "SET FOCUS on the location bar";
            m_pView->mainWindow()->focusLocationBar(); // #84867 usability improvement
        }
    }
}

KonqView *KonqFrame::childView() const
{
    return m_pView;
}

KonqView *KonqFrame::activeChildView() const
{
    return m_pView;
}

bool KonqFrame::accept(KonqFrameVisitor *visitor)
{
    return visitor->visit(this);
}


/* This file is part of the KDE project
   Copyright (C) 2003, George Staikos <staikos@kde.org>

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

#include "web_module.h"
#include "favicon_interface.h"

#include <QtCore/QFileInfo>
#include <QtGui/QSpinBox>
#include <QtCore/QTimer>

#include <dom/html_inline.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kparts/browserextension.h>
#include <konq_nameandurlinputdialog.h>
#include <kstandarddirs.h>
#include <khbox.h>


KonqSideBarWebModule::KonqSideBarWebModule(const KComponentData &componentData, QWidget *parent, const KConfigGroup& configGroup)
	: KonqSidebarModule(componentData, parent, configGroup)
{
	_htmlPart = new KHTMLSideBar();
        _htmlPart->setAutoDeletePart(false);
	connect(_htmlPart, SIGNAL(reload()), this, SLOT(reload()));
	connect(_htmlPart, SIGNAL(completed()), this, SLOT(pageLoaded()));
	connect(_htmlPart,
		SIGNAL(setWindowCaption(const QString&)),
		this,
		SLOT(setTitle(const QString&)));
	connect(_htmlPart,
		SIGNAL(openUrlRequest(QString, KParts::OpenUrlArguments, KParts::BrowserArguments)),
		this,
		SLOT(urlClicked(QString, KParts::OpenUrlArguments, KParts::BrowserArguments)));
	connect(_htmlPart->browserExtension(),
		SIGNAL(openUrlRequest(const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &)),
		this,
		SLOT(formClicked(const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &)) );
	connect(_htmlPart,
		SIGNAL(setAutoReload()), this, SLOT( setAutoReload() ));
	connect(_htmlPart,
		SIGNAL(openUrlNewWindow(QString,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs)),
		this,
		SLOT(urlNewWindow(QString,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::WindowArgs)));
	connect(_htmlPart,
		SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)),
		this,
		SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)));

        reloadTimeout = configGroup.readEntry("Reload", 0);
	_url = configGroup.readPathEntry("URL", QString());
	_htmlPart->openUrl(_url );
	// Must load this delayed
	QTimer::singleShot(0, this, SLOT(loadFavicon()));
}


KonqSideBarWebModule::~KonqSideBarWebModule() {
	delete _htmlPart;
	_htmlPart = 0L;
}


QWidget *KonqSideBarWebModule::getWidget() {
	return _htmlPart->widget();
}

void KonqSideBarWebModule::setAutoReload(){
	KDialog dlg( 0 );
  dlg.setModal( true );
  dlg.setCaption( i18n("Set Refresh Timeout (0 disables)" ) );
  dlg.setButtons( KDialog::Ok | KDialog::Cancel );

	KHBox *hbox = new KHBox( &dlg );
  dlg.setMainWidget( hbox );

	QSpinBox *mins = new QSpinBox( hbox );
	mins->setRange(0, 120);
	mins->setSuffix( i18n(" min") );
	QSpinBox *secs = new QSpinBox( 0, 59, 1, hbox );
	secs->setSuffix( i18n(" sec") );

	if( reloadTimeout > 0 )	{
		int seconds = reloadTimeout / 1000;
		secs->setValue( seconds % 60 );
		mins->setValue( ( seconds - secs->value() ) / 60 );
	}

	if( dlg.exec() == KDialog::Accepted ) {
		int msec = ( mins->value() * 60 + secs->value() ) * 1000;
		reloadTimeout = msec;
		configGroup().writeEntry("Reload", reloadTimeout);
		reload();
	}
}

void KonqSideBarWebModule::handleURL(const KUrl &) {
}


void KonqSideBarWebModule::urlNewWindow(const QString& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs, const KParts::WindowArgs& windowArgs)
{
    emit createNewWindow(KUrl(url), args, browserArgs, windowArgs);
}


void KonqSideBarWebModule::urlClicked(const QString& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs)
{
    emit openUrlRequest(KUrl(url), args, browserArgs);
}


void KonqSideBarWebModule::formClicked(const KUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs)
{
    _htmlPart->setArguments( args );
    _htmlPart->browserExtension()->setBrowserArguments(browserArgs);
    _htmlPart->openUrl(url);
}

void KonqSideBarWebModule::loadFavicon() {
    QString icon = KMimeType::favIconForUrl(_url);
    if (icon.isEmpty()) {
        org::kde::FavIcon favicon("org.kde.kded", "/modules/favicons", QDBusConnection::sessionBus());
        favicon.downloadHostIcon(_url.url());
        icon = KMimeType::favIconForUrl(_url);
    }

	if (!icon.isEmpty()) {
		emit setIcon(icon);

		if (icon != configGroup().readEntry("Icon", QString())) {
			configGroup().writeEntry("Icon", icon);
		}
	}
}


void KonqSideBarWebModule::reload() {
	_htmlPart->openUrl(_url);
}


void KonqSideBarWebModule::setTitle(const QString& title) {
    kDebug() << title;
    if (!title.isEmpty()) {
        emit setCaption(title);

        if (title != configGroup().readEntry("Name", QString())) {
            configGroup().writeEntry("Name", title);
        }
    }
}


void KonqSideBarWebModule::pageLoaded() {
	if( reloadTimeout > 0 ) {
		QTimer::singleShot( reloadTimeout, this, SLOT( reload() ) );
	}
}

bool KHTMLSideBar::urlSelected( const QString &url, int button,
                                int state, const QString &_target,
                                const KParts::OpenUrlArguments& args,
                                const KParts::BrowserArguments& browserArgs )
{
    if (button == Qt::LeftButton ){
        if (_target.toLower() == "_self") {
            openUrl(url);
        } else if (_target.toLower() == "_blank") {
            emit openUrlNewWindow(completeURL(url).url(), args);
        } else { // isEmpty goes here too
            emit openUrlRequest(completeURL(url).url(), args);
        }
        return true;
    }
    if (button == Qt::MidButton) {
        emit openUrlNewWindow(completeURL(url).url(),
                              args);
        return true;
    }
    // A refresh
    if (button == 0 && _target.toLower() == "_self") {
        openUrl(completeURL(url));
        return true;
    }
    return KHTMLPart::urlSelected(url, button, state, _target, args, browserArgs);
}

class KonqSidebarWebPlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarWebPlugin(QObject* parent, const QVariantList& args)
        : KonqSidebarPlugin(parent, args) {}
    virtual ~KonqSidebarWebPlugin() {}

    virtual KonqSidebarModule* createModule(const KComponentData &componentData, QWidget *parent,
                                            const KConfigGroup& configGroup,
                                            const QString &desktopname,
                                            const QVariant& unused)
    {
        Q_UNUSED(unused);
        Q_UNUSED(desktopname);
        return new KonqSideBarWebModule(componentData, parent, configGroup);
    }

    virtual QList<QAction*> addNewActions(QObject* parent,
                                          const QList<KConfigGroup>& existingModules,
                                          const QVariant& unused)
    {
        Q_UNUSED(unused);
        Q_UNUSED(existingModules);
        QAction* action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Web Sidebar Module"));
        action->setIcon(KIcon("internet-web-browser"));
        return QList<QAction *>() << action;
    }

    virtual QString templateNameForNewModule(const QVariant& actionData,
                                             const QVariant& unused) const
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);
        return QString::fromLatin1("websidebarplugin%1.desktop");
    }

    virtual bool createNewModule(const QVariant& actionData, KConfigGroup& configGroup,
                                 QWidget* parentWidget,
                                 const QVariant& unused)
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);

        KonqNameAndUrlInputDialog dlg(i18nc("@label", "Name:"), i18nc("@label", "Path or URL:"), KUrl(), parentWidget);
        dlg.setCaption(i18nc("@title:window", "Add web sidebar module"));
        if (!dlg.exec())
            return false;

        configGroup.writeEntry("Type", "Link");
        configGroup.writeEntry("Icon", "internet-web-browser");
        configGroup.writeEntry("Name", dlg.name());
        configGroup.writeEntry("URL", dlg.url().url());
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_web");
        return true;
    }
};

K_PLUGIN_FACTORY(KonqSidebarWebPluginFactory, registerPlugin<KonqSidebarWebPlugin>(); )
K_EXPORT_PLUGIN(KonqSidebarWebPluginFactory())

#include "web_module.moc"


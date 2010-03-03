/* This file is part of the KDE project
   Copyright (C) 2003 George Staikos <staikos@kde.org>

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

#ifndef web_module_h
#define web_module_h

#include <assert.h>
#include <khtml_part.h>
#include <kiconloader.h>
#include <klocale.h>
#include <konqsidebarplugin.h>
#include <kmenu.h>
#include <QtCore/QObject>


// A wrapper for KHTMLPart to make it behave the way we want it to.
class KHTMLSideBar : public KHTMLPart
{
    Q_OBJECT
public:
    KHTMLSideBar();
    virtual ~KHTMLSideBar() {}

	Q_SIGNALS:
		void submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&);
		void openUrlRequest(const QString& url,
                                    const KParts::OpenUrlArguments& args = KParts::OpenUrlArguments(),
                                    const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments() );
		void openUrlNewWindow(const QString& url,
                                      const KParts::OpenUrlArguments& args = KParts::OpenUrlArguments(),
                                      const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments(),
                                      const KParts::WindowArgs& windowArgs = KParts::WindowArgs());
		void reload();
		void setAutoReload();

	protected:
    virtual bool urlSelected( const QString &url, int button,
                              int state, const QString &_target,
                              const KParts::OpenUrlArguments& args = KParts::OpenUrlArguments(),
                              const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments() );

	protected Q_SLOTS:
		void loadPage() {
			emit openUrlRequest(completeURL(_lastUrl).url());
		}

		void loadNewWindow() {
			emit openUrlNewWindow(completeURL(_lastUrl).url());
		}

		void showMenu(const QString& url, const QPoint& pos) {
			if (url.isEmpty()) {
				_menu->popup(pos);
			} else {
				_lastUrl = url;
				_linkMenu->popup(pos);
			}
		}

		void formProxy(const char *action,
				const QString& url,
				const QByteArray& formData,
				const QString& target,
				const QString& contentType,
				const QString& boundary) {
			QString t = target.toLower();
			QString u;

			if (QString(action).toLower() != "post") {
				// GET
				KUrl kurl = completeURL(url);
				kurl.setQuery(formData.data());
				u = kurl.url();
			} else {
				u = completeURL(url).url();
			}

			// Some sites seem to use empty targets to send to the
			// main frame.
			if (t == "_content") {
				emit submitFormRequest(action, u, formData,
						target, contentType, boundary);
			} else if (t.isEmpty() || t == "_self") {
				setFormNotification(KHTMLPart::NoNotification);
				submitFormProxy(action, u, formData, target,
						contentType, boundary);
				setFormNotification(KHTMLPart::Only);
			}
		}
	private:
		KMenu *_menu, *_linkMenu;
		QString _lastUrl;
};



class KonqSideBarWebModule : public KonqSidebarModule
{
	Q_OBJECT
	public:
		KonqSideBarWebModule(const KComponentData &componentData, QWidget *parent,
                                     const KConfigGroup& configGroup);
		virtual ~KonqSideBarWebModule();

		virtual QWidget *getWidget();

	Q_SIGNALS:
                // TODO move to base class
		void submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&);
	protected:
		virtual void handleURL(const KUrl &url);

	private Q_SLOTS:
		void urlClicked(const QString& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs);
    void formClicked(const KUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs);
		void urlNewWindow(const QString& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs, const KParts::WindowArgs& windowArgs = KParts::WindowArgs());
		void pageLoaded();
		void loadFavicon();
		void setTitle(const QString&);
		void setAutoReload();
		void reload();

	private:
		KHTMLSideBar *_htmlPart;
		KUrl _url;
		int reloadTimeout;
};

#endif


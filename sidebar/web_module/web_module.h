/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2003 George Staikos <staikos@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef web_module_h
#define web_module_h

#include <assert.h>
#include <khtml_part.h>
#include <kiconloader.h>
#include <klocale.h>
#include <konqsidebarplugin.h>
#include <kmenu.h>
#include <QObject>

// A wrapper for KHTMLPart to make it behave the way we want it to.
class KHTMLSideBar : public KHTMLPart
{
    Q_OBJECT
public:
    KHTMLSideBar();
    virtual ~KHTMLSideBar() {}

Q_SIGNALS:
    void submitFormRequest(const char *, const QString &, const QByteArray &, const QString &, const QString &, const QString &);
    void openUrlRequest(const QString &url,
                        const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                        const BrowserArguments &browserArgs = BrowserArguments());
    void openUrlNewWindow(const QString &url,
                          const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                          const BrowserArguments &browserArgs = BrowserArguments(),
                          const KParts::WindowArgs &windowArgs = KParts::WindowArgs());
    void reload();
    void setAutoReload();

protected:
    virtual bool urlSelected(const QString &url, int button,
                             int state, const QString &_target,
                             const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                             const BrowserArguments &browserArgs = BrowserArguments());

protected Q_SLOTS:
    void loadPage()
    {
        emit openUrlRequest(completeURL(_lastUrl).url());
    }

    void loadNewWindow()
    {
        emit openUrlNewWindow(completeURL(_lastUrl).url());
    }

    void showMenu(const QString &url, const QPoint &pos)
    {
        if (url.isEmpty()) {
            _menu->popup(pos);
        } else {
            _lastUrl = url;
            _linkMenu->popup(pos);
        }
    }

    void formProxy(const char *action,
                   const QString &url,
                   const QByteArray &formData,
                   const QString &target,
                   const QString &contentType,
                   const QString &boundary)
    {
        QString t = target.toLower();
        QString u;

        if (QString(action).toLower() != "post") {
            // GET
            QUrl kurl = completeURL(url);
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
    KonqSideBarWebModule(QWidget *parent, const KConfigGroup &configGroup);
    virtual ~KonqSideBarWebModule();

    virtual QWidget *getWidget();

Q_SIGNALS:
    // TODO move to base class
    void submitFormRequest(const char *, const QString &, const QByteArray &, const QString &, const QString &, const QString &);
protected:
    virtual void handleURL(const QUrl &url);

private Q_SLOTS:
    void urlClicked(const QString &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs);
    void formClicked(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs);
    void urlNewWindow(const QString &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs, const KParts::WindowArgs &windowArgs = KParts::WindowArgs());
    void pageLoaded();
    void loadFavicon();
    void setTitle(const QString &);
    void setAutoReload();
    void reload();

private:
    KHTMLSideBar *_htmlPart;
    QUrl _url;
    int reloadTimeout;
};

#endif


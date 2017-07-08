#include "konqclientrequest.h"
#include <config-konqueror.h> // KONQ_HAVE_X11
#include <konq_main_interface.h>
#include <konq_mainwindow_interface.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QProcess>
#include <QUrl>

#if KONQ_HAVE_X11
#include <QX11Info>
#endif

#include <KConfig>
#include <KConfigGroup>
#include <KStartupInfo>
#include <KWindowSystem>

// keep in sync with konqpreloadinghandler.cpp
static const char s_preloadDBusName[] = "org.kde.konqueror.preloaded";

class KonqClientRequestPrivate
{
public:
    void sendASNChange();

    QUrl url;
    bool newTab = false;
    bool tempFile = false;
    QString mimeType;
    QByteArray startup_id_str;
};

KonqClientRequest::KonqClientRequest()
    : d(new KonqClientRequestPrivate)
{
    // read ASN env. variable
    d->startup_id_str = KStartupInfo::currentStartupIdEnv().id();
}

KonqClientRequest::~KonqClientRequest()
{
}

void KonqClientRequest::setUrl(const QUrl& url)
{
    d->url = url;
}

void KonqClientRequest::setNewTab(bool newTab)
{
    d->newTab = newTab;
}

void KonqClientRequest::setTempFile(bool tempFile)
{
    d->tempFile = tempFile;
}

void KonqClientRequest::setMimeType(const QString &mimeType)
{
    d->mimeType = mimeType;
}

void KonqClientRequestPrivate::sendASNChange()
{
#if KONQ_HAVE_X11
    if (KWindowSystem::platform() == KWindowSystem::Platform::X11) {
        KStartupInfoId id;
        id.initId(startup_id_str);
        KStartupInfoData data;
        data.addPid(0);     // say there's another process for this ASN with unknown PID
        data.setHostname(); // ( no need to bother to get this konqy's PID )
        KStartupInfo::sendChangeXcb(QX11Info::connection(), QX11Info::appScreen(), id, data);
    }
#endif
}

bool KonqClientRequest::openUrl()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    KConfig cfg(QStringLiteral("konquerorrc"));
    KConfigGroup fmSettings = cfg.group("FMSettings");
    if (d->newTab || fmSettings.readEntry("KonquerorTabforExternalURL", false)) {

        QString foundApp;
        QDBusObjectPath foundObj;
        QDBusReply<QStringList> reply = dbus.interface()->registeredServiceNames();
        if (reply.isValid()) {
            const QStringList allServices = reply;
            for (QStringList::const_iterator it = allServices.begin(), end = allServices.end(); it != end; ++it) {
                const QString service = *it;
                if (service.startsWith(QLatin1String("org.kde.konqueror"))) {
                    org::kde::Konqueror::Main konq(service, QStringLiteral("/KonqMain"), dbus);
                    QDBusReply<QDBusObjectPath> windowReply = konq.windowForTab();
                    if (windowReply.isValid()) {
                        QDBusObjectPath path = windowReply;
                        // "/" is the indicator for "no object found", since we can't use an empty path
                        if (path.path() != QLatin1String("/")) {
                            foundApp = service;
                            foundObj = path;
                        }
                    }
                }
            }
        }

        if (!foundApp.isEmpty()) {
            org::kde::Konqueror::MainWindow konqWindow(foundApp, foundObj.path(), dbus);
            QDBusReply<void> newTabReply = konqWindow.newTabASNWithMimeType(d->url.toString(), d->mimeType, d->startup_id_str, d->tempFile);
            if (newTabReply.isValid()) {
                d->sendASNChange();
                return true;
            }
        }
    }

    const QString appId = QString::fromLatin1(s_preloadDBusName);
    org::kde::Konqueror::Main konq(appId, QStringLiteral("/KonqMain"), dbus);
    QDBusReply<QDBusObjectPath> reply = konq.createNewWindow(d->url.toString(), d->mimeType, d->startup_id_str, d->tempFile);
    if (reply.isValid()) {
        d->sendASNChange();
        return true;
    } else {
        // pass kfmclient's startup id to konqueror using kshell
        KStartupInfoId id;
        id.initId(d->startup_id_str);
        id.setupStartupEnv();
        QStringList args;
        args << QStringLiteral("konqueror");
        if (!d->mimeType.isEmpty()) {
            args << QStringLiteral("--mimetype") << d->mimeType;
        }
        if (d->tempFile) {
            args << QStringLiteral("-tempfile");
        }
        args << d->url.toEncoded();
        qint64 pid;
#ifdef Q_OS_WIN
        const bool ok = QProcess::startDetached(QStringLiteral("kwrapper5"), args, QString(), &pid);
#else
        const bool ok = QProcess::startDetached(QStringLiteral("kshell5"), args, QString(), &pid);
#endif
        KStartupInfo::resetStartupEnv();
        if (ok) {
            qDebug() << "Konqueror started, pid=" << pid;
        } else {
            qWarning() << "Error starting konqueror";
        }
        return ok;
    }
}

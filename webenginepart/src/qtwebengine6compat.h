#ifndef QTWEBENGINE6COMPAT_H
#define QTWEBENGINE6COMPAT_H

//TODO KF6: when dropping support for Qt5, remove this file and include the files in the #else branch
#if QT_VERSION_MAJOR < 6
#include <QWebEngineDownloadItem>
#include <QWebEngineContextMenuData>
typedef QWebEngineDownloadItem QWebEngineDownloadRequest;
typedef QWebEngineContextMenuData QWebEngineContextMenuRequest;
#else
#include <QWebEngineDownloadRequest>
#include <QWebEngineContextMenuRequest>
#endif

#endif //QTWEBENGINE6COMPAT_H


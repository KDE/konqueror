#ifndef QTWEBENGINE6COMPAT_H
#define QTWEBENGINE6COMPAT_H

#include <QtGlobal>

//TODO KF6: when dropping support for Qt5, remove this file and include the files in the #else branch
#if QT_VERSION_MAJOR < 6
#include <QWebEngineDownloadItem>
#include <QWebEngineContextMenuData>
typedef QWebEngineDownloadItem QWebEngineDownloadRequest;
typedef QWebEngineContextMenuData QWebEngineContextMenuRequest;
typedef uint qHashReturnType;
#else
#include <QWebEngineDownloadRequest>
#include <QWebEngineContextMenuRequest>

//TODO: when dropping support for Qt5, change the return type of the various qHash functions from qHashReturnType to size_t
typedef size_t qHashReturnType;
#endif

#endif //QTWEBENGINE6COMPAT_H


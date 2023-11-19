#ifndef LIBKONQ_KF5COMPAT_H
#define LIBKONQ_KF5COMPAT_H

#include <QtGlobal>
#if QT_VERSION_MAJOR < 6
#include <KParts/BrowserExtension>
namespace KParts {
  typedef BrowserExtension NavigationExtension;
}
#else
#include <KParts/NavigationExtension>
#endif

#endif //LIBKONQ_KF5COMPAT_H



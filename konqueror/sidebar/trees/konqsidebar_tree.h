#ifndef KONQSIDEBAR_TREE_H
#define KONQSIDEBAR_TREE_H

#include <konqsidebarplugin.h>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/browserextension.h>
#include <kdialog.h>
#include <QtGui/QComboBox>
#include <QtCore/QStringList>
#include <klocale.h>
#include <QtGui/QLineEdit>

class KonqSidebarTree;

class KonqSidebar_Tree: public KonqSidebarPlugin
        {
                Q_OBJECT
                public:
                KonqSidebar_Tree(const KComponentData &componentData,QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0);
                ~KonqSidebar_Tree();
                virtual void *provides(const QString &);
//		void emitStatusBarText (const QString &);
                virtual QWidget *getWidget();
                protected:
                        class QWidget *widget;
                        class KonqSidebarTree *tree;
                        virtual void handleURL(const KUrl &url);
		protected Q_SLOTS:
			void copy();
			void cut();
			void paste();
			void trash();
			void del();
			void rename();
Q_SIGNALS:
			void openUrlRequest( const KUrl &url, const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                                             const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments());
  			void createNewWindow( const KUrl &url, const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                                             const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments(),
                                             const KParts::WindowArgs &windowArgs = KParts::WindowArgs(), KParts::ReadOnlyPart **part = 0 );
			void popupMenu( const QPoint &global, const KUrl &url,
					const QString &mimeType, mode_t mode = (mode_t)-1 );
			void popupMenu( const QPoint &global, const KFileItemList &items );
			void enableAction( const char * name, bool enabled );
        };

#endif // KONQSIDEBAR_TREE_H

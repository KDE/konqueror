#ifndef KONQSIDEBAR_TREE_H
#define KONQSIDEBAR_TREE_H

#include <konqsidebarplugin.h>

class KonqSidebarTree;

class KonqSidebar_Tree: public KonqSidebarModule
{
    Q_OBJECT
public:
    KonqSidebar_Tree(const KComponentData &componentData, QWidget *parent,
                     const QString &desktopName_, const KConfigGroup& configGroup);
    ~KonqSidebar_Tree();
    virtual QWidget *getWidget();
protected:
    class QWidget *widget;
    class KonqSidebarTree *tree;
    virtual void handleURL(const KUrl &url);
protected Q_SLOTS:
    void copy();
    void cut();
    void paste();
Q_SIGNALS:
    void enableAction( const char * name, bool enabled );
};

#endif // KONQSIDEBAR_TREE_H

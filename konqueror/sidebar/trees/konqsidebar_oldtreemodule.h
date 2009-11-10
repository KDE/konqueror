#ifndef KONQSIDEBAR_OLDTREEMODULE_H
#define KONQSIDEBAR_OLDTREEMODULE_H

#include <konqsidebarplugin.h>

class KonqSidebarTree;

class KonqSidebarOldTreeModule : public KonqSidebarModule
{
    Q_OBJECT
public:
    KonqSidebarOldTreeModule(const KComponentData &componentData, QWidget *parent,
                     const QString &desktopName_, const KConfigGroup& configGroup);
    ~KonqSidebarOldTreeModule();
    virtual QWidget *getWidget();

public Q_SLOTS:
    void copy();
    void cut();
    void paste();
    void pasteToSelection();

protected:
    QWidget *widget;
    KonqSidebarTree *tree;
    virtual void handleURL(const KUrl &url);
};

#endif // KONQSIDEBAR_TREEMODULE_H

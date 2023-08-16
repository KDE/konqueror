/*

    konq_extensionmanager.h - Extension Manager for Konqueror

    SPDX-FileCopyrightText: 2003 Martijn Klingens <klingens@kde.org>
    SPDX-FileCopyrightText: 2004 Arend van Beelen jr. <arend@auton.nl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQEXTENSIONMANAGER_H
#define KONQEXTENSIONMANAGER_H

#include <QDialog>

class KonqExtensionManagerPrivate;
class KonqMainWindow;
namespace KParts
{
class ReadOnlyPart;
}

/**
 * Extension Manager for Konqueror. See KPluginSelector in kdelibs for
 * documentation.
 *
 * @author Martijn Klingens <klingens@kde.org>
 * @author Arend van Beelen jr. <arend@auton.nl>
 */
class KonqExtensionManager
    : public QDialog
{
    Q_OBJECT

public:
    KonqExtensionManager(QWidget *parent, KonqMainWindow *mainWindow, KParts::ReadOnlyPart *activePart);
    ~KonqExtensionManager() override;

    void apply();

public Q_SLOTS:
    void setChanged(bool c);
    //TODO: Is this used anymore?
    void reparseConfiguration(const QByteArray &);
    void slotOk();
    void slotApply();
    void slotDefault();

protected:
    void showEvent(QShowEvent *event) override;

private:
    KonqExtensionManagerPrivate *d;
};

#endif // KONQEXTENSIONMANAGER_H

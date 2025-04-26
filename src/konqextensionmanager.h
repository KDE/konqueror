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
 * @brief Dialog where the user can enable or disable _part plugins_ for a specific part
 *
 * It uses a `KPluginWidget` to display the available plugins. The plugins are found using
 * `KPluginMetaData::findPlugins` and must be either:
 * - in the `konqueror/kpartplugins` directory
 * - in the `kpartplugins` subdirectory of a directory with the same name as the plugin id
 *
 * @author Martijn Klingens <klingens@kde.org>
 * @author Arend van Beelen jr. <arend@auton.nl>
 * @see KonqParts::Plugin
 */
class KonqExtensionManager
    : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     * @param parent the parent widget
     * @param mainWindow the window where the dialog should be displayed
     * @param activePart the part whose plugins should be enabled or disabled
     */
    KonqExtensionManager(QWidget *parent, KonqMainWindow *mainWindow, KParts::ReadOnlyPart *activePart);

    ~KonqExtensionManager() override; //!< Destructor

    /**
     * @brief Loads the chosen plugins
     */
    void apply();

public Q_SLOTS:

    /**
     * @brief Tells the object whether or not there are changes to be applied
     *
     * Currently, this only enables or disables the Apply button
     *
     * @param c `true` if there are changes to be applied and `false` otherwise
     */
    void setChanged(bool c);

    /**
     * @brief Reads again a configuration object
     *
     * @param conf the base name of the configuration object (without the `rc` suffix)
     */
    void reparseConfiguration(const QByteArray &conf);

    /**
     * @brief Slot called when the OK button is pressed
     *
     * It applies the changes before closing the dialog
     */
    void slotOk();

    /**
     * @brief Slot called when the Apply button is pressed
     *
     * It applies the current changes without closing the dialog
     */
    void slotApply();

    /**
     * @brief Slot called when the Default button is pressed
     *
     * It enables the default plugins and disables the others
     */
    void slotDefault();

protected:

    //TODO remove this
    /**
     * @brief Override of `QDialog::showEvent()`
     *
     * Currently this just calls the base class method
     *
     * @param event the event
     */
    void showEvent(QShowEvent *event) override;

private:
    KonqExtensionManagerPrivate *d; //!< The d-pointer
};

#endif // KONQEXTENSIONMANAGER_H

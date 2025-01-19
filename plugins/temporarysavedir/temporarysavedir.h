/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef TEMPORARYSAVEDIR_PLUGIN
#define TEMPORARYSAVEDIR_PLUGIN

#include <konq_kpart_plugin.h>

#include <QPointer>

class QAction;
class KActionMenu;

/**
 * @brief Class used to store the data for the TemporarySaveDirPlugin for each window
 * @see TemporarySaveDirPlugin
 */
class TemporarySaveDirCore : public QObject
{
    Q_OBJECT
public:
    TemporarySaveDirCore(QWidget *window);
    ~TemporarySaveDirCore();
    /**
     * @brief Sets the temporary save directory
     * @param dir the temporary save directory. Pass an empty string to disable the
     * use of the temporary save directory and use the default download directory
     */
    void setTemporarySaveDir(const QString &dir);
    QString temporarySaveDir() const;
    bool useTemporarySaveDir() const;

private:
    QPointer<QWidget> m_window;
    QString m_temporarySaveDir;
};

/**
 * @brief Plugin which allows the user to temporarily choose a directory to start the Save As dialogs in
 *
 * The temporary directory is used for all views inside a window. Since Konqueror doesn't yet support a
 * single instance of a plugin, but only an instance per part, the actual data is stored in an object of
 * class TemporarySaveDirCore which is created by the first TemporarySaveDirPlugin created for each window.
 * This object is stored in a property of the window itself so that other instances of the plugin for the same
 * window can access it.
 */
class TemporarySaveDirPlugin : public KonqParts::Plugin
{
    Q_OBJECT
public:
    TemporarySaveDirPlugin(QObject *parent, const KPluginMetaData& metaData, const QVariantList &);
    ~TemporarySaveDirPlugin() override;

private Q_SLOTS:
    void clearTemporarySaveDir();
    void chooseTemporarySaveDir();
    void updateMenu();

private:
    QWidget *window() const;

private:
    QPointer<TemporarySaveDirCore> m_core;
    KActionMenu *m_menu;
    QAction *m_chooseTemporarySaveDir;
    QAction *m_clearTemporarySaveDirectory;
};

#endif // TEMPORARYSAVEDIR_PLUGIN

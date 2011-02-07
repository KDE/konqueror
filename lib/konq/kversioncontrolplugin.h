/*****************************************************************************
 * Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>                 *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KVERSIONCONTROLPLUGIN_H
#define KVERSIONCONTROLPLUGIN_H

#include <libkonq_export.h>

#include <QObject>

class KFileItem;
class KFileItemList;
class QAction;

/**
 * @brief Base class for version control plugins.
 *
 * Enables the file manager to show the version state
 * of a versioned file. To write a custom plugin, the following
 * steps are required (in the example below it is assumed that a plugin for
 * Subversion will be written):
 *
 * - Create a fileviewsvnplugin.desktop file with the following content:
 *   <code>
 *   [Desktop Entry]
 *   Type=Service
 *   Name=Subversion
 *   X-KDE-ServiceTypes=FileViewVersionControlPlugin
 *   MimeType=text/plain;
 *   X-KDE-Library=fileviewsvnplugin
 *   </code>
 *
 * - Create a class FileViewSvnPlugin derived from KVersionControlPlugin and
 *   implement all abstract interfaces (fileviewsvnplugin.h, fileviewsvnplugin.cpp).
 *
 * - Take care that the constructor has the following signature:
 *   <code>
 *   FileViewSvnPlugin(QObject* parent, const QList<QVariant>& args);
 *   </code>
 *
 * - Add the following lines at the top of fileviewsvnplugin.cpp:
 *   <code>
 *   #include <KPluginFactory>
 *   #include <KPluginLoader>
 *   K_PLUGIN_FACTORY(FileViewSvnPluginFactory, registerPlugin<FileViewSvnPlugin>();)
 *   K_EXPORT_PLUGIN(FileViewSvnPluginFactory("fileviewsvnplugin"))
 *   </code>
 *
 * - Add the following lines to your CMakeLists.txt file:
 *   <code>
 *   kde4_add_plugin(fileviewsvnplugin fileviewsvnplugin.cpp)
 *   target_link_libraries(fileviewsvnplugin konq)
 *   install(FILES fileviewsvnplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})
 *   </code>
 *
 * General implementation notes:
 *
 *  - The implementations of beginRetrieval(), endRetrieval() and versionState()
 *    can contain blocking operations, as Dolphin will execute
 *    those methods in a separate thread. It is assured that
 *    all other methods are invoked in a serialized way, so that it is not necessary for
 *    the plugin to use any mutex.
 *
 * -  Dolphin keeps only one instance of the plugin, which is instantiated shortly after
 *    starting Dolphin. Take care that the constructor does no expensive and time
 *    consuming operations.
 *
 * @since 4.4
 */
class LIBKONQ_EXPORT KVersionControlPlugin : public QObject
{
    Q_OBJECT

public:
    enum VersionState
    {
        /** The file is not under version control. */
        UnversionedVersion,
        /**
         * The file is under version control and represents
         * the latest version.
         */
        NormalVersion,
        /**
         * The file is under version control and a newer
         * version exists on the main branch.
         */
        UpdateRequiredVersion,
        /**
         * The file is under version control and has been
         * modified locally. All modifications will be part
         * of the next commit.
         */
        LocallyModifiedVersion,
        /**
         * The file has not been under version control but
         * has been marked to get added with the next commit.
         */
        AddedVersion,
        /**
         * The file is under version control but has been marked
         * for getting removed with the next commit.
         */
        RemovedVersion,
        /**
         * The file is under version control and has been locally
         * modified. A modification has also been done on the main
         * branch.
         */
        ConflictingVersion,
        /**
         * The file is under version control and has local
         * modifications, which will not be part of the next
         * commit (or are "unstaged" in git jargon).
         * @since 4.6
         */
        LocallyModifiedUnstagedVersion
    };

    KVersionControlPlugin(QObject* parent = 0);
    virtual ~KVersionControlPlugin();

    /**
     * Returns the name of the file which stores
     * the version controls information.
     * (e. g. .svn, .cvs, .git).
     */
    virtual QString fileName() const = 0;

    /**
     * Is invoked whenever the version control
     * information will get retrieved for the directory
     * \p directory. It is assured that the directory
     * contains a trailing slash.
     */
    virtual bool beginRetrieval(const QString& directory) = 0;

    /**
     * Is invoked after the version control information has been
     * received. It is assured that
     * KVersionControlPlugin::beginInfoRetrieval() has been
     * invoked before.
     */
    virtual void endRetrieval() = 0;

    /**
     * Returns the version state for the file \p item.
     * It is assured that KVersionControlPlugin::beginInfoRetrieval() has been
     * invoked before and that the file is part of the directory specified
     * in beginInfoRetrieval().
     */
    // TODO: make const-correct for future versions where binary compatibility may get broken
    virtual VersionState versionState(const KFileItem& item) = 0;
    
    /**
     * Returns the list of actions that should be shown in the context menu
     * for the files \p items. It is assured that the passed list is not empty.
     * If an action triggers a change of the versions, the signal
     * KVersionControlPlugin::versionStatesChanged() must be emitted.
     */
    // TODO: make const-correct for future versions where binary compatibility may get broken
    virtual QList<QAction*> contextMenuActions(const KFileItemList& items) = 0;

    /**
     * Returns the list of actions that should be shown in the context menu
     * for the directory \p directory. If an action triggers a change of the versions,
     * the signal KVersionControlPlugin::versionStatesChanged() must be emitted.
     */
    // TODO: make const-correct for future versions where binary compatibility may get broken
    virtual QList<QAction*> contextMenuActions(const QString& directory) = 0;

signals:
    /**
     * Should be emitted when the version state of files might have been changed
     * after the last retrieval (e. g. by executing a context menu action
     * of the version control plugin). The file manager will be triggered to
     * update the version states of the directory \p directory by invoking
     * KVersionControlPlugin::beginRetrieval(),
     * KVersionControlPlugin::versionState() and
     * KVersionControlPlugin::endRetrieval().
     */
    void versionStatesChanged();

    /**
     * Is emitted if an information message with the content \a msg
     * should be shown.
     */
    void infoMessage(const QString& msg);

    /**
     * Is emitted if an error message with the content \a msg
     * should be shown.
     */
    void errorMessage(const QString& msg);

    /**
     * Is emitted if an "operation completed" message with the content \a msg
     * should be shown.
     */
    void operationCompletedMessage(const QString& msg);
};

#endif // KVERSIONCONTROLPLUGIN_H


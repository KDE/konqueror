//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef WEBENGINE_ACTONDOWNLOADEDFILEBAR_H
#define WEBENGINE_ACTONDOWNLOADEDFILEBAR_H

#include "browserarguments.h"

#include <KMessageWidget>
#include <KService>
#include <KPluginMetaData>

#include <QUrl>
#include <QPointer>

class WebEnginePart;

class QMenu;
class QTimer;
class QAbstractButton;

namespace WebEngine {

/**
 * @brief Message widget class allowing the user to open or embed a downloaded file
 *
 * This widget is shown by WebEnginePart when a file is downloaded but neither embedded or opened
 * (for example because the user used the "Save link as..." context menu action. It allows the user
 * to open the downloaded file in an external application or to display it in Konqueror.
 *
 * This is useful in case the user wants both to save the file and to look at its contents.
 *
 * To avoid disrupting the user workflow in case he isn't interested at opening it, this widget
 * is automatically hidden after 5 seconds, unless the user starts interacting with it.
 *
 * This widget provides the user three buttons:
 * - open: opens the file in an external application
 * - embed: shows the file in Konqueror replacing the current view
 * - embed in a new tab: shows the file in a new tab inside Konqueror
 *
 * If there are multiple applications able to open the file, the "Open" button also has a menu where
 * the user can choose which application to use. The same for the "Embed here" and "Embed in new tab"
 * buttons if there are multiple available parts.
 *
 * @warning Don't call `setText()` on this class. The widget text is determined internally from the parameters
 * given in the constructor
 *
 * @note This class is meant to be used for a single download only, as can be seen from the fact that it doesn't
 * provide ways to change the remote or local URLs.
 */
class ActOnDownloadedFileBar : public KMessageWidget {
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @note The constructor doesn't set the widget text because long URLs need to be elided to avoid
     * increasing the window width. Since in the constructor the widget's width is still unknown, there's
     * no way to do so. The text is set the in resizeEvent() in response to the first resize event the
     * widget receives.
     * @param url the remote URL which was downloaded. It's only used to build the widget's label
     * @param downloadUrl the URL to the local file where @p url was downloaded
     * @param part the part this widget is displayed in
     */
    ActOnDownloadedFileBar(const QUrl &url, const QUrl &downloadUrl, WebEnginePart *part);

    /**
     * @brief Destructor
     */
    ~ActOnDownloadedFileBar();

protected:

    /**
     * @brief Overrides KMessageWidget::resizeEvent
     *
     * It takes care of creating the widget text so that it fits the widget width.
     * It only does so the first time the widget is resized. After that it works as
     * the base class implementation.
     *
     * This is needed because in the constructor we don't know how much space the
     * widget has for text.
     * @param event the resize event
     */
    void resizeEvent(QResizeEvent * event) override;

private:

    void setElidedText();

    /**
     * @brief Creates a menu containing a list of all applications which can be used to open the file
     *
     * If there are less than two applications available, no menu is created as there's no choice to
     * be made.
     *
     * The `storageId` of the application associated to each action can be retrieved using the action `data()`
     * member.
     *
     * @param apps a list of suitable applications
     * @return the new menu or `nullptr` if @p apps contains less than two entries
     */
    QMenu* createOpenWithMenu(const KService::List &apps);

    /**
     * @brief Creates a menu containing a list of all parts which can be used to embed the file
     *
     * If there are less than two parts available, no menu is created as there's no choice to
     * be made.
     *
     * The id of the part associated to each action can be retrieved using the action `data()`
     * member.
     *
     * @param parts a list of suitable parts
     * @return the new menu or `nullptr` if @p parts contains less than two entries
     */
    QMenu* createEmbedWithMenu(const QList<KPluginMetaData> &parts);

    /**
     * @brief Helper function which carries out tasks common to both createEmbedWithMenu and createOpenWithMenu
     *
     * It creates the menu and make connections so that showing the menu stops the timer to close the widget
     * and closing it starts it again.
     *
     * @param actions the actions to insert in the menu
     * @return a new menu if @p actions contains at least two entries and `nullptr` otherwise
     */
    QMenu* createMenu(const QList<QAction*> &actions);

    /**
     * @brief Fills the contents of the "Open" action and creates its menu
     */
    void setupOpenAction();

    /**
     * @brief Fills the contents of the "Show" or "Show in new tab" actions and creates its menu
     * @param embedAction the action to prepare
     */
    void setupEmbedAction(QAction *embedAction);

    /**
     * @brief A list of all the buttons in the bar
     *
     * As `KMessageWidget` doesn't provide access to buttons, this uses `QObject::findChildren` with template
     * argument `QAbstractButton*`
     * @return a list of all `QAbstractButton` which are chilred of this object
     */
    QList<QAbstractButton*> buttons() const;

private Q_SLOTS:
    /**
     * @brief Carries out the choice made by the user
     *
     * @param choice whether to open or embed the file
     * @param newTab whether the file should be embedded in a new tab or here. It's ignored if @p action is #Open
     * @param data the data describing the part or application to use, if the user chose one from the menu.
     *  If invalid, the default application or part is used.
     */
    void actOnChoice(Konq::UrlAction choice, bool newTab, const QVariant &data);

private:
    QPointer<WebEnginePart> m_part; //!< The part associated with this widget
    QUrl m_url; //!<The remote URL
    QUrl m_downloadUrl; //!< The URL of the local file the remote URL was downloaded in
    QAction *m_openAction = nullptr; //!< The action which opens the file in an external application
    QAction *m_embedActionHere = nullptr; //!< The action which embeds the file in the current view
    QAction *m_embedActionNewTab = nullptr; //!< The action which embeds the file in a new tab
    QString m_mimeType; //!< The mimetype of the file. It's determined using `QMimeDatabase::mimeTypeForFile`
    QTimer* m_timer; //!< Timer which hides the widget. It's stopped when the user opens a menu and restarted when he closes it
};

}

#endif // WEBENGINE_ACTONDOWNLOADEDFILEBAR_H

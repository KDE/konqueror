/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_CONFIGDIALOG_H
#define KONQ_CONFIGDIALOG_H

#include <KCMultiDialog>

#include <QPointer>

namespace Konq {

/**
* @brief Konqueror configuration dialog
*
* The modules displayed in the configuration dialog can be of two types:
* - core KCMs, which are hard-coded and provide configuration for core Konqueror
* functionality
* - extra KCMs, which can be added by parts or plugins and allow to configure
* the functionality they provide.
*
* From the user point of view there's no difference between core and extra KCMs.
*/
class ConfigDialog : public KCMultiDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent the parent widget
     */
    ConfigDialog(QWidget *parent = nullptr);

    /**
     * @brief destructor
     */
    ~ConfigDialog();

    /**
     * @brief Overrides `KCMultiDialog::sizeHint()` to take into account the size of all pages
     *
     * This is needed because `KCMultiDialog::sizeHint()` only takes into account the size hint of the current page
     *
     * @return a `QSize` whose width and height are the maximum between the width and height of `KCMultiDialog::sizeHint()`
     * and of the size hints of each page
     * @note Once this method is called, this value will cached and won't be updated anymore
     */
    QSize sizeHint() const override;

    /**
     * @brief Enum describing the different core KCMs
     */
    enum Module {
        GeneralModule, //!< The general module
        PerformanceModule, //!< The performance module
        TabsModule, //!< The tab options module
        BookmarksModule, //!< The bookmarks module
        KonqModule, //!< The Konqueror behavior module
        DolphinGeneralModule, //!< The Dolphin general module
        DolphinViewModesModule, //!< The Dolphin views module
        TrashModule, //!< The trash module
        FileTypesModule, //!< The file types module
        HtmlBehaviorModule, //!< The HTML behavior module
        HtmlAppearanceModule, //!< The HTML HTML appearance module
        AdBlockModule, //!< The AdBlock module
        HtmlCacheModule, //!< The HTML cache module
        WebShortcutsModule, //!< The web shortcuts module
        ProxyModule, //!< The proxy module
        HistoryModule, //!< The history module
        CookiesModule, //!< The cookies module
        JavaModule, //!< The java/javascript module
        UserAgentModule, //!< The user agent module
        SpeedDialModule, //!< The speeddial module
    };

    /**
     * @brief Overload of `KCMultiDialog::setCurrentPage()` which activates the a given core module
     *
     * It sets the current page to any of the core modules, that is any for which
     * there exists a value in #Module.
     *
     * @param module the module which should become current
     */
    void setCurrentPage(Module module);

    /**
     * @brief Overload of `KCMultiDialog::setCurrentPage()` which activates the page corresponding to a non-core module
     *
     * It sets the page associated with the module at the given path to be the current one.
     *
     * @param module the path of the library providing the module which should become current
     */
    void setCurrentPage(const QString &module);

    /**
     * @brief Adds the module provided by the given library to the list of extra modules to load
     *
     * After a call to this function, a newly created ConfigDialog will include the module provided
     * by @p path among its pages.
     *
     * @param path the path of the library providing the KCM. It should be either an absolute path
     * or a path relative to one of the directories in `QStandardPaths::libraryPaths()`.
     */
    static void registerExtraModule(const QString &path);

private:

    /**
     * @brief Computes the size hint from the size hints of each module and from `KCMultiDialog::sizeHint()`
     */
    QSize computeSizeHint() const;

    /**
     * @brief Adjusts the General Dolphin KCM to better integrate it with Konqueror
     *
     * This function does two things:
     * - it removes from the KCM a tab which contains options which Konqueror doesn't use,
     * as not to confuse the user
     * - it changes the name and header of the KCM to make it more descriptive
     */
    void fixDolphinGeneralPage(KPageWidgetItem *it);

    /**
     * @brief Adds the page for a KCM
     *
     * This function creates the module, adds it to the dialog and inserts it in
     * #m_pages and, for extra KCMs, also to #m_extraPages.
     *
     * This function applies fixes for some specific KCMs.
     *
     * @param fileName the path of the library providing the KCM. It should be either an absolute path
     * or a path relative to one of the directories in `QStandardPaths::libraryPaths()`
     * @param type the module type of the KCM to add if it's a core KCM or `std::nullopt` for extra KCMs
     */
    void addPageForModule(const QString &fileName, std::optional<Module> type = std::nullopt);

    mutable QSize m_sizeHint; //!< The cached value of the size hint

    /**
     * @brief A map associating each page to a number
     *
     * For core KCMs the number is the corresponding #Module value. For other modules
     * the number is an arbitrary number determined at runtime, which shouldn't be used.
     */
    QHash<int, QPointer<KPageWidgetItem>> m_pages; //!< A hash mapping module types to pages
    QHash<QString, QPointer<KPageWidgetItem>> m_extraPages; //!< A hash associating each extra module with its page

    /**
     * @brief A list of the libraries providing the extra KCMs to load
     *
     * Each entry must be either an absolute path or a path relative to one of the directories in `QStandardPaths::libraryPaths()`.
     */
    static QStringList s_extraModules;
};
}
#endif //KONQ_CONFIGDIALOG_H

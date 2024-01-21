/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_CONFIGDIALOG_H
#define KONQ_CONFIGDIALOG_H

#include <KCMultiDialog>

#include <QPointer>

namespace Konq {

/**
* @brief Konqueror configuration dialog
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
     * @brief Enum describing the different pages
     */
    enum Module {
        GeneralModule, //!< The general module
        PerformanceModule, //!< The performance module
        TabsModule, //!< The tab options module
        BookmarksModule, //!< The bookmarks module
        KonqModule, //!< The Konqueror behavior module
        DolphinGeneralModule, //!< The Dolphin general module
#if QT_VERSION_MAJOR < 6
        DolphinNavigationModule, //!< The Dolphin navigation module
#endif
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
        UserAgentModule //!< The user agent module
    };

    /**
     * @brief Overload of `KCMultiDialog::setCurrentPage()` taking a Module
     * @param module the module to display
     */
    void setCurrentPage(Module module);

private:

    /**
     * @brief Computes the size hint from the size hints of each module and from `KCMultiDialog::sizeHint()`
     */
    QSize computeSizeHint() const;

    mutable QSize m_sizeHint; //!< The cached value of the size hint
    QHash<Module, QPointer<KPageWidgetItem>> m_pages; //!< A hash mapping module types to pages
};
}
#endif //KONQ_CONFIGDIALOG_H

/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef USERAGENT_H
#define USERAGENT_H

#include <KCModule>
#include <KSharedConfig>

#include <QDialog>
#include <QScopedPointer>
#include <QTimer>

class QTreeWidgetItem;

namespace Ui
{
class UserAgent;
}

/**
 * KCM which allows the user to configure the user agent string.
 *
 * It allows to choose a default user agent string and to create customized templates
 */
class UserAgent : public KCModule
{
    Q_OBJECT

public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     * @param md as in `KCModule` constructor
     * @param args as in `KCModule` constructor
     */
    UserAgent(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});

    /**
     * @brief Destructor
     */
    ~UserAgent();

#if QT_VERSION_MAJOR < 6
    void setNeedsSave(bool needs) {emit changed(needs);}
#endif

public slots:

    /**
     * @brief Loads the settings from the configuration files
     *
     * It fills the templates widget, enables or disable the custom user agent check box
     * and fills the custom user agent string
     */
    void load() override;

    /**
     * @brief Resets the KCM to its default values
     */
    void defaults() override;

    /**
     * @brief Saves the user settings
     *
     */
    void save() override;

private slots:

    /**
     * @brief Slot called when the user toggles the "Use custom user agent string" checkbox
     *
     * It enables or disables the custom user agent widget and the button to use the selected template
     * as custom user agent string
     * @param on whether the custom user agent should be enabled or disabled
     */
    void toggleCustomUA(bool on);

    /**
     * @brief Enables or disables the "use selected template" button
     *
     * The button is enabled if the use of a custom user agent is enabled and the template widget
     * has an item selected; otherwise it's disabled.
     */
    void enableDisableUseSelectedTemplateBtn();

    /**
     * @brief Fills the custom user agent lineedit with the selected template
     */
    void useSelectedTemplate();

    /**
     * @brief Fills the custom user agent lineedit with the double clicked template
     *
     * @param it the item the user doubled clicked on
     */
    void useDblClickedTemplate(QTreeWidgetItem *it, int);

    /**
     * @brief Creates a new template
     *
     * The user is shown a dialog to choose the name of the new template
     */
    void createNewTemplate();

    /**
     * @brief Creates a new template with the same content as the selected template
     *
     * The user is shown a dialog to choose the name of the new template
     */
    void duplicateTemplate();

    /**
     * @brief Delete the currently selected template
     */
    void deleteTemplate();

    /**
     * @brief Starts editing the currently selected template value
     *
     * Editing happens inline
     */
    void editTemplate();

    /**
     * @brief Starts renaming the currently selected template value
     *
     * Renaming happens inline
     */
    void renameTemplate();

    /**
     * @brief Enables or disables buttons depending on whether the template widget has a selection or not
     */
    void templateSelectionChanged();

    /**
     * @brief Slot called when the name or contents of a template changed
     *
     * It uses checkTemplatesValidity() to check if all templates are still valid and emits the `changed()` signal.
     * @param col the column which has changed
     */
    void templateChanged(QTreeWidgetItem*, int col);

    /**
     * @brief Checks whether the templates are valid
     *
     * If the templates aren't valid, the user is shown a messagewidget warning him of the problems.
     *
     * Templates are valid if they all have a name and there aren't duplicate names.
     * @note A template with an empty user agent string is considered valid, as the user may want not to
     * send an user agent string
     */
    void checkTemplatesValidity();

private:

    /**
     * @brief Whether or not the user has choose to enable a custom User Agent
     *
     * @return `true` if the user has enabled the use of a custom User Agent and
     * `false` if he chose to use the default User Agent.
     */
    bool useCustomUserAgent() const;

    /**
     * @brief Type representing templates with a name and a user agent string
     */
    using TemplateMap = QMap<QString, QString>;

    /**
     * @brief Fills the template widget with the given entries
     * @note All existing entries are removed from the template widgets before adding the new ones
     * @param templates the templates to insert in the widget
     */
    void fillTemplateWidget(const TemplateMap &templates);

    /**
     * @brief Fills the custom user agent lineedit with the given string
     */
    void useTemplate(const QString &templ);

    /**
     * @brief The selected item in the template widget
     *
     * @return The selected item in the template widget or `nullptr` if there's no selection
     */
    QTreeWidgetItem* selectedTemplate() const;

    /**
     * @brief Creates a new template
     *
     * This method is used by `createNewTemplate` and `duplicateTemplate` and does the following:
     * - asks the user for the name of the new template
     * - creates the `QTreeWidgetItem` item for the new template, with the chosen name
     * - selects the new item
     * @return the new item or `nullptr` if the user chose to cancel the operation
     */
    QTreeWidgetItem* createNewTemplateInternal();

    /**
     * @brief Converts the contents of the template widget to a TemplateMap
     *
     * @return A TemplateMap where the first column represents the keys and the second the values
     */
    TemplateMap templatesFromUI() const;

    /**
     * @brief Writes the templates to the config file
     */
    void saveTemplates();

    /**
     * @brief The user agent string used by WebEnginePart by default.
     *
     * @return The default WebEnginePart user agent string
     * @internal
     * @note Obtaining this string isn't easy because, once `QWebEngineProfile::setHttpUserAgent` has been called,
     * `QWebEngineProfile` doesn't provide a way to get the original value. To avoid this problem, a new `QWebEngineProfile`
     * could be created; however, this would be a waste of resources, so the following alternative approach is used:
     * - we assume that if someone called `QWebEngineProfile::setHttpUserAgent` on the default profile, it also set a
     * dynamic property called `defaultUserAgent` on the profile
     * - if the default profile has the `defaultUserAgent` dynamic property, its contents are used as default user agent string
     * - if the default profile doesn't have the `defaultUserAgent` dynamic property, we assume that `QWebEngineProfile::setHttpUserAgent`
     * hasn't been called and use `QWebEngineProfile::httpUserAgent()` as default user agent string
     */
    static QString defaultUserAgent();

private:
    /**
     * @brief The UI object
     */
    QScopedPointer<Ui::UserAgent> m_ui;

    /**
     * @brief The main config object
     */
    KSharedConfig::Ptr m_config;

    /**
     * @brief The config object where templates are saved
     */
    KSharedConfig::Ptr m_templatesConfig;
};

#endif // USERAGENT_H

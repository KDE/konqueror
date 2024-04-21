/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef TABSOPTIONS_H
#define TABSOPTIONS_H

#include <KCModule>
#include <KSharedConfig>

namespace Ui {
    class TabsOptions;
}

/**
 * @brief KCM to display tab-related options
 */
class TabsOptions : public KCModule
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
    TabsOptions(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});

    /**
     * @brief Destructor
     */
    ~TabsOptions();

public slots:

    /**
     * @brief Loads the settings from the configuration files
     */
    void load() override;

    /**
     * @brief Resets the KCM to its default values
     */
    void defaults() override;

    /**
     * @brief Saves the user settings
     */
    void save() override;

private:

    QScopedPointer<Ui::TabsOptions> m_ui; //!< The UI object

    KSharedConfig::Ptr m_config; //!< The configuration object
};

#endif //TABSOPTIONS_H

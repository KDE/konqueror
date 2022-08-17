/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef CACHE_H
#define CACHE_H

#include <KCModule>
#include <KSharedConfig>

#include <QDialog>
#include <QScopedPointer>

namespace Ui
{
class Cache;
}

/**
 * KCM which allows the user to configure the use of cache
 */
class Cache: public KCModule
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    Cache(QWidget* parent, const QVariantList&);

    /**
     * @brief Destructor
     */
    ~Cache();

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
     *
     */
    void save() override;

private slots:

    /**
     * @brief Slot called when the user toggles the "Don't store memory on disk" checkbox
     *
     * It enables or disables the "Use custom cache path" widgets
     * @param on whether memory cache was enabled or disabled
     */
    void toggleMemoryCache(bool on);

private:
    /**
     * @brief The UI object
     */
    QScopedPointer<Ui::Cache> m_ui;

    /**
     * @brief The main config object
     */
    KSharedConfig::Ptr m_config;
};

#endif // CACHE_H

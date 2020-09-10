/*
 * This file is part of the KDE project.
 *
 * Copyright 2020  Stefano Crocco <posta@stefanocrocco.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WEBENGINECUSTOMIZECACHEABLEFIELDSDLG_H
#define WEBENGINECUSTOMIZECACHEABLEFIELDSDLG_H

#include <QDialog>

#include "webenginewallet.h"

class WebFieldsDataViewPasswordDelegate;
class WebFieldsDataModel;

namespace Ui {
    class WebEngineCustomizeCacheableFieldsDlg;
};

/**
 * @brief Dialog where the user can choose which fields in a web page should be saved in KWallet
 *
 * The most important part of the dialog is a table which displays information about the fields in the page. Each field has a check box which the user
 * can use to select which fields should be cached. The list of fields chosen by the user can be obtained using selectedFields().
 *
 * The dialog also contains three check box:
 * - a check box to toggle displaying details about each field
 * - a check box to toggle showing or hiding passwords
 * - a check box to request caching fields contents immediately after the dialog has been closed. This choice can be obtained using immediatelyCacheData().
 */
class WebEngineCustomizeCacheableFieldsDlg : public QDialog
{
    Q_OBJECT

public:

    typedef QMap<QString, QStringList> OldCustomizationData;
    /**
     * @brief Constructor
     *
     * @param forms a list of all the forms in the page
     * @param oldCustomization the previous custom configuration. Each key of the map must be the name of one form and the corresponding value must be a list of
     * the chosen fields in that form. This is used to decide which fields should be initially checked. If the map is empty, the initially selected fields will be decided automatically
     * @param parent the parent widget
     */
    WebEngineCustomizeCacheableFieldsDlg(const WebEngineWallet::WebFormList &forms, const OldCustomizationData &oldCustomization,  QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~WebEngineCustomizeCacheableFieldsDlg(){}

    /**
     * @brief The list of fields chosen by the user
     *
     * @return a list of the fields the user has checked
     */
    WebEngineWallet::WebFormList selectedFields() const;

    /**
     * @brief Whether the user has requested to immediately save the contents of the selected field
     *
     * If this is @b true, it means that the user has checked the check box asking to cache the fields contents as soon as the dialog is closed.
     * The code which created the dialog should take care to do so.
     * @return Whether fields contents should be cached immediately or not
     */
    bool immediatelyCacheData() const;

private:

    void addChecksToPreviouslyChosenItems(const WebEngineWallet::WebFormList &forms, const OldCustomizationData &data);

    ///@brief The model used by the table
    WebFieldsDataModel *m_model;

    ///@brief The ui object
    Ui::WebEngineCustomizeCacheableFieldsDlg *m_ui;
};

#endif // WEBENGINECUSTOMFORMFIELDSWALLETDLG_H

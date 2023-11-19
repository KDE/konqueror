// /* This file is part of the KDE project
//     SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>
// 
//     SPDX-License-Identifier: LGPL-2.0-or-later
// */

#ifndef CHOOSEPAGESAVEFORMATDLG_H
#define CHOOSEPAGESAVEFORMATDLG_H

#include "qtwebengine6compat.h"

#include <QDialog>
#include <QScopedPointer>

class QButtonGroup;

namespace Ui
{
class ChoosePageSaveFormatDlg;
}

/**
 * Dialog where the user can choose the format to save a full web page
 */
class ChoosePageSaveFormatDlg : public QDialog
{
    Q_OBJECT

public:
    ChoosePageSaveFormatDlg(QWidget* parent=nullptr);
    ~ChoosePageSaveFormatDlg();

    QWebEngineDownloadRequest::SavePageFormat choosenFormat() const;

private slots:
    void updateInfoText(int id);

private:
    QScopedPointer<Ui::ChoosePageSaveFormatDlg> m_ui;
    QButtonGroup *m_choicesGroup;
};

#endif // CHOOSEPAGESAVEFORMATDLG_H

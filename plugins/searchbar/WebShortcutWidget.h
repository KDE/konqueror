/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WEBSHORTCUTWIDGET_H
#define WEBSHORTCUTWIDGET_H

#include <QDialog>

class QLabel;
class QLineEdit;

class WebShortcutWidget : public QDialog
{
    Q_OBJECT
public:
    WebShortcutWidget(QWidget *parent = nullptr);

    void show(const QString &openSearchName, const QString &fileName);

private slots:
    void okClicked();
    void cancelClicked();

signals:
    void webShortcutSet(const QString &openSearchName, const QString &webShortcut, const QString &fileName);

private:
    QLabel *m_searchTitleLabel;
    QLineEdit *m_wsLineEdit;
    QLineEdit *m_nameLineEdit;
    QString m_fileName;
};

#endif // WEBSHORTCUTWIDGET_H


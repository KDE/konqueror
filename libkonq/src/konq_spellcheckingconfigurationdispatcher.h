/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KONQ_SPELLCHECKINGCONFIGURATIONDISPATCHER_H
#define KONQ_SPELLCHECKINGCONFIGURATIONDISPATCHER_H

#include <QObject>

#include <libkonq_export.h>

class LIBKONQ_EXPORT KonqSpellCheckingConfigurationDispatcher : public QObject
{
    Q_OBJECT

public:
    static KonqSpellCheckingConfigurationDispatcher* self();

signals:
    void spellCheckingConfigurationChanged(bool enabled);

};

#endif // KONQ_SPELLCHECKINGCONFIGURATIONDISPATCHER_H

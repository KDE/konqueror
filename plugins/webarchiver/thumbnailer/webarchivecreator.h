/*
    SPDX-FileCopyrightText: 2001 Malte Starostik <malte@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WEBARCHIVECREATOR_H
#define WEBARCHIVECREATOR_H

#include <QObject>

#include <KIO/ThumbnailCreator>


class QTemporaryDir;


class WebArchiveCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT

public:
    WebArchiveCreator(QObject *parent, const QVariantList &va);
    ~WebArchiveCreator() override;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest & request) override;

private slots:
    void slotLoadFinished(bool ok);

    void slotProcessingTimeout();
    void slotRenderTimer();

private:
    QTemporaryDir *m_tempDir;

    bool m_rendered;
    bool m_error;
};



#endif // WEBARCHIVECREATOR_H

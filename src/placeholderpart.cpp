// This file is part of the KDE project
// SPDX-FileCopyrightText: <year> Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "placeholderpart.h"

#include <KPluginMetaData>

#include <QFile>
#include <QLabel>
#include <QJsonObject>

using namespace Konq;

KPluginMetaData Konq::PlaceholderPart::defaultMetadata()
{
    QJsonObject obj;
    obj.insert("KPlugin", QJsonObject{{"Id", "PlaceHolderPart"}});
    return {obj, QString()};
}

Konq::PlaceholderPart::PlaceholderPart(QWidget *parentWidget, QObject* parent) :
#if QT_VERSION_MAJOR < 6
KParts::ReadOnlyPart(parent),
#else
KParts::ReadOnlyPart(parent, defaultMetadata()),
#endif
m_widget(new QWidget(parentWidget))
{
    setWidget(m_widget);
#if QT_VERSION_MAJOR < 6
    setMetaData(defaultMetadata());
#endif
}

Konq::PlaceholderPart::~PlaceholderPart() noexcept
{
}

bool Konq::PlaceholderPart::openUrl(const QUrl& url)
{
    if (!url.isValid()) {
        return false;
    }

    setUrl(url);
    if (url.isLocalFile()) {
        setLocalFilePath(url.path());
    }
    emit setWindowCaption(url.toDisplayString());
    return true;
}

bool Konq::PlaceholderPart::openFile()
{
    return QFile::exists(localFilePath());
}




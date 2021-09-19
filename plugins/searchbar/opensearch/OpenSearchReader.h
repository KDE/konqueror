/*
    SPDX-FileCopyrightText: 2009 Jakub Wieczorek <faw217@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OPENSEARCHREADER_H
#define OPENSEARCHREADER_H

#include <QXmlStreamReader>

class OpenSearchEngine;

class OpenSearchReader : public QXmlStreamReader
{
public:
    OpenSearchReader();

    OpenSearchEngine *read(const QByteArray &data);
    OpenSearchEngine *read(QIODevice *device);

private:
    OpenSearchEngine *read();
};

#endif // OPENSEARCHREADER_H

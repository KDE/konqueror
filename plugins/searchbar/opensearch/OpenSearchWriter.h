/*
    SPDX-FileCopyrightText: 2009 Jakub Wieczorek <faw217@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OPENSEARCHWRITER_H
#define OPENSEARCHWRITER_H

#include <QXmlStreamWriter>

class QIODevice;

class OpenSearchEngine;

class OpenSearchWriter : public QXmlStreamWriter
{
public:
    OpenSearchWriter();

    bool write(QIODevice *device, OpenSearchEngine *engine);

private:
    void write(OpenSearchEngine *engine);
};

#endif


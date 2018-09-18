/*
 * Copyright 2009 Jakub Wieczorek <faw217@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "OpenSearchWriter.h"

#include "OpenSearchEngine.h"

#include <QIODevice>

OpenSearchWriter::OpenSearchWriter()
    : QXmlStreamWriter()
{
    setAutoFormatting(true);
}

bool OpenSearchWriter::write(QIODevice *device, OpenSearchEngine *engine)
{
    if (!engine) {
        return false;
    }

    if (!device->isOpen()) {
        device->open(QIODevice::WriteOnly);
    }

    setDevice(device);
    write(engine);
    return true;
}

void OpenSearchWriter::write(OpenSearchEngine *engine)
{
    writeStartDocument();
    writeStartElement(QStringLiteral("OpenSearchDescription"));
    writeDefaultNamespace(QStringLiteral("http://a9.com/-/spec/opensearch/1.1/"));

    if (!engine->name().isEmpty()) {
        writeTextElement(QStringLiteral("ShortName"), engine->name());
    }

    if (!engine->description().isEmpty()) {
        writeTextElement(QStringLiteral("Description"), engine->description());
    }

    if (!engine->searchUrlTemplate().isEmpty()) {
        writeStartElement(QStringLiteral("Url"));
        writeAttribute(QStringLiteral("method"), QStringLiteral("get"));
        writeAttribute(QStringLiteral("template"), engine->searchUrlTemplate());

        if (!engine->searchParameters().empty()) {
            writeNamespace(QStringLiteral("http://a9.com/-/spec/opensearch/extensions/parameters/1.0/"), QStringLiteral("p"));

            QList<OpenSearchEngine::Parameter>::const_iterator end = engine->searchParameters().constEnd();
            QList<OpenSearchEngine::Parameter>::const_iterator i = engine->searchParameters().constBegin();
            for (; i != end; ++i) {
                writeStartElement(QStringLiteral("p:Parameter"));
                writeAttribute(QStringLiteral("name"), i->first);
                writeAttribute(QStringLiteral("value"), i->second);
                writeEndElement();
            }
        }

        writeEndElement();
    }

    if (!engine->suggestionsUrlTemplate().isEmpty()) {
        writeStartElement(QStringLiteral("Url"));
        writeAttribute(QStringLiteral("method"), QStringLiteral("get"));
        writeAttribute(QStringLiteral("type"), QStringLiteral("application/x-suggestions+json"));
        writeAttribute(QStringLiteral("template"), engine->suggestionsUrlTemplate());

        if (!engine->suggestionsParameters().empty()) {
            writeNamespace(QStringLiteral("http://a9.com/-/spec/opensearch/extensions/parameters/1.0/"), QStringLiteral("p"));

            QList<OpenSearchEngine::Parameter>::const_iterator end = engine->suggestionsParameters().constEnd();
            QList<OpenSearchEngine::Parameter>::const_iterator i = engine->suggestionsParameters().constBegin();
            for (; i != end; ++i) {
                writeStartElement(QStringLiteral("p:Parameter"));
                writeAttribute(QStringLiteral("name"), i->first);
                writeAttribute(QStringLiteral("value"), i->second);
                writeEndElement();
            }
        }

        writeEndElement();
    }

    if (!engine->imageUrl().isEmpty()) {
        writeTextElement(QStringLiteral("Image"), engine->imageUrl());
    }

    writeEndElement();
    writeEndDocument();
}


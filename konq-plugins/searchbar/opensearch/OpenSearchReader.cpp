/*
 * Copyright 2009 Jakub Wieczorek <faw217@gmail.com>
 * Copyright 2009 Fredy Yanardi <fyanardi@gmail.com>
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

#include "OpenSearchReader.h"

#include "OpenSearchEngine.h"

#include <QtCore/QIODevice>

#include <KDE/KLocalizedString>

OpenSearchReader::OpenSearchReader()
    : QXmlStreamReader()
{
}

OpenSearchEngine *OpenSearchReader::read(const QByteArray &data)
{
    clear();

    addData(data);

    return read();
}

OpenSearchEngine *OpenSearchReader::read(QIODevice *device)
{
    clear();

    if (!device->isOpen()) {
        device->open(QIODevice::ReadOnly);
    }

    setDevice(device);
    return read();
}

OpenSearchEngine *OpenSearchReader::read()
{
    OpenSearchEngine *engine = new OpenSearchEngine();

    while (!isStartElement() && !atEnd()) {
        readNext();
    }

    if (name() != QLatin1String("OpenSearchDescription")
        || namespaceUri() != QLatin1String("http://a9.com/-/spec/opensearch/1.1/")) {
        raiseError(i18n("The file is not an OpenSearch 1.1 file."));
        return engine;
    }

    while (!(isEndElement() && name() == QLatin1String("OpenSearchDescription")) && !atEnd()) {
        readNext();

        if (!isStartElement()) {
            continue;
        }

        if (name() == QLatin1String("ShortName")) {
            engine->setName(readElementText());
        }
        else if (name() == QLatin1String("Description")) {
            engine->setDescription(readElementText());
        }
        else if (name() == QLatin1String("Url")) {
            QString type = attributes().value(QLatin1String("type")).toString();
            QString url = attributes().value(QLatin1String("template")).toString();

            if (url.isEmpty())
                continue;

            QList<OpenSearchEngine::Parameter> parameters;

            readNext();

            while (!(isEndElement() && name() == QLatin1String("Url"))) {
                if (!isStartElement() || (name() != QLatin1String("Param") && name() != QLatin1String("Parameter"))) {
                    readNext();
                    continue;
                }

                QString key = attributes().value(QLatin1String("name")).toString();
                QString value = attributes().value(QLatin1String("value")).toString();

                if (!key.isEmpty() && !value.isEmpty()) {
                    parameters.append(OpenSearchEngine::Parameter(key, value));
                }

                while (!isEndElement()) {
                    readNext();
                }
            }

            if (type == QLatin1String("application/x-suggestions+json")) {
                engine->setSuggestionsUrlTemplate(url);
                engine->setSuggestionsParameters(parameters);
            }
            else {
                engine->setSearchUrlTemplate(url);
                engine->setSearchParameters(parameters);
            }
        }
        else if (name() == QLatin1String("Image")) {
             engine->setImageUrl(readElementText());
        }

        if (!engine->name().isEmpty()
            && !engine->description().isEmpty()
            && !engine->suggestionsUrlTemplate().isEmpty()
            && !engine->searchUrlTemplate().isEmpty()
            && !engine->imageUrl().isEmpty()) {
            break;
        }
    }

    return engine;
}


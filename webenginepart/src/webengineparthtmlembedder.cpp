/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "webengineparthtmlembedder.h"

#include <KIO/StoredTransferJob>

#include <QWebEngineUrlRequestJob>
#include <QBuffer>
#include <QFileInfo>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QMetaObject>

static const char s_extractUrlsJs[] = "extractUrlsForTag = function(name, attr){\n"
    "  var elems = document.getElementsByTagName(name);\n"
    "  var urls = [];\n"
    "  for(var i = 0; i < elems.length; i++){\n"
    "    var url = elems[i].getAttribute(attr);\n"
    "    if(url.length > 0) urls.push(url);\n"
    "  }\n"
    "  return urls;\n"
    "};\n"
    "extractUrlsForTag(\"link\", \"href\").concat(extractUrlsForTag(\"img\", \"src\"));";
    
static const char s_replaceUrlsJs[] = "urlMap = %1;\n"
    "replaceUrlsForTag = function(name, attr){\n"
    "  var elems = document.getElementsByTagName(name);\n"
    "  var urls = [];\n"
    "  for(var i = 0; i < elems.length; i++){\n"
    "    var url = elems[i].getAttribute(attr);\n"
    "    var repl = urlMap[url];\n"
    "    if(repl) elems[i].setAttribute(attr, repl);\n"
    "  }\n"
    "}\n"
    "replaceUrlsForTag(\"link\", \"href\");\n"
    "replaceUrlsForTag(\"map\", \"src\");";

WebEnginePartHtmlEmbedder::WebEnginePartHtmlEmbedder(QObject* parent) : 
        QObject(parent), 
        m_profile(new QWebEngineProfile(this)),
        m_page(new QWebEnginePage(m_profile, this))
{
    connect(this, &WebEnginePartHtmlEmbedder::urlsExtracted, this, &WebEnginePartHtmlEmbedder::startReplacingUrls);
    connect(this, &WebEnginePartHtmlEmbedder::urlsReplaced, this, &WebEnginePartHtmlEmbedder::startRetrievingHtml);
}

void WebEnginePartHtmlEmbedder::startEmbedding(const QByteArray& html, const QString& mimeType)
{
    //Try avoiding problems with redirection (see documentation for this class)
    connect(m_page, &QWebEnginePage::loadFinished, this, &WebEnginePartHtmlEmbedder::startExtractingUrls);
    m_page->setContent(html, mimeType, QUrl::fromLocalFile("/"));
}

void WebEnginePartHtmlEmbedder::startExtractingUrls()
{
    //Try avoiding problems with redirection (see documentation for this class) 
    disconnect(m_page, &QWebEnginePage::loadFinished, this, &WebEnginePartHtmlEmbedder::startExtractingUrls);
    auto lambda = [this](const QVariant &res){emit urlsExtracted(res.toStringList());};
    m_page->runJavaScript(s_extractUrlsJs, lambda);
}

void WebEnginePartHtmlEmbedder::startReplacingUrls(const QStringList& urls)
{
    QStringList uniqueUrls(urls);
    uniqueUrls.removeDuplicates();
    QHash<QString, QVariant> map;
    foreach(const QString &url, uniqueUrls){
        QUrl u(url);
        QString data = dataUrl(u);
        if (!data.isEmpty()) {
            map[url] = QVariant(data);
        }
    }
    QJsonDocument doc = QJsonDocument::fromVariant(QVariant::fromValue(map));
    QString js = QString(s_replaceUrlsJs).arg(QString(doc.toJson()));
    m_page->runJavaScript(js, [this](const QVariant &){emit urlsReplaced();});
}

void WebEnginePartHtmlEmbedder::startRetrievingHtml()
{
    auto callback = [this](const QString &html){emit finished(html);};
    m_page->toHtml(callback);
}

QString WebEnginePartHtmlEmbedder::dataUrl(const QUrl& url) const
{
    if (url.scheme() != "file") {
        return QString();
    }
    QString path = url.toLocalFile();
    if (QFileInfo(path).isRelative()) {
        return QString();
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QByteArray content = file.readAll().toBase64();
    return "data:" + QMimeDatabase().mimeTypeForFile(path).name()+";charset=UTF-8;base64," + content;
}

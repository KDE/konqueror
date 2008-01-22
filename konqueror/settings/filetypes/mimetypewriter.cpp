/*  This file is part of the KDE project
    Copyright (C) 2007, 2008 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "mimetypewriter.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <kstandarddirs.h>

#include <QXmlStreamWriter>
#include <QFile>

class MimeTypeWriterPrivate
{
public:
    QString localFilePath() const;

    QString m_mimeType;
    QString m_comment;
    QStringList m_patterns;
    QString m_marker;
};

MimeTypeWriter::MimeTypeWriter(const QString& mimeType)
    : d(new MimeTypeWriterPrivate)
{
    d->m_mimeType = mimeType;
    Q_ASSERT(!mimeType.isEmpty());
}

MimeTypeWriter::~MimeTypeWriter()
{
    delete d;
}

void MimeTypeWriter::setComment(const QString& comment)
{
    d->m_comment = comment;
}

void MimeTypeWriter::setPatterns(const QStringList& patterns)
{
    d->m_patterns = patterns;
}

void MimeTypeWriter::setMarker(const QString& marker)
{
    d->m_marker = marker;
}

bool MimeTypeWriter::write()
{
    const QString packageFileName = d->localFilePath();
    kDebug() << "writing" << packageFileName;
    QFile packageFile(packageFileName);
    if (!packageFile.open(QIODevice::WriteOnly)) {
        kError() << "Couldn't open" << packageFileName << "for writing";
        return false;
    }
    QXmlStreamWriter writer(&packageFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    if (!d->m_marker.isEmpty()) {
        writer.writeComment(d->m_marker);
    }
    const QString nsUri = "http://www.freedesktop.org/standards/shared-mime-info";
    writer.writeDefaultNamespace(nsUri);
    writer.writeStartElement("mime-info");
    writer.writeStartElement(nsUri, "mime-type");
    writer.writeAttribute("type", d->m_mimeType);

    writer.writeStartElement(nsUri, "comment");
    writer.writeCharacters(d->m_comment);
    writer.writeEndElement(); // comment

    foreach(const QString& pattern, d->m_patterns) {
        writer.writeStartElement(nsUri, "glob");
        writer.writeAttribute("pattern", pattern);
        writer.writeEndElement(); // glob
    }

    writer.writeEndElement(); // mime-info
    writer.writeEndElement(); // mime-type
    writer.writeEndDocument();
    return true;
}

void MimeTypeWriter::runUpdateMimeDatabase()
{
    const QString localPackageDir = KStandardDirs::locateLocal("xdgdata-mime", QString());
    Q_ASSERT(!localPackageDir.isEmpty());
    KProcess proc;
    proc << "update-mime-database";
    proc << localPackageDir;
    const int exitCode = proc.execute();
    if (exitCode) {
        kWarning() << proc.program() << "exited with error code" << exitCode;
    }
}

QString MimeTypeWriterPrivate::localFilePath() const
{
    // XDG shared mime: we must write into a <kdehome>/share/mime/packages/ file...
    // To simplify our job, let's use one "input" file per mimetype, in the user's dir.
    // (this writes into $HOME/.local/share/mime by default)
    //
    // We could also use Override.xml, says the spec, but then we'd need to merge with other mimetypes,
    // and in ~/.local we don't really expect other packages to be installed anyway...
    // TODO this writes into $HOME/.local/share/mime which makes the unit test mess up the user's configuration...
    // can we avoid that? is $KDEHOME/share/mime available too?
    QString baseName = m_mimeType;
    baseName.replace('/', '-');
    return KStandardDirs::locateLocal( "xdgdata-mime", "packages/" + baseName + ".xml" );
}

#if 0
QString MimeTypeWriter::existingDefinitionFile() const
{
    QString baseName = d->m_mimeType;
    baseName.replace('/', '-');
    return KGlobal::dirs()->findResource( "xdgdata-mime", "packages/" + baseName + ".xml" );
}
#endif

/*
SPDX-FileCopyrightText: 2008 Xavier Vello <xavier.vello@gmail.com>

SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kio_bookmarks.h"

#include <stdio.h>
#include <stdlib.h>

#include <KShell>
#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>
#include <KBookmark>
#include <KBookmarkManager>
#include <KColorScheme>
#include <KColorUtils>

#include <QStandardPaths>
#include <QFontDatabase>

void BookmarksProtocol::echoBookmark( const KBookmark &bm)
{
    QString descriptionAsTitle = bm.description().toHtmlEscaped();
    if (!descriptionAsTitle.isEmpty())
        descriptionAsTitle.prepend(QLatin1String("\" title=\""));
    echo ("<li class=\"link\"><a href=\"" + bm.url().url() + descriptionAsTitle + "\"><img src=\"/icon/" + bm.icon() + "\"/>" + bm.text().toHtmlEscaped() + "</a></li>");
}

void BookmarksProtocol::echoSeparator()
{
    echo ("<hr/>");
}

void BookmarksProtocol::echoFolder( const KBookmarkGroup &folder )
{
    if (sizeOfGroup(folder.toGroup(), true) > 1)
    {
        QString descriptionAsTitle = folder.description();
        if (!descriptionAsTitle.isEmpty())
            descriptionAsTitle.prepend(QLatin1String("\" title=\""));

        if (folder.parentGroup() == tree)
        {
            if (config.readEntry("ShowBackgrounds", true))
                echo("<ul style=\"background-image: url(/background/" + folder.icon() + ")\">");
            else
                echo("<ul>");

            echo ("<li class=\"title" + descriptionAsTitle + "\">" + folder.fullText() + "</li>");
        }
        else
        {
            echo("<ul>");
            echo ("<li class=\"title" + descriptionAsTitle + "\"><img src=\"/icon/" + folder.icon() + "\"/>" + folder.text() + "</li>");
        }
        indent++;

        for (KBookmark bm = folder.first(); !bm.isNull(); bm = folder.next(bm))
        {
            if (bm.isGroup())
                echoFolder(bm.toGroup());
            else if (bm.isSeparator())
                echoSeparator();
            else
                echoBookmark(bm);
        }

        indent--;
        echo("</ul>");
    }
}

void BookmarksProtocol::echoIndex()
{
    parseTree();

    echoHead();

    KBookmark bm = tree.first();

    if(bm.isNull()) {
        echo("<p class=\"message\">" + i18n("There are no bookmarks to display yet.") + "</p>");
    }
    else {
        for (int i = 1; i <= columns; i++)
        {
            int size = 0;
            echo("<div class=\"column\">");
            indent++;

            while(!bm.isNull() && (size + sizeOfGroup(bm.toGroup())*2/3 < (totalsize / columns) || size == 0))
            {
                echoFolder(bm.toGroup());
                size += sizeOfGroup(bm.toGroup());
                bm = tree.next(bm);
            }

            if (i == columns)
            {
                while(!bm.isNull())
                {
                    echoFolder(bm.toGroup());
                    bm = tree.next(bm);
                }
            }
            indent--;
            echo("</div>");
        }
    }
    indent--;
    echo("</body>");
    echo("</html>");
}

void BookmarksProtocol::echoHead(const QString &redirect)
{
    WorkerBase::mimeType("text/html");

    QString css(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kio_bookmarks/kio_bookmarks.css"));
    if (css.isEmpty())
        this->warning(i18n("kio_bookmarks CSS file not found. Output will look ugly.\n"
                           "Check your installation."));

    echo("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>");
    echo("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"");
    echo("\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">");
    echo("<html xmlns=\"http://www.w3.org/1999/xhtml\">");
    echo("<head>");
    indent++;
    echo("<title>" + i18n("My Bookmarks") + "</title>");
    echo("<link rel=\"stylesheet\" type=\"text/css\" href=\"file://" + QString::fromUtf8(css.toUtf8()) + "\" />");
    echoStyle();

    if (!redirect.isEmpty())
        echo("<meta http-equiv=\"Refresh\" content=\"0;url=" + redirect + "\"/>");

    indent--;
    echo("</head>");
    echo("<body>");
    indent++;

    /*echo("<div class=\"toolbar\">");
    indent++;
    echo("<a title=\"" + i18n("Configuration") + "\" href=\"/config\"><img src=\"/icon/preferences-system?size=32\"/></a>");
    echo("<a title=\"" + i18n("Edit bookmarks") + "\" href=\"/editbookmarks\"><img src=\"/icon/bookmarks-organize?size=32\"/></a>");
    echo("<a title=\"" + i18n("Help") + "\" href=\"help:/kioslave/bookmarks.html\"><img src=\"/icon/help-contents?size=32\"/></a>");
    indent--;
    echo("</div>");*/

    if (!redirect.isEmpty())
        echo("</body></html>");

}

void BookmarksProtocol::echoStyle()
{
    KColorScheme window = KColorScheme(QPalette::Active, KColorScheme::Window);
    KColorScheme view = KColorScheme(QPalette::Active, KColorScheme::View);
    KColorScheme selection = KColorScheme(QPalette::Active, KColorScheme::Selection);

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);

    echo("<style type=\"text/css\">");
    indent++;
    echo("li.link:hover, div.toolbar img:hover { background: " +
         htmlColor(KColorUtils::tint(view.background().color(), view.decoration(KColorScheme::HoverColor).color(), 0.4)) + "; }");
    echo("div.column > ul, div.toolbar, p.message { background-color: " + htmlColor(view.background()) + "; " +
         "border: 1px solid " + htmlColor(view.shade(KColorScheme::LightShade)) + "; }");
    echo("div.column > ul:hover, p.message:hover { border: 1px solid " + htmlColor(view.decoration(KColorScheme::HoverColor)) + "; }");
    echo("div.toolbar {border-top: none; border-right: none;}");
    echo("div.column { width : " + QString::number(100/columns) + "%; }");
    echo("body { font-size: " + QString::number(font.pointSize()) + "pt; " +
         "background: " + htmlColor(window.background()) + "; " +
         "color: " + htmlColor(view.foreground()) + "; }");
    indent--;
    echo("</style>");
}

void BookmarksProtocol::echo( const QString &string )
{
    for(int i = 0; i < indent; i++)
    {
        data("  ");
    }
    data(string.toUtf8() + '\n');
}

QString BookmarksProtocol::htmlColor(const QColor &col)
{
    int r, g, b;
    col.getRgb(&r, &g, &b);
    const QString num = QString::asprintf("#%02X%02X%02X", r, g, b);
    return num;
}

QString BookmarksProtocol::htmlColor(const QBrush &brush)
{
    return htmlColor(brush.color());
}

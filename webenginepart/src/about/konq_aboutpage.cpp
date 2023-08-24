#include "konq_aboutpage.h"

#include <QApplication>
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QBuffer>
#include <QWebEngineUrlRequestJob>

#include <kiconloader.h>
#include <KLocalizedString>

#include <webenginepart_debug.h>

Q_GLOBAL_STATIC(KonqAboutPageSingleton, s_staticData)

static QString loadFile(const QString &file)
{
    QString res;
    if (file.isEmpty()) {
        return res;
    }

    QFile f(file);

    if (!f.open(QIODevice::ReadOnly)) {
        return res;
    }

    QTextStream t(&f);

    res = t.readAll();

    // otherwise all embedded objects are referenced as konq/...
    QString basehref = QLatin1String("<BASE HREF=\"file:") +
                       file.left(file.lastIndexOf('/')) +
                       QLatin1String("/\">\n");
    res.replace(QLatin1String("<head>"), "<head>\n\t" + basehref, Qt::CaseInsensitive);
    return res;
}

QString KonqAboutPageSingleton::launch()
{
    if (!m_launch_html.isEmpty()) {
        return m_launch_html;
    }

    QString res = loadFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/launch.html")));
    if (res.isEmpty()) {
        return res;
    }

    KIconLoader *iconloader = KIconLoader::global();
    int iconSize = iconloader->currentSize(KIconLoader::Desktop);
    QString home_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("user-home"), KIconLoader::Desktop)).toString();
    QString remote_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("folder-remote"), KIconLoader::Desktop)).toString();
    QString wastebin_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("user-trash-full"), KIconLoader::Desktop)).toString();
    QString bookmarks_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("bookmarks"), KIconLoader::Desktop)).toString();
    QString home_folder = QUrl::fromLocalFile(QDir::homePath()).toString();
    QString continue_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QApplication::isRightToLeft() ? "go-previous" : "go-next", KIconLoader::Small)).toString();

    res = res.arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage.css"))).toString());
    if (qApp->layoutDirection() == Qt::RightToLeft) {
        res = res.arg(QStringLiteral("@import \"%1\";")).arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage_rtl.css"))).toString());
    } else {
        res = res.arg(QLatin1String(""));
    }

    res = res.arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror"))
          .arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror is a web browser, file manager and universal document viewer."))
          .arg(i18nc("Link that points to the first page of the Konqueror 'about page', Starting Points contains links to Home, Network Folders, Trash, etc.", "Starting Points"))
          .arg(i18n("Introduction"))
          .arg(i18n("Tips"))
          .arg(i18n("Specifications"))
          .arg(home_folder)
          .arg(home_icon_path)
          .arg(iconSize).arg(iconSize)
          .arg(home_folder)
          .arg(i18n("Home Folder"))
          .arg(i18n("Your personal files"))
          .arg(wastebin_icon_path)
          .arg(iconSize).arg(iconSize)
          .arg(i18n("Trash"))
          .arg(i18n("Browse and restore the trash"))
          .arg(remote_icon_path)
          .arg(iconSize).arg(iconSize)
          .arg(i18n("Network Folders"))
          .arg(i18n("Shared files and folders"))
          .arg(bookmarks_icon_path)
          .arg(iconSize).arg(iconSize)
          .arg(i18n("Bookmarks"))
          .arg(i18n("Quick access to your bookmarks"))
          .arg(continue_icon_path)
          .arg(KIconLoader::SizeSmall).arg(KIconLoader::SizeSmall)
          .arg(i18n("Next: An Introduction to Konqueror"))
          ;
    i18n("Search the Web");//i18n for possible future use

    m_launch_html = res;
    qCDebug(WEBENGINEPART_LOG) << " HTML : "<<res;
    return res;
}

QString KonqAboutPageSingleton::intro()
{
    if (!m_intro_html.isEmpty()) {
        return m_intro_html;
    }

    QString res = loadFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/intro.html")));
    if (res.isEmpty()) {
        return res;
    }

    KIconLoader *iconloader = KIconLoader::global();
    QString back_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QApplication::isRightToLeft() ? "go-next" : "go-previous", KIconLoader::Small)).toString();
    QString gohome_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("go-home"), KIconLoader::Small)).toString();
    QString continue_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QApplication::isRightToLeft() ? "go-previous" : "go-next", KIconLoader::Small)).toString();

    res = res.arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage.css"))).toString());
    if (qApp->layoutDirection() == Qt::RightToLeft) {
        res = res.arg(QStringLiteral("@import \"%1\";")).arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage_rtl.css"))).toString());
    } else {
        res = res.arg(QLatin1String(""));
    }

    res = res.arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror"))
          .arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror is a web browser, file manager and universal document viewer."))
          .arg(i18nc("Link that points to the first page of the Konqueror 'about page', Starting Points contains links to Home, Network Folders, Trash, etc.", "Starting Points"))
          .arg(i18n("Introduction"))
          .arg(i18n("Tips"))
          .arg(i18n("Specifications"))
          .arg(i18n("Konqueror makes working with and managing your files easy. You can browse "
                    "both local and networked folders while enjoying advanced features "
                    "such as the powerful sidebar and file previews."
                   ))
          .arg(i18n("Konqueror is also a full featured and easy to use web browser which you "
                    "can use to explore the Internet. "
                    "Enter the address (e.g. <a href=\"http://www.kde.org\">http://www.kde.org</a>) "
                    "of a web page you would like to visit in the location bar and press Enter, "
                    "or choose an entry from the Bookmarks menu."))
          .arg(i18n("To return to the previous "
                    "location, press the back button <img width='16' height='16' src=\"%1\"></img> "
                    "in the toolbar. ",  back_icon_path))
          .arg(i18n("To quickly go to your Home folder press the "
                    "home button <img width='16' height='16' src=\"%1\"></img>.", gohome_icon_path))
          .arg(i18n("For more detailed documentation on Konqueror click <a href=\"%1\">here</a>.",
                    QStringLiteral("exec:/khelpcenter help:/konqueror")))
          .arg(QStringLiteral("<img width='16' height='16' src=\"%1\">")).arg(continue_icon_path)
          .arg(i18n("Next: Tips & Tricks"))
          ;

    m_intro_html = res;
    return res;
}

QString KonqAboutPageSingleton::specs()
{
    if (!m_specs_html.isEmpty()) {
        return m_specs_html;
    }

    KIconLoader *iconloader = KIconLoader::global();
    QString res = loadFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/specs.html")));
    QString continue_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QApplication::isRightToLeft() ? "go-previous" : "go-next", KIconLoader::Small)).toString();
    if (res.isEmpty()) {
        return res;
    }

    res = res.arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage.css"))).toString());
    if (qApp->layoutDirection() == Qt::RightToLeft) {
        res = res.arg(QStringLiteral("@import \"%1\";")).arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage_rtl.css"))).toString());
    } else {
        res = res.arg(QLatin1String(""));
    }

    res = res.arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror"))
          .arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror is a web browser, file manager and universal document viewer."))
          .arg(i18nc("Link that points to the first page of the Konqueror 'about page', Starting Points contains links to Home, Network Folders, Trash, etc.", "Starting Points"))
          .arg(i18n("Introduction"))
          .arg(i18n("Tips"))
          .arg(i18n("Specifications"))
          .arg(i18n("Specifications"))
          .arg(i18n("Konqueror is designed to embrace and support Internet standards. "
                    "The aim is to fully implement the officially sanctioned standards "
                    "from organizations such as the W3 and OASIS, while also adding "
                    "extra support for other common usability features that arise as "
                    "de facto standards across the Internet. Along with this support, "
                    "for such functions as favicons, Web Shortcuts, and <A HREF=\"%1\">XBEL bookmarks</A>, "
                    "Konqueror also implements:", QStringLiteral("http://xbel.sourceforge.net/")))
          .arg(i18n("Web Browsing"))
          .arg(i18n("Supported standards"))
          .arg(i18n("Additional requirements*"))
          .arg(i18n("<A HREF=\"%1\">DOM</A> (Level 1, partially Level 2) based "
                    "<A HREF=\"%2\">HTML 4.01</A>", QStringLiteral("http://www.w3.org/DOM"), QStringLiteral("http://www.w3.org/TR/html4/")))
          .arg(i18n("built-in"))
          .arg(i18n("<A HREF=\"%1\">Cascading Style Sheets</A> (CSS 1, partially CSS 2)", QStringLiteral("http://www.w3.org/Style/CSS/")))
          .arg(i18n("built-in"))
          .arg(i18n("<A HREF=\"%1\">ECMA-262</A> Edition 3 (roughly equals JavaScript 1.5)",
                    QStringLiteral("http://www.ecma-international.org/publications/standards/ECMA-262.HTM")))
          .arg(i18n("JavaScript disabled (globally). Enable JavaScript <A HREF=\"%1\">here</A>.", QStringLiteral("exec:/kcmshell5 khtml_java_js")))
          .arg(i18n("JavaScript enabled (globally). Configure JavaScript <A HREF=\\\"%1\\\">here</A>.", QStringLiteral("exec:/kcmshell5 khtml_java_js")))   // leave the double backslashes here, they are necessary for javascript !
          .arg(i18n("Secure <A HREF=\"%1\">Java</A><SUP>&reg;</SUP> support", QStringLiteral("http://www.oracle.com/technetwork/java/index.html")))
          .arg(i18n("JDK 1.2.0 (Java 2) compatible VM (<A HREF=\"%1\">OpenJDK</A> or <A HREF=\"%2\">Sun/Oracle</A>)",
                    QStringLiteral("http://openjdk.java.net/"), QStringLiteral("http://www.oracle.com/technetwork/java/index.html")))
          .arg(i18n("Enable Java (globally) <A HREF=\"%1\">here</A>.", QStringLiteral("exec:/kcmshell5 khtml_java_js")))   // TODO Maybe test if Java is enabled ?
          .arg(i18n("NPAPI <A HREF=\"%2\">plugins</A> (for viewing <A HREF=\"%1\">Flash<SUP>&reg;</SUP></A>, etc.)",
                    QStringLiteral("https://get.adobe.com/flashplayer/"),
                    QStringLiteral("konq:plugins")))
          .arg(i18n("built-in"))
          .arg(i18n("Secure Sockets Layer"))
          .arg(i18n("(TLS/SSL v2/3) for secure communications up to 168bit"))
          .arg(i18n("OpenSSL"))
          .arg(i18n("Bidirectional 16bit unicode support"))
          .arg(i18n("built-in"))
          .arg(i18n("AutoCompletion for forms"))
          .arg(i18n("built-in"))
          .arg(i18nc("Title of an html 'group box' explaining konqueror features", "General"))
          .arg(i18n("Feature"))
          .arg(i18n("Details"))
          .arg(i18n("Image formats"))
          .arg(i18n("PNG<br />JPG<br />GIF"))
          .arg(i18n("Transfer protocols"))
          .arg(i18n("HTTP 1.1 (including gzip/bzip2 compression)"))
          .arg(i18n("FTP"))
          .arg(i18n("and <A HREF=\"%1\">many more (see Kioslaves in KHelpCenter)...</A>", QStringLiteral("exec:/khelpcenter")))
          .arg(i18nc("A feature of Konqueror", "URL-Completion"))
          .arg(i18n("Manual"))
          .arg(i18n("Popup"))
          .arg(i18n("(Short-) Automatic"))
          .arg(QStringLiteral("<img width='16' height='16' src=\"%1\">")).arg(continue_icon_path)
          .arg(i18nc("Link that points to the first page of the Konqueror 'about page', Starting Points contains links to Home, Network Folders, Trash, etc.", "<a href=\"%1\">Return to Starting Points</a>", QStringLiteral("konq:konqueror")))

          ;

    m_specs_html = res;
    return res;
}

QString KonqAboutPageSingleton::tips()
{
    if (!m_tips_html.isEmpty()) {
        return m_tips_html;
    }

    QString res = loadFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/tips.html")));
    if (res.isEmpty()) {
        return res;
    }

    KIconLoader *iconloader = KIconLoader::global();
    QString viewmag_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("format-font-size-more"), KIconLoader::Small)).toString();
    QString history_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("view-history"), KIconLoader::Small)).toString();
    QString openterm_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("utilities-terminal"), KIconLoader::Small)).toString();
    QString locationbar_erase_rtl_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("edit-clear-locationbar-ltr"), KIconLoader::Small)).toString();
    QString locationbar_erase_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("edit-clear-locationbar-rtl"), KIconLoader::Small)).toString();
    QString window_fullscreen_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("view-fullscreen"), KIconLoader::Small)).toString();
    QString view_left_right_icon_path =
        QUrl::fromLocalFile(iconloader->iconPath(QStringLiteral("view-split-left-right"), KIconLoader::Small)).toString();
    QString continue_icon_path = QUrl::fromLocalFile(iconloader->iconPath(QApplication::isRightToLeft() ? "go-previous" : "go-next", KIconLoader::Small)).toString();

    res = res.arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage.css"))).toString());
    if (qApp->layoutDirection() == Qt::RightToLeft) {
        res = res.arg(QStringLiteral("@import \"%1\";")).arg(QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konqueror/about/kde_infopage_rtl.css"))).toString());
    } else {
        res = res.arg(QLatin1String(""));
    }

    res = res.arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror"))
          .arg(i18nc("KDE 4 tag line, see http://kde.org/img/kde40.png", "Be free."))
          .arg(i18n("Konqueror is a web browser, file manager and universal document viewer."))
          .arg(i18nc("Link that points to the first page of the Konqueror 'about page', Starting Points contains links to Home, Network Folders, Trash, etc.", "Starting Points"))
          .arg(i18n("Introduction"))
          .arg(i18n("Tips"))
          .arg(i18n("Specifications"))
          .arg(i18n("Tips & Tricks"))
          .arg(i18n("Use Web-Shortcuts: by typing \"gg: KDE\" one can search the Internet, "
                    "using Google, for the search phrase \"KDE\". There are a lot of "
                    "Web-Shortcuts predefined to make searching for software or looking "
                    "up certain words in an encyclopedia a breeze. You can even "
                    "<a href=\"%1\">create your own</a> Web-Shortcuts.", QStringLiteral("exec:/kcmshell5 webshortcuts")))
          .arg(i18n("Use the magnifier button <img width='16' height='16' src=\"%1\"></img> in the HTML"
                    " toolbar to increase the font size on your web page.", viewmag_icon_path))
          .arg(i18n("When you want to paste a new address into the Location toolbar you might want to "
                    "clear the current entry by pressing the black arrow with the white cross "
                    "<img width='16' height='16' src=\"%1\"></img> in the toolbar.",
                    QApplication::isRightToLeft() ? locationbar_erase_rtl_icon_path : locationbar_erase_icon_path))
          .arg(i18n("To create a link on your desktop pointing to the current page, "
                    "simply drag the icon (favicon) that is to the left of the Location toolbar, drop it on to "
                    "the desktop, and choose \"Icon\"."))
          .arg(i18n("You can also find <img width='16' height='16' src=\"%1\" /> \"Full-Screen Mode\" "
                    "in the Settings menu. This feature is very useful for \"Talk\" "
                    "sessions.", window_fullscreen_icon_path))
          .arg(i18n("Divide et impera (lat. \"Divide and conquer\") - by splitting a window "
                    "into two parts (e.g. Window -> <img width='16' height='16' src=\"%1\" /> Split View "
                    "Left/Right) you can make Konqueror appear the way you like.", view_left_right_icon_path))
          .arg(i18n("Use the <a href=\"%1\">user-agent</a> feature if the website you are visiting "
                    "asks you to use a different browser "
                    "(and do not forget to send a complaint to the webmaster!)", QStringLiteral("exec:/kcmshell5 useragent")))
          .arg(i18n("The <img width='16' height='16' src=\"%1\"></img> History in your Sidebar ensures "
                    "that you can keep track of the pages you have visited recently.", history_icon_path))
          .arg(i18n("Use a caching <a href=\"%1\">proxy</a> to speed up your"
                    " Internet connection.", QStringLiteral("exec:/kcmshell5 proxy")))
          .arg(i18n("Advanced users will appreciate the Konsole which you can embed into "
                    "Konqueror (Settings -> <img width='16' height='16' SRC=\"%1\"></img> Show "
                    "Terminal Emulator).", openterm_icon_path))
          .arg(QStringLiteral("<img width='16' height='16' src=\"%1\">")).arg(continue_icon_path)
          .arg(i18n("Next: Specifications"))
          ;

    m_tips_html = res;
    return res;
}

QString KonqAboutPageSingleton::plugins()
{
    if (!m_plugins_html.isEmpty()) {
        return m_plugins_html;
    }

    QString res = loadFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, qApp->layoutDirection() == Qt::RightToLeft ? "konqueror/about/plugins_rtl.html" : "konqueror/about/plugins.html"))
                  .arg(i18n("Installed Plugins"))
                  .arg(i18n("<td>Plugin</td><td>Description</td><td>File</td><td>Types</td>"))
                  .arg(i18n("Installed"))
                  .arg(i18n("<td>Mime Type</td><td>Description</td><td>Suffixes</td><td>Plugin</td>"));
    if (res.isEmpty()) {
        return res;
    }

    m_plugins_html = res;
    return res;
}

KonqUrlSchemeHandler::KonqUrlSchemeHandler(QObject *parent) : QWebEngineUrlSchemeHandler(parent)
{
}

KonqUrlSchemeHandler::~KonqUrlSchemeHandler()
{
}

void KonqUrlSchemeHandler::requestStarted(QWebEngineUrlRequestJob *req)
{
    QBuffer* buf = new QBuffer(this);
    buf->open(QBuffer::ReadWrite);
    connect(buf, &QIODevice::aboutToClose, buf, &QObject::deleteLater);
    QString data;
    QString path = req->requestUrl().path();
    if (path.endsWith(QStringLiteral("specs"))) {
        data = s_staticData->specs();
    } else if (path.endsWith(QStringLiteral("blank"))) {
        data = QStringLiteral();
    } else if (path.endsWith(QStringLiteral("intro"))) {
        data = s_staticData->intro();
    } else if (path.endsWith(QStringLiteral("tips"))) {
        data = s_staticData->tips();
    } else if (path.endsWith(QStringLiteral("plugins"))) {
        data = s_staticData->plugins();
    } else {
        data = s_staticData->launch();
    }
    buf->write(data.toUtf8());
    buf->seek(0);
    req->reply("text/html", buf);
}

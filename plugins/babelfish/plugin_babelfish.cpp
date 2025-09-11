/* This file is part of the KDE Project
    Original code: plugin code, connecting to Babelfish and support for selected text
    SPDX-FileCopyrightText: 2001 Kurt Granroth <granroth@kde.org>

    Submenus, KConfig file and support for other translation engines
    SPDX-FileCopyrightText: 2003 Rand 2342 <rand2342@yahoo.com>

    Add webkitkde support
    SPDX-FileCopyrightText: 2008 Montel Laurent <montel@kde.org>

    Ported to KParts::TextExtension
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/
#include "plugin_babelfish.h"

#include <kparts/part.h>
#include <KParts/NavigationExtension>

#include <kwidgetsaddons_version.h>
#include <kactioncollection.h>
#include <kcountryflagemojiiconengine.h>
#include <QMenu>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <KLazyLocalizedString>
#include <kconfig.h>
#include <kpluginfactory.h>
#include <kaboutdata.h>
#include <KIO/Job>

#include <KConfigGroup>

#include <QDebug>

#include "textextension.h"
#include "browserextension.h"

static const KAboutData aboutdata(QStringLiteral("babelfish"), i18n("Translate Web Page"), QStringLiteral("1.0"));
K_PLUGIN_CLASS_WITH_JSON(PluginBabelFish, "plugin_translator.json")

PluginBabelFish::PluginBabelFish(QObject *parent,
                                 const QVariantList &)
    : KonqParts::Plugin(parent),
      m_actionGroup(this)
{
    m_menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("babelfish")), i18n("Translate Web &Page"),
                             actionCollection());
    actionCollection()->addAction(QStringLiteral("translatewebpage"), m_menu);
    m_menu->setPopupMode(QToolButton::InstantPopup);
    connect(m_menu->menu(), &QMenu::aboutToShow, this, &PluginBabelFish::slotAboutToShow);

    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent);
    if (part) {
        connect(part, &KParts::ReadOnlyPart::started, this, &PluginBabelFish::slotEnableMenu);
        connect(part, &KParts::ReadOnlyPart::completed, this, &PluginBabelFish::slotEnableMenu);
        connect(part, &KParts::ReadOnlyPart::completedWithPendingAction, this, &PluginBabelFish::slotEnableMenu);
    }
}

static QIcon iconForLanguage(const QString &languageCode)
{
    // The 2-letter codes used internally in this plugin are language codes,
    // but KCountryFlagEmojiIconEngine generates a flag image for a country
    // code. So what is really wanted is "the country that is the generally
    // recognised homeland of that language".  To obtain this a QLocale is
    // constructed with the language code duplicated to specify a language
    // and country.  Without that, for example, QLocale("pt") guesses Brazil
    // for the country and generates the wrong flag, but QLocale("pt_pt")
    // has the intended country.  Some fixes still need to be applied later.
    const QLocale loc(languageCode+"_"+languageCode);
    QString countryCode = QLocale::territoryToCode(loc.territory());

    if (countryCode.isEmpty() && languageCode.startsWith(QStringLiteral("zt"))) {
        // Language code "zt" is only used internally to this plugin.
       countryCode = QStringLiteral("cn");
    }
    else if (loc.language() == QLocale::English && QLocale::system().territory() == QLocale::UnitedKingdom) {
        // British English users get their own flag for that language.
        // The original country code returned by QLocale in this case
        // will have been "us".
        countryCode = QStringLiteral("gb");
    }
    //qDebug() << languageCode << "->" << countryCode;

    if (countryCode.isEmpty()) return (QIcon());

    // Generate the flag icon using the Noto Emoji font.
    // The QIcon constructor takes ownership of the QIconEngine.
    KCountryFlagEmojiIconEngine *engine = new KCountryFlagEmojiIconEngine(countryCode);
    return (!engine->isNull() ? QIcon(engine) : QIcon());
}

/* private */ void PluginBabelFish::addMenuAction(KActionMenu *menu, const QString &srcName, const QString &text, const QString &dstName)
{
    QAction *a = actionCollection()->addAction(srcName);
    a->setText(text);
    a->setIcon(iconForLanguage(!dstName.isEmpty() ? dstName : srcName));
    addMenuAction(menu, a);
    m_actionGroup.addAction(a);
}

/* private */ void PluginBabelFish::addMenuAction(KActionMenu *menu, QAction *action)
{
    // Insert an action into a menu, in an appropriate place so that
    // the menu actions end up in alphabetical order in the UI language.
    const QString actionText = KLocalizedString::removeAcceleratorMarker(action->text());
    QList<QAction *> existingActions = menu->menu()->actions();
    QAction *before = nullptr;

    for (QAction *existingAction : std::as_const(existingActions)) {
        const QString existingText = KLocalizedString::removeAcceleratorMarker(existingAction->text());
        if (existingText.compare(actionText, Qt::CaseInsensitive)>0) {
            before = existingAction;
            break;
        }
    }

    menu->insertAction(before, action);
}

/* private */ KActionMenu *PluginBabelFish::createMenu(const QString &languageCode, const QString &title)
{
    KActionMenu *menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("babelfish")), title, actionCollection());
    menu->setIcon(iconForLanguage(languageCode));
    actionCollection()->addAction(QStringLiteral("translatewebpage_")+languageCode, menu);
    return (menu);
}

void PluginBabelFish::slotAboutToShow()
{
    if (!m_menu->menu()->actions().isEmpty()) { // already populated
        return;
    }

    connect(&m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(translateURL(QAction*)));

    // Create a menu for each "source->dest" translation possibility where
    // there's more than one dest for a given source.

    KActionMenu *menu_en = createMenu(QStringLiteral("en"), i18n("&English To"));
    KActionMenu *menu_fr = createMenu(QStringLiteral("fr"), i18n("&French To"));
    KActionMenu *menu_de = createMenu(QStringLiteral("de"), i18n("&German To"));
    KActionMenu *menu_el = createMenu(QStringLiteral("el"), i18n("&Greek To"));
    KActionMenu *menu_es = createMenu(QStringLiteral("es"), i18n("&Spanish To"));
    KActionMenu *menu_pt = createMenu(QStringLiteral("pt"), i18n("&Portugese To"));
    KActionMenu *menu_it = createMenu(QStringLiteral("it"), i18n("&Italian To"));
    KActionMenu *menu_nl = createMenu(QStringLiteral("nl"), i18n("&Dutch To"));
    KActionMenu *menu_ru = createMenu(QStringLiteral("ru"), i18n("&Russian To"));

    // List of translation possibilities for filling the above menus
    // Mostly from babelfish.
    // TODO: add all translation paths from www.reverso.net
    // and all translation paths from www.freetranslation.com
    // and all those from translate.google.com!! Full matrix supported... we need a different system...
    // [possibly in a different list, so that we can remove it more easily if
    //  it goes offline?]
    // TODO: Maybe the entire list of possibilities could come from translaterc?
    // (It would need source, dest, engine, language-code-on-that-engine)
    // Here we would just have the i18n texts for each lang code,
    // and we would make up the menus on the fly.

    struct {
        KLazyLocalizedString name;
        const char *language;
    } static constexpr const translations[] = {
        { kli18n("&Chinese (Simplified)"), "en_zh" },
        { kli18n("Chinese (&Traditional)"), "en_zt" },
        { kli18n("&Dutch"), "en_nl" },
        { kli18n("&French"), "en_fr" },
        { kli18n("&German"), "en_de" },
        { kli18n("&Greek"), "en_el" },
        { kli18n("&Italian"), "en_it" },
        { kli18n("&Japanese"), "en_ja" },
        { kli18n("&Korean"), "en_ko" },
        { kli18n("&Norwegian"), "en_no" },
        { kli18n("&Portuguese"), "en_pt" },
        { kli18n("&Russian"), "en_ru" },
        { kli18n("&Spanish"), "en_es" },
        { kli18n("T&hai"), "en_th" }, // only on parsit
        { kli18n("&Arabic"), "fr_ar" }, // google
        { kli18n("&Dutch"), "fr_nl" },
        { kli18n("&English"), "fr_en" },
        { kli18n("&German"), "fr_de" },
        { kli18n("&Greek"), "fr_el" },
        { kli18n("&Italian"), "fr_it" },
        { kli18n("&Portuguese"), "fr_pt" },
        { kli18n("&Spanish"), "fr_es" },
        { kli18n("&Russian"), "fr_ru" }, // only on voila
        { kli18n("&English"), "de_en" },
        { kli18n("&French"), "de_fr" },
        { kli18n("&English"), "el_en" },
        { kli18n("&French"), "el_fr" },
        { kli18n("&English"), "es_en" },
        { kli18n("&French"), "es_fr" },
        { kli18n("&English"), "pt_en" },
        { kli18n("&French"), "pt_fr" },
        { kli18n("&English"), "it_en" },
        { kli18n("&French"), "it_fr" },
        { kli18n("&English"), "nl_en" },
        { kli18n("&French"), "nl_fr" },
        { kli18n("&English"), "ru_en" },
        { kli18n("&French"), "ru_fr" }, // only on voila
    };

    for (const auto &entry : translations) {
        const QString translation = QString::fromLatin1(entry.language);
        const int underScorePos = translation.indexOf(QLatin1Char('_'));
        const QString srcLang = translation.left(underScorePos);
        QAction *srcAction = actionCollection()->action(QLatin1String("translatewebpage_") + srcLang);
        if (KActionMenu *actionMenu = qobject_cast<KActionMenu *>(srcAction)) {
            const QString destLang = translation.mid(underScorePos+1);
            addMenuAction(actionMenu, translation, entry.name.toString(), destLang);
        } else {
            qWarning() << "No menu found for" << srcLang;
        }
    }

    // Fill the toplevel menu, both with single source->dest possibilities
    // and with submenus.

    addMenuAction(m_menu, menu_nl);
    addMenuAction(m_menu, menu_en);
    addMenuAction(m_menu, menu_fr);
    addMenuAction(m_menu, menu_de);
    addMenuAction(m_menu, menu_el);
    addMenuAction(m_menu, menu_it);
    addMenuAction(m_menu, menu_pt);
    addMenuAction(m_menu, menu_ru);
    addMenuAction(m_menu, menu_es);

    addMenuAction(m_menu, QStringLiteral("zh_en"), i18n("&Chinese (Simplified) to English"));
    addMenuAction(m_menu, QStringLiteral("zt_en"), i18n("Chinese (&Traditional) to English"));
    addMenuAction(m_menu, QStringLiteral("ja_en"), i18n("&Japanese to English"));
    addMenuAction(m_menu, QStringLiteral("ko_en"), i18n("&Korean to English"));
}

PluginBabelFish::~PluginBabelFish()
{
    delete m_menu;
}

// Decide whether or not to enable the menu
void PluginBabelFish::slotEnableMenu()
{
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());
    TextExtension *textExt = TextExtension::childObject(parent());

    //if (part)
    //    kDebug() << part->url();

    if (part && textExt) {
        const QString scheme = part->url().scheme();	// always lower case
        if ((scheme == QLatin1String("http")) || (scheme == QLatin1String("https"))) {
            if (KParts::NavigationExtension::childObject(part)) {
                m_menu->setEnabled(true);
                return;
            }
        }
    }

    m_menu->setEnabled(false);
}

void PluginBabelFish::translateURL(QAction *action)
{
    // KHTMLPart and KWebKitPart provide a TextExtension, at least.
    // So if we got a part without a TextExtension, give an error.
    TextExtension *textExt = TextExtension::childObject(parent());
    Q_ASSERT(textExt); // already checked in slotAboutToShow

    // Select engine
    const KConfig cfg(QStringLiteral("translaterc"));
    const KConfigGroup grp(&cfg, QString());
    const QString language = action->objectName();
    const QString engine = grp.readEntry(language, QStringLiteral("google"));

    KParts::NavigationExtension *ext = KParts::NavigationExtension::childObject(parent());
    if (!ext) {
        return;
    }
    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent());

    // we check if we have text selected.  if so, we translate that. If
    // not, we translate the url
    QString textForQuery;
    QUrl url = part->url();
    const bool hasSelection = textExt->hasSelection();

    if (hasSelection) {
        QString selection = textExt->selectedText(TextExtension::PlainText);
        textForQuery = QString::fromLatin1(QUrl::toPercentEncoding(selection));
    } else {
        // Check syntax
        if (!url.isValid()) {
            QString title = i18nc("@title:window", "Malformed URL");
            QString text = i18n("The URL you entered is not valid, please "
                                "correct it and try again.");
            KMessageBox::error(part->widget(), text, title);
            return;
        }
    }
    const QString urlForQuery = QLatin1String(QUrl::toPercentEncoding(url.url()));

    if (url.scheme() == QLatin1String("https")) {
        if (KMessageBox::warningContinueCancel(part->widget(),
                                               xi18nc("@info", "\
You are viewing this page over a secure connection.<nl/><nl/>\
The URL of the page will sent to the online translation service,<nl/>\
which may fetch the insecure version of the page."),
                                               i18nc("@title:window", "Security Warning"),
                                               KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel(),
                                               QStringLiteral("insecureTranslate")) != KMessageBox::Continue) {
            return;
        }
    }

    // Create URL
    QUrl result;
    QString query;

    if (engine == QLatin1String("freetranslation")) {
        query = QStringLiteral("sequence=core&language=");
        if (language == QStringLiteral("en_es")) {
            query += QLatin1String("English/Spanish");
        } else if (language == QStringLiteral("en_de")) {
            query += QLatin1String("English/German");
        } else if (language == QStringLiteral("en_it")) {
            query += QLatin1String("English/Italian");
        } else if (language == QStringLiteral("en_nl")) {
            query += QLatin1String("English/Dutch");
        } else if (language == QStringLiteral("en_pt")) {
            query += QLatin1String("English/Portuguese");
        } else if (language == QStringLiteral("en_no")) {
            query += QLatin1String("English/Norwegian");
        } else if (language == QStringLiteral("en_zh")) {
            query += QLatin1String("English/SimplifiedChinese");
        } else if (language == QStringLiteral("en_zhTW")) {
            query += QLatin1String("English/TraditionalChinese");
        } else if (language == QStringLiteral("es_en")) {
            query += QLatin1String("Spanish/English");
        } else if (language == QStringLiteral("fr_en")) {
            query += QLatin1String("French/English");
        } else if (language == QStringLiteral("de_en")) {
            query += QLatin1String("German/English");
        } else if (language == QStringLiteral("it_en")) {
            query += QLatin1String("Italian/English");
        } else if (language == QStringLiteral("nl_en")) {
            query += QLatin1String("Dutch/English");
        } else if (language == QStringLiteral("pt_en")) {
            query += QLatin1String("Portuguese/English");
        } else { // Should be en_fr
            query += QLatin1String("English/French");
        }
        if (hasSelection) {
            // ## does not work
            result = QUrl(QStringLiteral("http://ets.freetranslation.com"));
            query += QLatin1String("&mode=html&template=results_en-us.htm&srctext=");
            query += textForQuery;
        } else {
            result = QUrl(QStringLiteral("http://www.freetranslation.com/web.asp"));
            query += QLatin1String("&url=");
            query += urlForQuery;
        }
    } else if (engine == QLatin1String("parsit")) {
        // Does only English -> Thai
        result = QUrl(QStringLiteral("http://c3po.links.nectec.or.th/cgi-bin/Parsitcgi.exe"));
        query = QStringLiteral("mode=test&inputtype=");
        if (hasSelection) {
            query += QLatin1String("text&TxtEng=");
            query += textForQuery;
        } else {
            query += QLatin1String("html&inputurl=");
            query += urlForQuery;
        }
    } else if (engine == QLatin1String("reverso")) {
        result = QUrl(QStringLiteral("http://www.reverso.net/url/frame.asp"));
        query = QStringLiteral("autotranslate=on&templates=0&x=0&y=0&directions=");
        if (language == QStringLiteral("de_fr")) {
            query += QLatin1String("524292");
        } else if (language == QStringLiteral("fr_en")) {
            query += QLatin1String("65544");
        } else if (language == QStringLiteral("fr_de")) {
            query += QLatin1String("262152");
        } else if (language == QStringLiteral("de_en")) {
            query += QLatin1String("65540");
        } else if (language == QStringLiteral("en_de")) {
            query += QLatin1String("262145");
        } else if (language == QStringLiteral("en_es")) {
            query += QLatin1String("2097153");
        } else if (language == QStringLiteral("es_en")) {
            query += QLatin1String("65568");
        } else if (language == QStringLiteral("fr_es")) {
            query += QLatin1String("2097160");
        } else { // "en_fr"
            query += QLatin1String("524289");
        }
        query += QLatin1String("&url=");
        query += urlForQuery;
    } else if (engine == QLatin1String("tsail")) {
        result = QUrl(QStringLiteral("http://www.t-mail.com/cgi-bin/tsail"));
        query = QStringLiteral("sail=full&lp=");
        if (language == QStringLiteral("zhTW_en")) {
            query += QLatin1String("tw-en");
        } else if (language == QStringLiteral("en_zhTW")) {
            query += QLatin1String("en-tw");
        } else {
            query += language;
            query[15] = '-';
        }
        query += urlForQuery;
    } else if (engine == QLatin1String("voila")) {
        result = QUrl(QStringLiteral("http://tr.voila.fr/traduire-une-page-web-frame.php"));
        const QStringList parts = language.split('_');
        if (parts.count() == 2) {
            // The translation direction is "first letter of source, first letter of dest"
            query = QStringLiteral("translationDirection=");
            query += parts[0][0];
            query += parts[1][0];
            if (false /*hasSelection*/) {   // TODO: needs POST
                query += QLatin1String("&isText=1&stext=");
                query += textForQuery;
            } else {
                query += QLatin1String("&urlToTranslate=");
                query += urlForQuery;
            }
        }
    } else {
        const QStringList parts = language.split('_');

        if (hasSelection) { //https://translate.google.com/#en|de|text_to_translate
            query = QStringLiteral("https://translate.google.com/#");
            if (parts.count() == 2) {
                query += parts[0] + "|" + parts[1];
            }
            query += "|" + textForQuery;
            result = QUrl(query);
        } else { //https://translate.google.com/translate?hl=en&sl=en&tl=de&u=http%3A%2F%2Fkde.org%2F%2F
            result = QUrl(QStringLiteral("https://translate.google.com/translate"));
            query = QStringLiteral("ie=UTF-8");
            if (parts.count() == 2) {
                query += "&sl=" + parts[0] + "&tl=" + parts[1];
            }
            query += "&u=" + urlForQuery;
            result.setQuery(query);
        }
    }

    // Connect to the fish
    // copied from plugins/akregator/konqfeedicon.cpp
    if (auto browserExtension = qobject_cast<BrowserExtension *>(ext)) {
        emit browserExtension->browserOpenUrlRequest(result);
    } else {
        emit ext->openUrlRequest(result);
    }
}

#include <plugin_babelfish.moc>

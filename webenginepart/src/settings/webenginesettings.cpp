/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "webenginesettings.h"

#include "webengine_filter.h"
#include <webenginepart_debug.h>
#include "../../src/htmldefaults.h"
#include "profile.h"

#include <KConfig>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KIO/Job>
#include <KMessageBox>

#include <QWebEngineSettings>
#include <QFontDatabase>
#include <QFileInfo>
#include <QDir>
#include <QStringView>
#include <QRegularExpression>

using namespace KonqWebEnginePart;

QDataStream & operator<<(QDataStream& ds, const WebEngineSettings::WebFormInfo& info)
{
    ds << info.name<< info.framePath << info.fields;
    return ds;
}

QDataStream & operator>>(QDataStream& ds, WebEngineSettings::WebFormInfo& info)
{
    ds >> info.name >> info.framePath >> info.fields;
    return ds;
}

/**
 * @internal
 * Contains all settings which are both available globally and per-domain
 */
struct KPerDomainSettings {
    bool m_bEnableJava : 1;
    bool m_bEnableJavaScript : 1;
    bool m_bEnablePlugins : 1;
    // don't forget to maintain the bitfields as the enums grow
    HtmlSettingsInterface::JSWindowOpenPolicy m_windowOpenPolicy : 2;
    HtmlSettingsInterface::JSWindowStatusPolicy m_windowStatusPolicy : 1;
    HtmlSettingsInterface::JSWindowFocusPolicy m_windowFocusPolicy : 1;
    HtmlSettingsInterface::JSWindowMovePolicy m_windowMovePolicy : 1;
    HtmlSettingsInterface::JSWindowResizePolicy m_windowResizePolicy : 1;

#ifdef DEBUG_SETTINGS
    void dump(const QString &infix = QString()) const {
      qCDebug(WEBENGINEPART_LOG) << "KPerDomainSettings " << infix << " @" << this << ":";
      qCDebug(WEBENGINEPART_LOG) << "  m_bEnableJava: " << m_bEnableJava;
      qCDebug(WEBENGINEPART_LOG) << "  m_bEnableJavaScript: " << m_bEnableJavaScript;
      qCDebug(WEBENGINEPART_LOG) << "  m_bEnablePlugins: " << m_bEnablePlugins;
      qCDebug(WEBENGINEPART_LOG) << "  m_windowOpenPolicy: " << m_windowOpenPolicy;
      qCDebug(WEBENGINEPART_LOG) << "  m_windowStatusPolicy: " << m_windowStatusPolicy;
      qCDebug(WEBENGINEPART_LOG) << "  m_windowFocusPolicy: " << m_windowFocusPolicy;
      qCDebug(WEBENGINEPART_LOG) << "  m_windowMovePolicy: " << m_windowMovePolicy;
      qCDebug(WEBENGINEPART_LOG) << "  m_windowResizePolicy: " << m_windowResizePolicy;
    }
#endif
};

typedef QMap<QString,KPerDomainSettings> PolicyMap;

class WebEngineSettingsData
{
public:  
    bool m_bChangeCursor : 1;
    bool m_bOpenMiddleClick : 1;
    bool m_underlineLink : 1;
    bool m_hoverLink : 1;
    bool m_bEnableJavaScriptDebug : 1;
    bool m_bEnableJavaScriptErrorReporting : 1;
    bool enforceCharset : 1;
    bool m_bAutoLoadImages : 1;
    bool m_bUnfinishedImageFrame : 1;
    bool m_formCompletionEnabled : 1;
    bool m_autoDelayedActionsEnabled : 1;
    bool m_jsErrorsEnabled : 1;
    bool m_follow_system_colors : 1;
    bool m_allowTabulation : 1;
    bool m_autoSpellCheck : 1;
    bool m_adFilterEnabled : 1;
    bool m_hideAdsEnabled : 1;
    bool m_jsPopupBlockerPassivePopup : 1;
    bool m_accessKeysEnabled : 1;
    bool m_zoomTextOnly : 1;
    bool m_useCookieJar : 1;
#ifndef MANAGE_COOKIES_INTERNALLY
    bool m_acceptCrossDomainCookies : 1;
#endif
    bool m_bAutoRefreshPage: 1;
    bool m_bEnableFavicon:1;
    bool m_disableInternalPluginHandling:1;
    bool m_offerToSaveWebSitePassword:1;
    bool m_loadPluginsOnDemand:1;
    bool m_enableLocalStorage:1;
    bool m_enableOfflineStorageDb:1;
    bool m_enableOfflineWebAppCache:1;
    bool m_enableWebGL:1;
    bool m_zoomToDPI:1;
    bool m_allowActiveMixedContent:1;
    bool m_allowMixedContentDisplay:1;

    // the virtual global "domain"
    KPerDomainSettings global;

    int m_fontSize;
    int m_minFontSize;
    int m_maxFormCompletionItems;
    WebEngineSettings::KAnimationAdvice m_showAnimations;
    WebEngineSettings::KSmoothScrollingMode m_smoothScrolling;

    QString m_encoding;
    QString m_userSheet;

    QColor m_textColor;
    QColor m_baseColor;
    QColor m_linkColor;
    QColor m_vLinkColor;

    PolicyMap domainPolicy;
    QStringList fonts;
    QStringList defaultFonts;

    KDEPrivate::FilterSet adBlackList;
    KDEPrivate::FilterSet adWhiteList;
    QList< QPair< QString, QChar > > m_fallbackAccessKeysAssignments;

    KSharedConfig::Ptr nonPasswordStorableSites;
    KSharedConfig::Ptr sitesWithCustomForms;
    bool m_internalPdfViewer;
};

class WebEngineSettingsPrivate : public QObject, public WebEngineSettingsData
{
    Q_OBJECT
public:
    void adblockFilterLoadList(const QString& filename)
    {
        /** load list file and process each line */
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream ts(&file);
            QString line = ts.readLine();
            while (!line.isEmpty()) {
                //qCDebug(WEBENGINEPART_LOG) << "Adding filter:" << line;
                /** white list lines start with "@@" */
                if (line.startsWith(QLatin1String("@@")))
                    adWhiteList.addFilter(line);
                else
                    adBlackList.addFilter(line);
                line = ts.readLine();
            }
            file.close();
        }
    }

public Q_SLOTS:
    void adblockFilterResult(KJob *job)
    {
        KIO::StoredTransferJob *tJob = qobject_cast<KIO::StoredTransferJob*>(job);
        Q_ASSERT(tJob);

        if ( job->error() == KJob::NoError )
        {
            const QByteArray byteArray = tJob->data();
            const QString localFileName = tJob->property( "webenginesettings_adBlock_filename" ).toString();

            QFile file(localFileName);
            if ( file.open(QFile::WriteOnly) )
            {
                const bool success = (file.write(byteArray) == byteArray.size());
                if ( success )
                    adblockFilterLoadList(localFileName);
                else
                    qCWarning(WEBENGINEPART_LOG) << "Could not write" << byteArray.size() << "to file" << localFileName;
                file.close();
            }
            else
                qCDebug(WEBENGINEPART_LOG) << "Cannot open file" << localFileName << "for filter list";
        }
        else
            qCDebug(WEBENGINEPART_LOG) << "Downloading" << tJob->url() << "failed with message:" << job->errorText();
    }
};


/** Returns a writeable per-domains settings instance for the given domain
  * or a deep copy of the global settings if not existent.
  */
static KPerDomainSettings &setup_per_domain_policy(WebEngineSettingsPrivate* const d, const QString &domain)
{
  if (domain.isEmpty())
    qCWarning(WEBENGINEPART_LOG) << "setup_per_domain_policy: domain is empty";

  const QString ldomain = domain.toLower();
  PolicyMap::iterator it = d->domainPolicy.find(ldomain);
  if (it == d->domainPolicy.end()) {
    // simply copy global domain settings (they should have been initialized
    // by this time)
    it = d->domainPolicy.insert(ldomain,d->global);
  }
  return *it;
}

template<typename T>
static T readEntry(const KConfigGroup& config, const QString& key, int defaultValue)
{
    return static_cast<T>(config.readEntry(key, defaultValue));
}

void WebEngineSettings::readDomainSettings(const KConfigGroup &config, bool reset,
                                        bool global, KPerDomainSettings &pd_settings)
{
  const QString javaPrefix ((global ? QString() : QStringLiteral("java.")));
  const QString jsPrefix ((global ? QString() : QStringLiteral("javascript.")));
  const QString pluginsPrefix (global ? QString() : QStringLiteral("plugins."));

  // The setting for Java
  QString key = javaPrefix + QLatin1String("EnableJava");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_bEnableJava = config.readEntry( key, false );
  else if ( !global )
    pd_settings.m_bEnableJava = d->global.m_bEnableJava;

  // The setting for Plugins
  key = pluginsPrefix + QLatin1String("EnablePlugins");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_bEnablePlugins = config.readEntry( key, true );
  else if ( !global )
    pd_settings.m_bEnablePlugins = d->global.m_bEnablePlugins;

  // The setting for JavaScript
  key = jsPrefix + QLatin1String("EnableJavaScript");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_bEnableJavaScript = config.readEntry( key, true );
  else if ( !global )
    pd_settings.m_bEnableJavaScript = d->global.m_bEnableJavaScript;

  // window property policies
  key = jsPrefix + QLatin1String("WindowOpenPolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowOpenPolicy = readEntry<HtmlSettingsInterface::JSWindowOpenPolicy>(config, key, HtmlSettingsInterface::JSWindowOpenSmart);
  else if ( !global )
    pd_settings.m_windowOpenPolicy = d->global.m_windowOpenPolicy;

  key = jsPrefix + QLatin1String("WindowMovePolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowMovePolicy = readEntry<HtmlSettingsInterface::JSWindowMovePolicy>(config, key, HtmlSettingsInterface::JSWindowMoveAllow);
  else if ( !global )
    pd_settings.m_windowMovePolicy = d->global.m_windowMovePolicy;

  key = jsPrefix + QLatin1String("WindowResizePolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowResizePolicy = readEntry<HtmlSettingsInterface::JSWindowResizePolicy>(config, key, HtmlSettingsInterface::JSWindowResizeAllow);
  else if ( !global )
    pd_settings.m_windowResizePolicy = d->global.m_windowResizePolicy;

  key = jsPrefix + QLatin1String("WindowStatusPolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowStatusPolicy = readEntry<HtmlSettingsInterface::JSWindowStatusPolicy>(config, key, HtmlSettingsInterface::JSWindowStatusAllow);
  else if ( !global )
    pd_settings.m_windowStatusPolicy = d->global.m_windowStatusPolicy;

  key = jsPrefix + QLatin1String("WindowFocusPolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowFocusPolicy = readEntry<HtmlSettingsInterface::JSWindowFocusPolicy>(config, key, HtmlSettingsInterface::JSWindowFocusAllow);
  else if ( !global )
    pd_settings.m_windowFocusPolicy = d->global.m_windowFocusPolicy;
}


WebEngineSettings::WebEngineSettings()
  :d (new WebEngineSettingsPrivate)
{
  init();
}

WebEngineSettings::~WebEngineSettings()
{
  delete d;
}

bool WebEngineSettings::changeCursor() const
{
  return d->m_bChangeCursor;
}

bool WebEngineSettings::underlineLink() const
{
  return d->m_underlineLink;
}

bool WebEngineSettings::hoverLink() const
{
  return d->m_hoverLink;
}

bool WebEngineSettings::internalPdfViewer() const
{
    return d->m_internalPdfViewer;
}

void WebEngineSettings::init()
{
  initWebEngineSettings();

  KConfig global( QStringLiteral("khtmlrc"), KConfig::NoGlobals );
  init( &global, true );

  KSharedConfig::Ptr local = KSharedConfig::openConfig();
  if ( local ) {
      init( local.data(), false );
  }

  initNSPluginSettings();
  initCookieJarSettings();
}

void WebEngineSettings::init( KConfig * config, bool reset )
{
  KConfigGroup cg( config, "MainView Settings" );
  if (reset || cg.exists() )
  {
    if ( reset || cg.hasKey( "OpenMiddleClick" ) )
        d->m_bOpenMiddleClick = cg.readEntry( "OpenMiddleClick", true );
  }

  KConfigGroup cgAccess(config,"Access Keys" );
  if (reset || cgAccess.exists() ) {
      d->m_accessKeysEnabled = cgAccess.readEntry( "Enabled", true );
  }

  KConfigGroup cgFilter( config, "Filter Settings" );
  if ((reset || cgFilter.exists()) && (d->m_adFilterEnabled = cgFilter.readEntry("Enabled", false)))
  {
      d->m_hideAdsEnabled = cgFilter.readEntry("Shrink", false);

      d->adBlackList.clear();
      d->adWhiteList.clear();

      /** read maximum age for filter list files, minimum is one day */
      int htmlFilterListMaxAgeDays = cgFilter.readEntry(QStringLiteral("HTMLFilterListMaxAgeDays")).toInt();
      if (htmlFilterListMaxAgeDays < 1)
          htmlFilterListMaxAgeDays = 1;

      QMapIterator<QString,QString> it (cgFilter.entryMap());
      while (it.hasNext())
      {
          it.next();
          int id = -1;
          const QString name = it.key();
          const QString url = it.value();

          if (name.startsWith(QLatin1String("Filter")))
          {
              if (url.startsWith(QLatin1String("@@")))
                  d->adWhiteList.addFilter(url);
              else
                  d->adBlackList.addFilter(url);
          }
          else if (name.startsWith(QLatin1String("HTMLFilterListName-")) && (id = QStringView{name}.mid(19).toInt()) > 0)
          {
              /** check if entry is enabled */
              bool filterEnabled = cgFilter.readEntry(QStringLiteral("HTMLFilterListEnabled-").append(QString::number(id))) != QLatin1String("false");

              /** get url for HTMLFilterList */
              QUrl url(cgFilter.readEntry(QStringLiteral("HTMLFilterListURL-").append(QString::number(id))));

              if (filterEnabled && url.isValid()) {
                  /** determine where to cache HTMLFilterList file */
                  QString localFile = cgFilter.readEntry(QStringLiteral("HTMLFilterListLocalFilename-").append(QString::number(id)));
                  QString dirName = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
                  QDir().mkpath(dirName);
                  localFile =  dirName + '/' + localFile;

                  /** determine existence and age of cache file */
                  QFileInfo fileInfo(localFile);

                  /** load cached file if it exists, irrespective of age */
                  if (fileInfo.exists())
                      d->adblockFilterLoadList( localFile );

                  /** if no cache list file exists or if it is too old ... */
                  if (!fileInfo.exists() || fileInfo.lastModified().daysTo(QDateTime::currentDateTime()) > htmlFilterListMaxAgeDays)
                  {
                      /** ... in this case, refetch list asynchronously */
                      // qCDebug(WEBENGINEPART_LOG) << "Fetching filter list from" << url << "to" << localFile;
                      KIO::StoredTransferJob *job = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo );
                      QObject::connect( job, &KJob::result, d, &WebEngineSettingsPrivate::adblockFilterResult);
                      /** for later reference, store name of cache file */
                      job->setProperty("webenginesettings_adBlock_filename", localFile);
                  }
              }
          }
      }
  }

  KConfigGroup cgHtml( config, "HTML Settings" );
  if (reset || cgHtml.exists() )
  {
    // Fonts and colors
    if( reset ) {
        d->defaultFonts = QStringList();
        d->defaultFonts.append( cgHtml.readEntry( "StandardFont", QFontDatabase::systemFont(QFontDatabase::GeneralFont).family() ) );
        d->defaultFonts.append( cgHtml.readEntry( "FixedFont", QFontDatabase::systemFont(QFontDatabase::FixedFont).family() ));
        d->defaultFonts.append( cgHtml.readEntry( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT ) );
        d->defaultFonts.append( cgHtml.readEntry( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT ) );
        d->defaultFonts.append( cgHtml.readEntry( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT ) );
        d->defaultFonts.append( cgHtml.readEntry( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT ) );
        d->defaultFonts.append( QStringLiteral( "0" ) ); // font size adjustment
    }

    if ( reset || cgHtml.hasKey( "MinimumFontSize" ) )
        d->m_minFontSize = cgHtml.readEntry( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE );

    if ( reset || cgHtml.hasKey( "MediumFontSize" ) )
        d->m_fontSize = cgHtml.readEntry( "MediumFontSize", 12 );

    d->fonts = cgHtml.readEntry( "Fonts", QStringList() );

    if ( reset || cgHtml.hasKey( "DefaultEncoding" ) )
        d->m_encoding = cgHtml.readEntry( "DefaultEncoding", "" );

    if ( reset || cgHtml.hasKey( "EnforceDefaultCharset" ) )
        d->enforceCharset = cgHtml.readEntry( "EnforceDefaultCharset", false );

    // Behavior

    if ( reset || cgHtml.hasKey("UnderlineLinks") )
        d->m_underlineLink = cgHtml.readEntry( "UnderlineLinks", true );

    if ( reset || cgHtml.hasKey( "HoverLinks" ) )
    {
        if ( (d->m_hoverLink = cgHtml.readEntry( "HoverLinks", false )))
            d->m_underlineLink = false;
    }

    if ( reset || cgHtml.hasKey( "AllowTabulation" ) )
        d->m_allowTabulation = cgHtml.readEntry( "AllowTabulation", false );

    if ( reset || cgHtml.hasKey( "AutoSpellCheck" ) )
        d->m_autoSpellCheck = cgHtml.readEntry( "AutoSpellCheck", true );

    // Other
    if ( reset || cgHtml.hasKey( "AutoLoadImages" ) )
      d->m_bAutoLoadImages = cgHtml.readEntry( "AutoLoadImages", true );

    if ( reset || cgHtml.hasKey( "AutoDelayedActions" ) )
        d->m_bAutoRefreshPage = cgHtml.readEntry( "AutoDelayedActions", true );

    if ( reset || cgHtml.hasKey( "UnfinishedImageFrame" ) )
      d->m_bUnfinishedImageFrame = cgHtml.readEntry( "UnfinishedImageFrame", true );

    if ( reset || cgHtml.hasKey( "ShowAnimations" ) )
    {
      QString value = cgHtml.readEntry( "ShowAnimations").toLower();
      if (value == QLatin1String("disabled"))
         d->m_showAnimations = KAnimationDisabled;
      else if (value == QLatin1String("looponce"))
         d->m_showAnimations = KAnimationLoopOnce;
      else
         d->m_showAnimations = KAnimationEnabled;
    }

    if ( reset || cgHtml.hasKey( "SmoothScrolling" ) )
    {
      QString value = cgHtml.readEntry( "SmoothScrolling", "whenefficient" ).toLower();
      if (value == QLatin1String("disabled"))
         d->m_smoothScrolling = KSmoothScrollingDisabled;
      else if (value == QLatin1String("whenefficient"))
         d->m_smoothScrolling = KSmoothScrollingWhenEfficient;
      else
         d->m_smoothScrolling = KSmoothScrollingEnabled;
    } 

    if ( reset || cgHtml.hasKey( "ZoomTextOnly" ) ) {
        d->m_zoomTextOnly = cgHtml.readEntry( "ZoomTextOnly", false );
    }

    if ( reset || cgHtml.hasKey( "ZoomToDPI" ) ) {
        d->m_zoomToDPI = cgHtml.readEntry( "ZoomToDPI", false );
    }

    if (cgHtml.readEntry("UserStyleSheetEnabled", false)) {
        if (reset || cgHtml.hasKey("UserStyleSheet"))
            d->m_userSheet = cgHtml.readEntry("UserStyleSheet", QString());
    } else {
        d->m_userSheet.clear();
    }

    if (reset || cgHtml.hasKey("InternalPdfViewer")) {
        d->m_internalPdfViewer = cgHtml.readEntry("InternalPdfViewer", false);
    }

    d->m_formCompletionEnabled = cgHtml.readEntry("FormCompletion", true);
    d->m_maxFormCompletionItems = cgHtml.readEntry("MaxFormCompletionItems", 10);
    d->m_autoDelayedActionsEnabled = cgHtml.readEntry ("AutoDelayedActions", true);
    d->m_jsErrorsEnabled = cgHtml.readEntry("ReportJSErrors", true);
    const QStringList accesskeys = cgHtml.readEntry("FallbackAccessKeysAssignments", QStringList());
    d->m_fallbackAccessKeysAssignments.clear();
    for( QStringList::ConstIterator it = accesskeys.begin(); it != accesskeys.end(); ++it )
        if( (*it).length() > 2 && (*it)[ 1 ] == ':' )
            d->m_fallbackAccessKeysAssignments.append( qMakePair( (*it).mid( 2 ), (*it)[ 0 ] ));

    d->m_bEnableFavicon = cgHtml.readEntry("EnableFavicon", true);
    d->m_offerToSaveWebSitePassword = cgHtml.readEntry("OfferToSaveWebsitePassword", true);
  }

  // Colors
  //In which group ?????
  if ( reset || cg.hasKey( "FollowSystemColors" ) )
      d->m_follow_system_colors = cg.readEntry( "FollowSystemColors", false );

  KConfigGroup cgGeneral( config, "General" );
  if ( reset || cgGeneral.exists( ) )
  {
    if ( reset || cgGeneral.hasKey( "foreground" ) ) {
      QColor def(HTML_DEFAULT_TXT_COLOR);
      d->m_textColor = cgGeneral.readEntry( "foreground", def );
    }

    if ( reset || cgGeneral.hasKey( "linkColor" ) ) {
      QColor def(HTML_DEFAULT_LNK_COLOR);
      d->m_linkColor = cgGeneral.readEntry( "linkColor", def );
    }

    if ( reset || cgGeneral.hasKey( "visitedLinkColor" ) ) {
      QColor def(HTML_DEFAULT_VLNK_COLOR);
      d->m_vLinkColor = cgGeneral.readEntry( "visitedLinkColor", def);
    }

    if ( reset || cgGeneral.hasKey( "background" ) ) {
      QColor def(HTML_DEFAULT_BASE_COLOR);
      d->m_baseColor = cgGeneral.readEntry( "background", def);
    }
  }

  KConfigGroup cgJava( config, "Java/JavaScript Settings" );
  if( reset || cgJava.exists() )
  {
    // The global setting for JavaScript debugging
    // This is currently always enabled by default
    if ( reset || cgJava.hasKey( "EnableJavaScriptDebug" ) )
      d->m_bEnableJavaScriptDebug = cgJava.readEntry( "EnableJavaScriptDebug", false );

    // The global setting for JavaScript error reporting
    if ( reset || cgJava.hasKey( "ReportJavaScriptErrors" ) )
      d->m_bEnableJavaScriptErrorReporting = cgJava.readEntry( "ReportJavaScriptErrors", false );

    // The global setting for popup block passive popup
    if ( reset || cgJava.hasKey( "PopupBlockerPassivePopup" ) )
      d->m_jsPopupBlockerPassivePopup = cgJava.readEntry("PopupBlockerPassivePopup", true );

    // Read options from the global "domain"
    readDomainSettings(cgJava,reset,true,d->global);
#ifdef DEBUG_SETTINGS
    d->global.dump("init global");
#endif

    // The domain-specific settings.

    static const char *const domain_keys[] = {	// always keep order of keys
    	"ECMADomains", "JavaDomains", "PluginDomains"
    };
    bool check_old_ecma_settings = true;
    bool check_old_java_settings = true;
    // merge all domains into one list
    QSet<QString> domainList;
    for (unsigned i = 0; i < sizeof domain_keys/sizeof domain_keys[0]; ++i) {
        if (reset || cgJava.hasKey(domain_keys[i])) {
            if (i == 0) check_old_ecma_settings = false;
            else if (i == 1) check_old_java_settings = false;
            const QStringList dl = cgJava.readEntry( domain_keys[i], QStringList() );
            const QSet<QString>::Iterator notfound = domainList.end();
            QStringList::ConstIterator it = dl.begin();
            const QStringList::ConstIterator itEnd = dl.end();
            for (; it != itEnd; ++it) {
                const QString domain = (*it).toLower();
                QSet<QString>::Iterator pos = domainList.find(domain);
                if (pos == notfound) domainList.insert(domain);
            }/*next it*/
        }
    }/*next i*/

    if (reset)
      d->domainPolicy.clear();

    {
      QSet<QString>::ConstIterator it = domainList.constBegin();
      const QSet<QString>::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        const QString domain = *it;
        KConfigGroup cg( config, domain );
        readDomainSettings(cg,reset,false,d->domainPolicy[domain]);
#ifdef DEBUG_SETTINGS
        d->domainPolicy[domain].dump("init "+domain);
#endif
      }
    }

    bool check_old_java = true;
    if( (reset || cgJava.hasKey("JavaDomainSettings")) && check_old_java_settings)
    {
      check_old_java = false;
      const QStringList domainList = cgJava.readEntry( "JavaDomainSettings", QStringList() );
      QStringList::ConstIterator it = domainList.constBegin();
      const QStringList::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        QString domain;
        HtmlSettingsInterface::JavaScriptAdvice javaAdvice;
        HtmlSettingsInterface::JavaScriptAdvice javaScriptAdvice;
        HtmlSettingsInterface::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        setup_per_domain_policy(d,domain).m_bEnableJava = javaAdvice == HtmlSettingsInterface::JavaScriptAccept;
#ifdef DEBUG_SETTINGS
        setup_per_domain_policy(d,domain).dump("JavaDomainSettings 4 "+domain);
#endif
      }
    }

    bool check_old_ecma = true;
    if( ( reset || cgJava.hasKey( "ECMADomainSettings" ) ) && check_old_ecma_settings )
    {
      check_old_ecma = false;
      const QStringList domainList = cgJava.readEntry( "ECMADomainSettings", QStringList() );
      QStringList::ConstIterator it = domainList.constBegin();
      const QStringList::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        QString domain;
        HtmlSettingsInterface::JavaScriptAdvice javaAdvice;
        HtmlSettingsInterface::JavaScriptAdvice javaScriptAdvice;
        HtmlSettingsInterface::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        setup_per_domain_policy(d,domain).m_bEnableJavaScript = javaScriptAdvice == HtmlSettingsInterface::JavaScriptAccept;
#ifdef DEBUG_SETTINGS
	setup_per_domain_policy(d,domain).dump("ECMADomainSettings 4 "+domain);
#endif
      }
    }

    if( ( reset || cgJava.hasKey( "JavaScriptDomainAdvice" ) )
             && ( check_old_java || check_old_ecma )
	     && ( check_old_ecma_settings || check_old_java_settings ) )
    {
      const QStringList domainList = cgJava.readEntry( "JavaScriptDomainAdvice", QStringList() );
      QStringList::ConstIterator it = domainList.constBegin();
      const QStringList::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        QString domain;
        HtmlSettingsInterface::JavaScriptAdvice javaAdvice;
        HtmlSettingsInterface::JavaScriptAdvice javaScriptAdvice;
        HtmlSettingsInterface::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        if( check_old_java )
          setup_per_domain_policy(d,domain).m_bEnableJava = javaAdvice == HtmlSettingsInterface::JavaScriptAccept;
        if( check_old_ecma )
          setup_per_domain_policy(d,domain).m_bEnableJavaScript = javaScriptAdvice == HtmlSettingsInterface::JavaScriptAccept;
#ifdef DEBUG_SETTINGS
        setup_per_domain_policy(d,domain).dump("JavaScriptDomainAdvice 4 "+domain);
#endif
      }
    }
  }

#if 0
  // DNS Prefect support...
  if ( reset || cgHtml.hasKey( "DNSPrefetch" ) )
  {
    // Enabled, Disabled, OnlyWWWAndSLD
    QString value = cgHtml.readEntry( "DNSPrefetch", "Enabled" ).toLower();

    if (value == "enabled")
        Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    else
        Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
  }

  // Sync with QWebEngineSettings.
  if (!d->m_encoding.isEmpty())
      Profile::defaultProfile()->settings()->setDefaultTextEncoding(d->m_encoding);
  Profile::defaultProfile()->settings()->setUserStyleSheetUrl(QUrl::fromUserInput(userStyleSheet()));
#endif


  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, autoLoadImages());
  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, isJavaScriptEnabled());
 // Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::JavaEnabled, isJavaEnabled());
  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, isPluginsEnabled());

  // By default disable JS window.open when policy is deny or smart.
  const HtmlSettingsInterface::JSWindowOpenPolicy policy = windowOpenPolicy();
  if (policy == HtmlSettingsInterface::JSWindowOpenDeny || policy == HtmlSettingsInterface::JSWindowOpenSmart)
      Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
  else
      Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);

//  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::ZoomTextOnly, zoomTextOnly());
//  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled, isJavaScriptDebugEnabled());
  Profile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::StandardFont, stdFontName());
  Profile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::FixedFont, fixedFontName());
  Profile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::SerifFont, serifFontName());
  Profile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::SansSerifFont, sansSerifFontName());
  Profile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::CursiveFont, cursiveFontName());
  Profile::defaultProfile()->settings()->setFontFamily(QWebEngineSettings::FantasyFont, fantasyFontName());

  // TODO: Create a webengine config module that gets embedded into Konqueror's kcm.
  // Turn on WebGL support
//  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, d->m_enableWebGL);
  // Turn on HTML 5 local and offline storage capabilities...
//  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled, d->m_enableOfflineStorageDb);
//  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled, d->m_enableOfflineWebAppCache);
  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, d->m_enableLocalStorage);

  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, smoothScrolling() != KSmoothScrollingDisabled);

  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PdfViewerEnabled, internalPdfViewer());

  Profile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

  // These numbers should be calculated from real "logical" DPI/72, using a default dpi of 96 for now
  computeFontSizes(96);

}


void WebEngineSettings::computeFontSizes( int logicalDpi )
{
  if (zoomToDPI())
      logicalDpi = 96;

  float toPix = logicalDpi/72.0;

  if (toPix < 96.0/72.0)
      toPix = 96.0/72.0;

  Profile::defaultProfile()->settings()->setFontSize(QWebEngineSettings::MinimumFontSize, qRound(minFontSize() * toPix));
  Profile::defaultProfile()->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, qRound(mediumFontSize() * toPix));
}

bool WebEngineSettings::zoomToDPI() const
{
    return d->m_zoomToDPI;
}

void WebEngineSettings::setZoomToDPI(bool enabled)
{
  d->m_zoomToDPI = enabled;
  // save it
  KConfigGroup cg( KSharedConfig::openConfig(), "HTML Settings");
  cg.writeEntry("ZoomToDPI", enabled);
  cg.sync();
}

/** Local helper for retrieving per-domain settings.
  *
  * In case of doubt, the global domain is returned.
  */
static const KPerDomainSettings& lookup_hostname_policy(const WebEngineSettingsPrivate* const d,
                                                        const QString& hostname)
{
#ifdef DEBUG_SETTINGS
  qCDebug(WEBENGINEPART_LOG) << "lookup_hostname_policy(" << hostname << ")";
#endif
  if (hostname.isEmpty()) {
#ifdef DEBUG_SETTINGS
    d->global.dump("global");
#endif
    return d->global;
  }

  const PolicyMap::const_iterator notfound = d->domainPolicy.constEnd();

  // First check whether there is a perfect match.
  PolicyMap::const_iterator it = d->domainPolicy.find(hostname);
  if( it != notfound ) {
#ifdef DEBUG_SETTINGS
    qCDebug(WEBENGINEPART_LOG) << "perfect match";
    (*it).dump(hostname);
#endif
    // yes, use it (unless dunno)
    return *it;
  }

  // Now, check for partial match.  Chop host from the left until
  // there's no dots left.
  QString host_part = hostname;
  int dot_idx = -1;
  while( (dot_idx = host_part.indexOf(QChar('.'))) >= 0 ) {
    host_part.remove(0,dot_idx);
    it = d->domainPolicy.find(host_part);
    Q_ASSERT(notfound == d->domainPolicy.end());
    if( it != notfound ) {
#ifdef DEBUG_SETTINGS
      qCDebug(WEBENGINEPART_LOG) << "partial match";
      (*it).dump(host_part);
#endif
      return *it;
    }
    // assert(host_part[0] == QChar('.'));
    host_part.remove(0,1); // Chop off the dot.
  }

  // No domain-specific entry: use global domain
#ifdef DEBUG_SETTINGS
  qCDebug(WEBENGINEPART_LOG) << "no match";
  d->global.dump("global");
#endif
  return d->global;
}

bool WebEngineSettings::isOpenMiddleClickEnabled()
{
  return d->m_bOpenMiddleClick;
}

bool WebEngineSettings::accessKeysEnabled() const
{
    return d->m_accessKeysEnabled;
}

bool WebEngineSettings::favIconsEnabled() const
{
    return d->m_bEnableFavicon;
}

bool WebEngineSettings::isAdFilterEnabled() const
{
    return d->m_adFilterEnabled;
}

bool WebEngineSettings::isHideAdsEnabled() const
{
    return d->m_hideAdsEnabled;
}

bool WebEngineSettings::isAdFiltered( const QString &url ) const
{
    if (!d->m_adFilterEnabled)
        return false;

    if (url.startsWith(QLatin1String("data:")))
        return false;

    return d->adBlackList.isUrlMatched(url) && !d->adWhiteList.isUrlMatched(url);
}

QString WebEngineSettings::adFilteredBy( const QString &url, bool *isWhiteListed ) const
{
    QString m = d->adWhiteList.urlMatchedBy(url);

    if (!m.isEmpty()) {
        if (isWhiteListed != nullptr)
            *isWhiteListed = true;
        return m;
    }

    m = d->adBlackList.urlMatchedBy(url);
    if (m.isEmpty())
        return QString();

    if (isWhiteListed != nullptr)
        *isWhiteListed = false;
    return m;
}

void WebEngineSettings::addAdFilter( const QString &url )
{
    KConfigGroup config = KSharedConfig::openConfig( QStringLiteral("khtmlrc"), KConfig::NoGlobals )->group( "Filter Settings" );

    QRegularExpression rx;

    // Try compiling to avoid invalid stuff. Only support the basic syntax here...
    // ### refactor somewhat
    if (url.length()>2 && url[0]=='/' && url[url.length()-1] == '/')
    {
        const QString inside = url.mid(1, url.length()-2);
        rx.setPattern(inside);
    }
    else
    {
        rx.setPattern(QRegularExpression::wildcardToRegularExpression(url));
    }

    if (rx.isValid())
    {
        int last=config.readEntry("Count", 0);
        const QString key = "Filter-" + QString::number(last);
        config.writeEntry(key, url);
        config.writeEntry("Count",last+1);
        config.sync();

        if (url.startsWith(QLatin1String("@@")))
            d->adWhiteList.addFilter(url);
        else
            d->adBlackList.addFilter(url);
    }
    else
    {
        KMessageBox::error(nullptr,
                           rx.errorString(),
                           i18n("Filter error"));
    }
}

bool WebEngineSettings::isJavaEnabled( const QString& hostname ) const
{
  return lookup_hostname_policy(d,hostname.toLower()).m_bEnableJava;
}

bool WebEngineSettings::isJavaScriptEnabled( const QString& hostname ) const
{
  return lookup_hostname_policy(d,hostname.toLower()).m_bEnableJavaScript;
}

bool WebEngineSettings::isJavaScriptDebugEnabled( const QString& /*hostname*/ ) const
{
  // debug setting is global for now, but could change in the future
  return d->m_bEnableJavaScriptDebug;
}

bool WebEngineSettings::isJavaScriptErrorReportingEnabled( const QString& /*hostname*/ ) const
{
  // error reporting setting is global for now, but could change in the future
  return d->m_bEnableJavaScriptErrorReporting;
}

bool WebEngineSettings::isPluginsEnabled( const QString& hostname ) const
{
  return lookup_hostname_policy(d,hostname.toLower()).m_bEnablePlugins;
}

HtmlSettingsInterface::JSWindowOpenPolicy WebEngineSettings::windowOpenPolicy(const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowOpenPolicy;
}

HtmlSettingsInterface::JSWindowMovePolicy WebEngineSettings::windowMovePolicy(const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowMovePolicy;
}

HtmlSettingsInterface::JSWindowResizePolicy WebEngineSettings::windowResizePolicy(const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowResizePolicy;
}

HtmlSettingsInterface::JSWindowStatusPolicy WebEngineSettings::windowStatusPolicy(const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowStatusPolicy;
}

HtmlSettingsInterface::JSWindowFocusPolicy WebEngineSettings::windowFocusPolicy(const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowFocusPolicy;
}

int WebEngineSettings::mediumFontSize() const
{
    return d->m_fontSize;
}

int WebEngineSettings::minFontSize() const
{
  return d->m_minFontSize;
}

QString WebEngineSettings::settingsToCSS() const
{
    // lets start with the link properties
    QString str = QStringLiteral("a:link {\ncolor: ");
    str += d->m_linkColor.name();
    str += ';';
    if(d->m_underlineLink)
        str += QLatin1String("\ntext-decoration: underline;");

    if( d->m_bChangeCursor )
    {
        str += QLatin1String("\ncursor: pointer;");
        str += QLatin1String("\n}\ninput[type=image] { cursor: pointer;");
    }
    str += QLatin1String("\n}\n");
    str += QLatin1String("a:visited {\ncolor: ");
    str += d->m_vLinkColor.name();
    str += ';';
    if(d->m_underlineLink)
        str += QLatin1String("\ntext-decoration: underline;");

    if( d->m_bChangeCursor )
        str += QLatin1String("\ncursor: pointer;");
    str += QLatin1String("\n}\n");

    if(d->m_hoverLink)
        str += QLatin1String("a:link:hover, a:visited:hover { text-decoration: underline; }\n");

    return str;
}

QString WebEngineSettings::lookupFont(int i) const
{
    if (d->fonts.count() > i) {
        return d->fonts.at(i);
    }

    if (d->defaultFonts.count() > i) {
        return d->defaultFonts.at(i);
    }

    return QString();
}

QString WebEngineSettings::stdFontName() const
{
    return lookupFont(0);
}

QString WebEngineSettings::fixedFontName() const
{
    return lookupFont(1);
}

QString WebEngineSettings::serifFontName() const
{
    return lookupFont(2);
}

QString WebEngineSettings::sansSerifFontName() const
{
    return lookupFont(3);
}

QString WebEngineSettings::cursiveFontName() const
{
    return lookupFont(4);
}

QString WebEngineSettings::fantasyFontName() const
{
    return lookupFont(5);
}

void WebEngineSettings::setStdFontName(const QString &n)
{
    while(d->fonts.count() <= 0)
        d->fonts.append(QString());
    d->fonts[0] = n;
}

void WebEngineSettings::setFixedFontName(const QString &n)
{
    while(d->fonts.count() <= 1)
        d->fonts.append(QString());
    d->fonts[1] = n;
}

QString WebEngineSettings::userStyleSheet() const
{
    return d->m_userSheet;
}

bool WebEngineSettings::isFormCompletionEnabled() const
{
  return d->m_formCompletionEnabled;
}

int WebEngineSettings::maxFormCompletionItems() const
{
  return d->m_maxFormCompletionItems;
}

const QString &WebEngineSettings::encoding() const
{
  return d->m_encoding;
}

bool WebEngineSettings::followSystemColors() const
{
    return d->m_follow_system_colors;
}

const QColor& WebEngineSettings::textColor() const
{
  return d->m_textColor;
}

const QColor& WebEngineSettings::baseColor() const
{
  return d->m_baseColor;
}

const QColor& WebEngineSettings::linkColor() const
{
  return d->m_linkColor;
}

const QColor& WebEngineSettings::vLinkColor() const
{
  return d->m_vLinkColor;
}

bool WebEngineSettings::autoPageRefresh() const
{
  return d->m_bAutoRefreshPage;
}

bool WebEngineSettings::autoLoadImages() const
{
  return d->m_bAutoLoadImages;
}

bool WebEngineSettings::unfinishedImageFrame() const
{
  return d->m_bUnfinishedImageFrame;
}

WebEngineSettings::KAnimationAdvice WebEngineSettings::showAnimations() const
{
  return d->m_showAnimations;
}

WebEngineSettings::KSmoothScrollingMode WebEngineSettings::smoothScrolling() const
{
  return d->m_smoothScrolling;
}

bool WebEngineSettings::zoomTextOnly() const
{
  return d->m_zoomTextOnly;
}

bool WebEngineSettings::isAutoDelayedActionsEnabled() const
{
  return d->m_autoDelayedActionsEnabled;
}

bool WebEngineSettings::jsErrorsEnabled() const
{
  return d->m_jsErrorsEnabled;
}

void WebEngineSettings::setJSErrorsEnabled(bool enabled)
{
  d->m_jsErrorsEnabled = enabled;
  // save it
  KConfigGroup cg( KSharedConfig::openConfig(), "HTML Settings");
  cg.writeEntry("ReportJSErrors", enabled);
  cg.sync();
}

bool WebEngineSettings::allowTabulation() const
{
    return d->m_allowTabulation;
}

bool WebEngineSettings::autoSpellCheck() const
{
    return d->m_autoSpellCheck;
}

QList< QPair< QString, QChar > > WebEngineSettings::fallbackAccessKeysAssignments() const
{
    return d->m_fallbackAccessKeysAssignments;
}

void WebEngineSettings::setJSPopupBlockerPassivePopup(bool enabled)
{
    d->m_jsPopupBlockerPassivePopup = enabled;
    // save it
    KConfigGroup cg( KSharedConfig::openConfig(), "Java/JavaScript Settings");
    cg.writeEntry("PopupBlockerPassivePopup", enabled);
    cg.sync();
}

bool WebEngineSettings::jsPopupBlockerPassivePopup() const
{
    return d->m_jsPopupBlockerPassivePopup;
}

#ifndef MANAGE_COOKIES_INTERNALLY
bool WebEngineSettings::isCookieJarEnabled() const
{
    return d->m_useCookieJar;
}

bool WebEngineSettings::acceptCrossDomainCookies() const
{
    return d->m_acceptCrossDomainCookies;
}
#endif

// Password storage...
KConfigGroup WebEngineSettings::nonPasswordStorableSitesCg() const
{
    if (!d->nonPasswordStorableSites) {
        d->nonPasswordStorableSites = KSharedConfig::openConfig(QString(), KConfig::NoGlobals);
    }
    return KConfigGroup(d->nonPasswordStorableSites, "NonPasswordStorableSites");
}

bool WebEngineSettings::isNonPasswordStorableSite(const QString &host) const
{
    KConfigGroup cg = nonPasswordStorableSitesCg();
    const QStringList sites = cg.readEntry("Sites", QStringList());
    return sites.contains(host);
}

void WebEngineSettings::addNonPasswordStorableSite(const QString &host)
{
    KConfigGroup cg = nonPasswordStorableSitesCg();
    QStringList sites = cg.readEntry("Sites", QStringList());
    sites.append(host);
    cg.writeEntry("Sites", sites);
    cg.sync();
}

void WebEngineSettings::removeNonPasswordStorableSite(const QString &host)
{
    KConfigGroup cg = nonPasswordStorableSitesCg();
    QStringList sites = cg.readEntry("Sites", QStringList());
    sites.removeOne(host);
    cg.writeEntry("Sites", sites);
    cg.sync();
}

KConfigGroup WebEngineSettings::pagesWithCustomizedCacheableFieldsCg() const
{
    if (!d->sitesWithCustomForms) {
        d->sitesWithCustomForms = KSharedConfig::openConfig(QString(), KConfig::NoGlobals); 
    }
    return KConfigGroup(d->sitesWithCustomForms, "PagesWithCustomizedCacheableFields");
}

void WebEngineSettings::setCustomizedCacheableFieldsForPage(const QString& url, const WebFormInfoList& forms)
{
    KConfigGroup cg = pagesWithCustomizedCacheableFieldsCg();
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << forms;
    cg.writeEntry(url, data);
    cg.sync();
}

bool WebEngineSettings::hasPageCustomizedCacheableFields(const QString& url) const
{
    KConfigGroup cg = pagesWithCustomizedCacheableFieldsCg();
    return cg.hasKey(url);
}

void WebEngineSettings::removeCacheableFieldsCustomizationForPage(const QString& url)
{
    KConfigGroup cg = pagesWithCustomizedCacheableFieldsCg();
    cg.deleteEntry(url);
    cg.sync();
}

WebEngineSettings::WebFormInfoList WebEngineSettings::customizedCacheableFieldsForPage(const QString& url)
{
    KConfigGroup cg = pagesWithCustomizedCacheableFieldsCg();
    QByteArray data = cg.readEntry(url, QByteArray());
    if (data.isEmpty()) {
        return {};
    }
    QDataStream ds(data);
    WebFormInfoList res;
    ds >> res;
    return res;
}

bool WebEngineSettings::askToSaveSitePassword() const
{
    return d->m_offerToSaveWebSitePassword;
}

bool WebEngineSettings::isInternalPluginHandlingDisabled() const
{
    return d->m_disableInternalPluginHandling;
}

bool WebEngineSettings::isLoadPluginsOnDemandEnabled() const
{
    return d->m_loadPluginsOnDemand;
}

bool WebEngineSettings::allowMixedContentDisplay() const
{
    return d->m_allowMixedContentDisplay;
}

bool WebEngineSettings::alowActiveMixedContent() const
{
    return d->m_allowActiveMixedContent;
}


void WebEngineSettings::initWebEngineSettings()
{
    KConfig cfg (QStringLiteral("webenginepartrc"), KConfig::NoGlobals);
    KConfigGroup generalCfg (&cfg, "General");
    d->m_disableInternalPluginHandling = generalCfg.readEntry("DisableInternalPluginHandling", false);
    d->m_enableLocalStorage = generalCfg.readEntry("EnableLocalStorage", true);
    d->m_enableOfflineStorageDb = generalCfg.readEntry("EnableOfflineStorageDatabase", true);
    d->m_enableOfflineWebAppCache = generalCfg.readEntry("EnableOfflineWebApplicationCache", true);
    d->m_enableWebGL = generalCfg.readEntry("EnableWebGL", true);
    d->m_allowActiveMixedContent = generalCfg.readEntry("AllowActiveMixedContent", false);
    d->m_allowMixedContentDisplay = generalCfg.readEntry("AllowMixedContentDisplay", true);

    // Force the reloading of the non password storable sites settings.
    d->nonPasswordStorableSites.reset();
}

void WebEngineSettings::initCookieJarSettings()
{
    KSharedConfig::Ptr cookieCfgPtr = KSharedConfig::openConfig(QStringLiteral("kcookiejarrc"), KConfig::NoGlobals);
    KConfigGroup cookieCfg ( cookieCfgPtr, "Cookie Policy");
    d->m_useCookieJar = cookieCfg.readEntry("Cookies", false);
#ifndef MANAGE_COOKIES_INTERNALLY
    d->m_acceptCrossDomainCookies = !cookieCfg.readEntry("RejectCrossDomainCookies", true);
#endif
}

void WebEngineSettings::initNSPluginSettings()
{
    KSharedConfig::Ptr cookieCfgPtr = KSharedConfig::openConfig(QStringLiteral("kcmnspluginrc"), KConfig::NoGlobals);
    KConfigGroup cookieCfg ( cookieCfgPtr, "Misc");
    d->m_loadPluginsOnDemand = cookieCfg.readEntry("demandLoad", false);
}


WebEngineSettings* WebEngineSettings::self()
{
    static WebEngineSettings s_webEngineSettings;
    return &s_webEngineSettings;
}

QDebug operator<<(QDebug dbg, const WebEngineSettings::WebFormInfo& info)
{
    QDebugStateSaver state(dbg);
    dbg.nospace() << "CustomWebFormInfo{";
    dbg << info.name << ", " << info.framePath << ", " << info.fields << "}";
    return dbg;
}

#include "webenginesettings.moc"

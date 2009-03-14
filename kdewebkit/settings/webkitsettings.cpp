/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "webkitsettings.h"
#include "webkitdefaults.h"

#include "khtml_filter_p.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <QWebSettings>

#include <QtGui/QFontDatabase>

/**
 * @internal
 * Contains all settings which are both available globally and per-domain
 */
struct KPerDomainSettings {
    bool m_bEnableJava : 1;
    bool m_bEnableJavaScript : 1;
    bool m_bEnablePlugins : 1;
    // don't forget to maintain the bitfields as the enums grow
    WebKitSettings::KJSWindowOpenPolicy m_windowOpenPolicy : 2;
    WebKitSettings::KJSWindowStatusPolicy m_windowStatusPolicy : 1;
    WebKitSettings::KJSWindowFocusPolicy m_windowFocusPolicy : 1;
    WebKitSettings::KJSWindowMovePolicy m_windowMovePolicy : 1;
    WebKitSettings::KJSWindowResizePolicy m_windowResizePolicy : 1;

#ifdef DEBUG_SETTINGS
    void dump(const QString &infix = QString()) const {
      kDebug() << "KPerDomainSettings " << infix << " @" << this << ":";
      kDebug() << "  m_bEnableJava: " << m_bEnableJava;
      kDebug() << "  m_bEnableJavaScript: " << m_bEnableJavaScript;
      kDebug() << "  m_bEnablePlugins: " << m_bEnablePlugins;
      kDebug() << "  m_windowOpenPolicy: " << m_windowOpenPolicy;
      kDebug() << "  m_windowStatusPolicy: " << m_windowStatusPolicy;
      kDebug() << "  m_windowFocusPolicy: " << m_windowFocusPolicy;
      kDebug() << "  m_windowMovePolicy: " << m_windowMovePolicy;
      kDebug() << "  m_windowResizePolicy: " << m_windowResizePolicy;
    }
#endif
};

QString *WebKitSettings::avFamilies = 0;
typedef QMap<QString,KPerDomainSettings> PolicyMap;

class WebKitSettingsPrivate
{
public:
    bool m_bChangeCursor : 1;
    bool m_bOpenMiddleClick : 1;
    bool m_bBackRightClick : 1;
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

    // the virtual global "domain"
    KPerDomainSettings global;

    int m_fontSize;
    int m_minFontSize;
    int m_maxFormCompletionItems;
    WebKitSettings::KAnimationAdvice m_showAnimations;
    WebKitSettings::KSmoothScrollingMode m_smoothScrolling;

    QString m_encoding;
    QString m_userSheet;

    QColor m_textColor;
    QColor m_baseColor;
    QColor m_linkColor;
    QColor m_vLinkColor;

    PolicyMap domainPolicy;
    QStringList fonts;
    QStringList defaultFonts;

    khtml::FilterSet adBlackList;
    khtml::FilterSet adWhiteList;
    QList< QPair< QString, QChar > > m_fallbackAccessKeysAssignments;
};


/** Returns a writeable per-domains settings instance for the given domain
  * or a deep copy of the global settings if not existent.
  */
static KPerDomainSettings &setup_per_domain_policy(
				WebKitSettingsPrivate* const d,
				const QString &domain) {
  if (domain.isEmpty()) {
    kWarning() << "setup_per_domain_policy: domain is empty";
  }
  const QString ldomain = domain.toLower();
  PolicyMap::iterator it = d->domainPolicy.find(ldomain);
  if (it == d->domainPolicy.end()) {
    // simply copy global domain settings (they should have been initialized
    // by this time)
    it = d->domainPolicy.insert(ldomain,d->global);
  }
  return *it;
}


WebKitSettings::KJavaScriptAdvice WebKitSettings::strToAdvice(const QString& _str)
{
  KJavaScriptAdvice ret = KJavaScriptDunno;

  if (_str.isNull())
        ret = KJavaScriptDunno;

  if (_str.toLower() == QLatin1String("accept"))
        ret = KJavaScriptAccept;
  else if (_str.toLower() == QLatin1String("reject"))
        ret = KJavaScriptReject;

  return ret;
}

const char* WebKitSettings::adviceToStr(KJavaScriptAdvice _advice)
{
    switch( _advice ) {
    case KJavaScriptAccept: return I18N_NOOP("Accept");
    case KJavaScriptReject: return I18N_NOOP("Reject");
    default: return 0;
    }
        return 0;
}


void WebKitSettings::splitDomainAdvice(const QString& configStr, QString &domain,
                                      KJavaScriptAdvice &javaAdvice, KJavaScriptAdvice& javaScriptAdvice)
{
    QString tmp(configStr);
    int splitIndex = tmp.indexOf(':');
    if ( splitIndex == -1)
    {
        domain = configStr.toLower();
        javaAdvice = KJavaScriptDunno;
        javaScriptAdvice = KJavaScriptDunno;
    }
    else
    {
        domain = tmp.left(splitIndex).toLower();
        QString adviceString = tmp.mid( splitIndex+1, tmp.length() );
        int splitIndex2 = adviceString.indexOf( ':' );
        if( splitIndex2 == -1 ) {
            // Java advice only
            javaAdvice = strToAdvice( adviceString );
            javaScriptAdvice = KJavaScriptDunno;
        } else {
            // Java and JavaScript advice
            javaAdvice = strToAdvice( adviceString.left( splitIndex2 ) );
            javaScriptAdvice = strToAdvice( adviceString.mid( splitIndex2+1,
                                                              adviceString.length() ) );
        }
    }
}

void WebKitSettings::readDomainSettings(const KConfigGroup &config, bool reset,
	bool global, KPerDomainSettings &pd_settings) {
  QString jsPrefix = global ? QString()
  				: QString::fromLatin1("javascript.");
  QString javaPrefix = global ? QString()
  				: QString::fromLatin1("java.");
  QString pluginsPrefix = global ? QString()
  				: QString::fromLatin1("plugins.");

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
    pd_settings.m_windowOpenPolicy = (KJSWindowOpenPolicy)
    		config.readEntry( key, uint(KJSWindowOpenSmart) );
  else if ( !global )
    pd_settings.m_windowOpenPolicy = d->global.m_windowOpenPolicy;

  key = jsPrefix + QLatin1String("WindowMovePolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowMovePolicy = (KJSWindowMovePolicy)
    		config.readEntry( key, uint(KJSWindowMoveAllow) );
  else if ( !global )
    pd_settings.m_windowMovePolicy = d->global.m_windowMovePolicy;

  key = jsPrefix + QLatin1String("WindowResizePolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowResizePolicy = (KJSWindowResizePolicy)
    		config.readEntry( key, uint(KJSWindowResizeAllow) );
  else if ( !global )
    pd_settings.m_windowResizePolicy = d->global.m_windowResizePolicy;

  key = jsPrefix + QLatin1String("WindowStatusPolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowStatusPolicy = (KJSWindowStatusPolicy)
    		config.readEntry( key, uint(KJSWindowStatusAllow) );
  else if ( !global )
    pd_settings.m_windowStatusPolicy = d->global.m_windowStatusPolicy;

  key = jsPrefix + QLatin1String("WindowFocusPolicy");
  if ( (global && reset) || config.hasKey( key ) )
    pd_settings.m_windowFocusPolicy = (KJSWindowFocusPolicy)
    		config.readEntry( key, uint(KJSWindowFocusAllow) );
  else if ( !global )
    pd_settings.m_windowFocusPolicy = d->global.m_windowFocusPolicy;

}


WebKitSettings::WebKitSettings()
	:d (new WebKitSettingsPrivate())
{
  init();
}

WebKitSettings::WebKitSettings(const WebKitSettings &other)
	:d(new WebKitSettingsPrivate())
{
  *d = *other.d;
}

WebKitSettings::~WebKitSettings()
{
  delete d;
}

bool WebKitSettings::changeCursor() const
{
  return d->m_bChangeCursor;
}

bool WebKitSettings::underlineLink() const
{
  return d->m_underlineLink;
}

bool WebKitSettings::hoverLink() const
{
  return d->m_hoverLink;
}

void WebKitSettings::init()
{
  KConfig global( "khtmlrc", KConfig::NoGlobals );
  init( &global, true );

  KSharedConfig::Ptr local = KGlobal::config();
  if ( !local )
    return;

  init( local.data(), false );

}

void WebKitSettings::init( KConfig * config, bool reset )
{
  KConfigGroup cg( config, "MainView Settings" );
  if (reset || cg.exists() )
  {
    if ( reset || cg.hasKey( "OpenMiddleClick" ) )
        d->m_bOpenMiddleClick = cg.readEntry( "OpenMiddleClick", true );

    if ( reset || cg.hasKey( "BackRightClick" ) )
        d->m_bBackRightClick = cg.readEntry( "BackRightClick", false );
  }

  KConfigGroup cgAccess(config,"Access Keys" );
  if (reset || cgAccess.exists() ) {
      d->m_accessKeysEnabled = cgAccess.readEntry( "Enabled", true );
  }

  KConfigGroup cgFilter( config, "Filter Settings" );

  if (reset || cgFilter.exists() )
  {
      d->m_adFilterEnabled = cgFilter.readEntry("Enabled", false);
      d->m_hideAdsEnabled = cgFilter.readEntry("Shrink", false);

      d->adBlackList.clear();
      d->adWhiteList.clear();

      QMap<QString,QString> entryMap = cgFilter.entryMap();
      QMap<QString,QString>::ConstIterator it;
      for( it = entryMap.constBegin(); it != entryMap.constEnd(); ++it )
      {
          QString name = it.key();
          QString url = it.value();

          if (name.startsWith("Filter"))
          {
              if (url.startsWith(QLatin1String("@@")))
                  d->adWhiteList.addFilter(url);
              else
                  d->adBlackList.addFilter(url);
          }
      }
  }

  KConfigGroup cgHtml( config, "HTML Settings" );
  if (reset || cgHtml.exists() )
  {
    // Fonts and colors
    if( reset ) {
        d->defaultFonts = QStringList();
        d->defaultFonts.append( cgHtml.readEntry( "StandardFont", KGlobalSettings::generalFont().family() ) );
        d->defaultFonts.append( cgHtml.readEntry( "FixedFont", KGlobalSettings::fixedFont().family() ) );
        d->defaultFonts.append( cgHtml.readEntry( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT ) );
        d->defaultFonts.append( cgHtml.readEntry( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT ) );
        d->defaultFonts.append( cgHtml.readEntry( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT ) );
        d->defaultFonts.append( cgHtml.readEntry( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT ) );
        d->defaultFonts.append( QString( "0" ) ); // font size adjustment
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
    if ( reset || cgHtml.hasKey( "ChangeCursor" ) )
        d->m_bChangeCursor = cgHtml.readEntry( "ChangeCursor", KDE_DEFAULT_CHANGECURSOR );

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

    if ( reset || cgHtml.hasKey( "UnfinishedImageFrame" ) )
      d->m_bUnfinishedImageFrame = cgHtml.readEntry( "UnfinishedImageFrame", true );

    if ( reset || cgHtml.hasKey( "ShowAnimations" ) )
    {
      QString value = cgHtml.readEntry( "ShowAnimations").toLower();
      if (value == "disabled")
         d->m_showAnimations = KAnimationDisabled;
      else if (value == "looponce")
         d->m_showAnimations = KAnimationLoopOnce;
      else
         d->m_showAnimations = KAnimationEnabled;
    }

    if ( reset || cgHtml.hasKey( "SmoothScrolling" ) )
    {
      QString value = cgHtml.readEntry( "SmoothScrolling", "whenefficient" ).toLower();
      if (value == "disabled")
         d->m_smoothScrolling = KSmoothScrollingDisabled;
      else if (value == "whenefficient")
         d->m_smoothScrolling = KSmoothScrollingWhenEfficient;
      else
         d->m_smoothScrolling = KSmoothScrollingEnabled;
    }

    if ( cgHtml.readEntry( "UserStyleSheetEnabled", false ) == true ) {
        if ( reset || cgHtml.hasKey( "UserStyleSheet" ) )
            d->m_userSheet = cgHtml.readEntry( "UserStyleSheet", "" );
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
    QMap<QString,int> domainList;	// why can't Qt have a QSet?
    for (unsigned i = 0; i < sizeof domain_keys/sizeof domain_keys[0]; ++i) {
      if ( reset || cgJava.hasKey(domain_keys[i]) ) {
        if (i == 0) check_old_ecma_settings = false;
	else if (i == 1) check_old_java_settings = false;
        const QStringList dl = cgJava.readEntry( domain_keys[i], QStringList() );
	const QMap<QString,int>::Iterator notfound = domainList.end();
	QStringList::ConstIterator it = dl.begin();
	const QStringList::ConstIterator itEnd = dl.end();
	for (; it != itEnd; ++it) {
	  const QString domain = (*it).toLower();
	  QMap<QString,int>::Iterator pos = domainList.find(domain);
	  if (pos == notfound) domainList.insert(domain,0);
	}/*next it*/
      }
    }/*next i*/

    if (reset)
      d->domainPolicy.clear();

    {
      QMap<QString,int>::ConstIterator it = domainList.constBegin();
      const QMap<QString,int>::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        const QString domain = it.key();
        KConfigGroup cg( config, domain );
        readDomainSettings(cg,reset,false,d->domainPolicy[domain]);
#ifdef DEBUG_SETTINGS
        d->domainPolicy[domain].dump("init "+domain);
#endif
      }
    }

    bool check_old_java = true;
    if( ( reset || cgJava.hasKey( "JavaDomainSettings" ) )
    	&& check_old_java_settings )
    {
      check_old_java = false;
      const QStringList domainList = cgJava.readEntry( "JavaDomainSettings", QStringList() );
      QStringList::ConstIterator it = domainList.constBegin();
      const QStringList::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        QString domain;
        KJavaScriptAdvice javaAdvice;
        KJavaScriptAdvice javaScriptAdvice;
        splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        setup_per_domain_policy(d,domain).m_bEnableJava =
		javaAdvice == KJavaScriptAccept;
#ifdef DEBUG_SETTINGS
	setup_per_domain_policy(d,domain).dump("JavaDomainSettings 4 "+domain);
#endif
      }
    }

    bool check_old_ecma = true;
    if( ( reset || cgJava.hasKey( "ECMADomainSettings" ) )
	&& check_old_ecma_settings )
    {
      check_old_ecma = false;
      const QStringList domainList = cgJava.readEntry( "ECMADomainSettings", QStringList() );
      QStringList::ConstIterator it = domainList.constBegin();
      const QStringList::ConstIterator itEnd = domainList.constEnd();
      for ( ; it != itEnd; ++it)
      {
        QString domain;
        KJavaScriptAdvice javaAdvice;
        KJavaScriptAdvice javaScriptAdvice;
        splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        setup_per_domain_policy(d,domain).m_bEnableJavaScript =
			javaScriptAdvice == KJavaScriptAccept;
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
        KJavaScriptAdvice javaAdvice;
        KJavaScriptAdvice javaScriptAdvice;
        splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        if( check_old_java )
          setup_per_domain_policy(d,domain).m_bEnableJava =
	  		javaAdvice == KJavaScriptAccept;
        if( check_old_ecma )
          setup_per_domain_policy(d,domain).m_bEnableJavaScript =
	  		javaScriptAdvice == KJavaScriptAccept;
#ifdef DEBUG_SETTINGS
	setup_per_domain_policy(d,domain).dump("JavaScriptDomainAdvice 4 "+domain);
#endif
      }

      //save all the settings into the new keywords if they don't exist
#if 0
      if( check_old_java )
      {
        QStringList domainConfig;
        PolicyMap::Iterator it;
        for( it = d->javaDomainPolicy.begin(); it != d->javaDomainPolicy.end(); ++it )
        {
          QByteArray javaPolicy = adviceToStr( it.value() );
          QByteArray javaScriptPolicy = adviceToStr( KJavaScriptDunno );
          domainConfig.append(QString::fromLatin1("%1:%2:%3").arg(it.key()).arg(javaPolicy).arg(javaScriptPolicy));
        }
        cg.writeEntry( "JavaDomainSettings", domainConfig );
      }

      if( check_old_ecma )
      {
        QStringList domainConfig;
        PolicyMap::Iterator it;
        for( it = d->javaScriptDomainPolicy.begin(); it != d->javaScriptDomainPolicy.end(); ++it )
        {
          QByteArray javaPolicy = adviceToStr( KJavaScriptDunno );
          QByteArray javaScriptPolicy = adviceToStr( it.value() );
          domainConfig.append(QString::fromLatin1("%1:%2:%3").arg(it.key()).arg(javaPolicy).arg(javaScriptPolicy));
        }
        cg.writeEntry( "ECMADomainSettings", domainConfig );
      }
#endif
    }
  }

  // Sync with QWebSettings.
  if (!userStyleSheet().isEmpty()) {
      QWebSettings::globalSettings()->setUserStyleSheetUrl(QUrl(userStyleSheet()));
  }
  QWebSettings::globalSettings()->setAttribute(QWebSettings::AutoLoadImages, autoLoadImages());
  QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, isJavaScriptEnabled());
  QWebSettings::globalSettings()->setAttribute(QWebSettings::JavaEnabled, isJavaEnabled());
  QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, isPluginsEnabled());
  QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,
                                               windowOpenPolicy() != WebKitSettings::KJSWindowOpenDeny);

  QWebSettings::globalSettings()->setFontFamily(QWebSettings::StandardFont, stdFontName());
  QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont, fixedFontName());
  QWebSettings::globalSettings()->setFontFamily(QWebSettings::SerifFont, serifFontName());
  QWebSettings::globalSettings()->setFontFamily(QWebSettings::SansSerifFont, sansSerifFontName());
  QWebSettings::globalSettings()->setFontFamily(QWebSettings::CursiveFont, cursiveFontName());
  QWebSettings::globalSettings()->setFontFamily(QWebSettings::FantasyFont, fantasyFontName());
  QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, minFontSize() * 1.45);
  QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFontSize, mediumFontSize() * 1.45);
}


/** Local helper for retrieving per-domain settings.
  *
  * In case of doubt, the global domain is returned.
  */
static const KPerDomainSettings &lookup_hostname_policy(
			const WebKitSettingsPrivate* const d,
			const QString& hostname)
{
#ifdef DEBUG_SETTINGS
  kDebug() << "lookup_hostname_policy(" << hostname << ")";
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
    kDebug() << "perfect match";
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
      kDebug() << "partial match";
      (*it).dump(host_part);
#endif
      return *it;
    }
    // assert(host_part[0] == QChar('.'));
    host_part.remove(0,1); // Chop off the dot.
  }

  // No domain-specific entry: use global domain
#ifdef DEBUG_SETTINGS
  kDebug() << "no match";
  d->global.dump("global");
#endif
  return d->global;
}

bool WebKitSettings::isOpenMiddleClickEnabled()
{
  return d->m_bOpenMiddleClick;
}

bool WebKitSettings::isBackRightClickEnabled()
{
  return d->m_bBackRightClick;
}

bool WebKitSettings::accessKeysEnabled() const
{
    return d->m_accessKeysEnabled;
}

bool WebKitSettings::isAdFilterEnabled() const
{
    return d->m_adFilterEnabled;
}

bool WebKitSettings::isHideAdsEnabled() const
{
    return d->m_hideAdsEnabled;
}

bool WebKitSettings::isAdFiltered( const QString &url ) const
{
    if (d->m_adFilterEnabled)
    {
        if (!url.startsWith("data:"))
        {
            // Check the blacklist, and only if that matches, the whitelist
            return d->adBlackList.isUrlMatched(url) && !d->adWhiteList.isUrlMatched(url);
        }
    }
    return false;
}

void WebKitSettings::addAdFilter( const QString &url )
{
    KConfigGroup config = KSharedConfig::openConfig( "khtmlrc", KConfig::NoGlobals )->group( "Filter Settings" );

    QRegExp rx;
    
    // Try compiling to avoid invalid stuff. Only support the basic syntax here...
    // ### refactor somewhat
    if (url.length()>2 && url[0]=='/' && url[url.length()-1] == '/')
    {
        QString inside = url.mid(1, url.length()-2);
        rx.setPattern(inside);
    }
    else
    {
        rx.setPatternSyntax(QRegExp::Wildcard);
        rx.setPattern(url);
    }

    if (rx.isValid())
    {
        int last=config.readEntry("Count", 0);
        QString key = "Filter-" + QString::number(last);
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
        KMessageBox::error(0,
                           rx.errorString(),
                           i18n("Filter error"));
    }
}

bool WebKitSettings::isJavaEnabled( const QString& hostname ) const
{
  return lookup_hostname_policy(d,hostname.toLower()).m_bEnableJava;
}

bool WebKitSettings::isJavaScriptEnabled( const QString& hostname ) const
{
  return lookup_hostname_policy(d,hostname.toLower()).m_bEnableJavaScript;
}

bool WebKitSettings::isJavaScriptDebugEnabled( const QString& /*hostname*/ ) const
{
  // debug setting is global for now, but could change in the future
  return d->m_bEnableJavaScriptDebug;
}

bool WebKitSettings::isJavaScriptErrorReportingEnabled( const QString& /*hostname*/ ) const
{
  // error reporting setting is global for now, but could change in the future
  return d->m_bEnableJavaScriptErrorReporting;
}

bool WebKitSettings::isPluginsEnabled( const QString& hostname ) const
{
  return lookup_hostname_policy(d,hostname.toLower()).m_bEnablePlugins;
}

WebKitSettings::KJSWindowOpenPolicy WebKitSettings::windowOpenPolicy(
				const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowOpenPolicy;
}

WebKitSettings::KJSWindowMovePolicy WebKitSettings::windowMovePolicy(
				const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowMovePolicy;
}

WebKitSettings::KJSWindowResizePolicy WebKitSettings::windowResizePolicy(
				const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowResizePolicy;
}

WebKitSettings::KJSWindowStatusPolicy WebKitSettings::windowStatusPolicy(
				const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowStatusPolicy;
}

WebKitSettings::KJSWindowFocusPolicy WebKitSettings::windowFocusPolicy(
				const QString& hostname) const {
  return lookup_hostname_policy(d,hostname.toLower()).m_windowFocusPolicy;
}

int WebKitSettings::mediumFontSize() const
{
    return d->m_fontSize;
}

int WebKitSettings::minFontSize() const
{
  return d->m_minFontSize;
}

QString WebKitSettings::settingsToCSS() const
{
    // lets start with the link properties
    QString str = "a:link {\ncolor: ";
    str += d->m_linkColor.name();
    str += ';';
    if(d->m_underlineLink)
        str += "\ntext-decoration: underline;";

    if( d->m_bChangeCursor )
    {
        str += "\ncursor: pointer;";
        str += "\n}\ninput[type=image] { cursor: pointer;";
    }
    str += "\n}\n";
    str += "a:visited {\ncolor: ";
    str += d->m_vLinkColor.name();
    str += ';';
    if(d->m_underlineLink)
        str += "\ntext-decoration: underline;";

    if( d->m_bChangeCursor )
        str += "\ncursor: pointer;";
    str += "\n}\n";

    if(d->m_hoverLink)
        str += "a:link:hover, a:visited:hover { text-decoration: underline; }\n";

    return str;
}

const QString &WebKitSettings::availableFamilies()
{
    if ( !avFamilies ) {
        avFamilies = new QString;
        QFontDatabase db;
        QStringList families = db.families();
        QStringList s;
        QRegExp foundryExp(" \\[.+\\]");

        //remove foundry info
        QStringList::Iterator f = families.begin();
        const QStringList::Iterator fEnd = families.end();

        for ( ; f != fEnd; ++f ) {
                (*f).remove(foundryExp);
                if (!s.contains(*f))
                        s << *f;
        }
        s.sort();

        *avFamilies = ',' + s.join(",") + ',';
    }

  return *avFamilies;
}

QString WebKitSettings::lookupFont(int i) const
{
    QString font;
    if (d->fonts.count() > i)
       font = d->fonts[i];
    if (font.isEmpty())
        font = d->defaultFonts[i];
    return font;
}

QString WebKitSettings::stdFontName() const
{
    return lookupFont(0);
}

QString WebKitSettings::fixedFontName() const
{
    return lookupFont(1);
}

QString WebKitSettings::serifFontName() const
{
    return lookupFont(2);
}

QString WebKitSettings::sansSerifFontName() const
{
    return lookupFont(3);
}

QString WebKitSettings::cursiveFontName() const
{
    return lookupFont(4);
}

QString WebKitSettings::fantasyFontName() const
{
    return lookupFont(5);
}

void WebKitSettings::setStdFontName(const QString &n)
{
    while(d->fonts.count() <= 0)
        d->fonts.append(QString());
    d->fonts[0] = n;
}

void WebKitSettings::setFixedFontName(const QString &n)
{
    while(d->fonts.count() <= 1)
        d->fonts.append(QString());
    d->fonts[1] = n;
}

QString WebKitSettings::userStyleSheet() const
{
    return d->m_userSheet;
}

bool WebKitSettings::isFormCompletionEnabled() const
{
  return d->m_formCompletionEnabled;
}

int WebKitSettings::maxFormCompletionItems() const
{
  return d->m_maxFormCompletionItems;
}

const QString &WebKitSettings::encoding() const
{
  return d->m_encoding;
}

bool WebKitSettings::followSystemColors() const
{
    return d->m_follow_system_colors;
}

const QColor& WebKitSettings::textColor() const
{
  return d->m_textColor;
}

const QColor& WebKitSettings::baseColor() const
{
  return d->m_baseColor;
}

const QColor& WebKitSettings::linkColor() const
{
  return d->m_linkColor;
}

const QColor& WebKitSettings::vLinkColor() const
{
  return d->m_vLinkColor;
}

bool WebKitSettings::autoLoadImages() const
{
  return d->m_bAutoLoadImages;
}

bool WebKitSettings::unfinishedImageFrame() const
{
  return d->m_bUnfinishedImageFrame;
}

WebKitSettings::KAnimationAdvice WebKitSettings::showAnimations() const
{
  return d->m_showAnimations;
}

WebKitSettings::KSmoothScrollingMode WebKitSettings::smoothScrolling() const
{
  return d->m_smoothScrolling;
}

bool WebKitSettings::isAutoDelayedActionsEnabled() const
{
  return d->m_autoDelayedActionsEnabled;
}

bool WebKitSettings::jsErrorsEnabled() const
{
  return d->m_jsErrorsEnabled;
}

void WebKitSettings::setJSErrorsEnabled(bool enabled)
{
  d->m_jsErrorsEnabled = enabled;
  // save it
  KConfigGroup cg( KGlobal::config(), "HTML Settings");
  cg.writeEntry("ReportJSErrors", enabled);
  cg.sync();
}

bool WebKitSettings::allowTabulation() const
{
    return d->m_allowTabulation;
}

bool WebKitSettings::autoSpellCheck() const
{
    return d->m_autoSpellCheck;
}

QList< QPair< QString, QChar > > WebKitSettings::fallbackAccessKeysAssignments() const
{
    return d->m_fallbackAccessKeysAssignments;
}

void WebKitSettings::setJSPopupBlockerPassivePopup(bool enabled)
{
    d->m_jsPopupBlockerPassivePopup = enabled;
    // save it
    KConfigGroup cg( KGlobal::config(), "Java/JavaScript Settings");
    cg.writeEntry("PopupBlockerPassivePopup", enabled);
    cg.sync();
}

bool WebKitSettings::jsPopupBlockerPassivePopup() const
{
    return d->m_jsPopupBlockerPassivePopup;
}

K_GLOBAL_STATIC(WebKitSettings, s_webKitSettings)

WebKitSettings* WebKitSettings::self()
{
    return s_webKitSettings;
}


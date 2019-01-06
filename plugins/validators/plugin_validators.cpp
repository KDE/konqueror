/* This file is part of Validators
 *
 *  It's a merge of the HTML- and the CSSValidator
 *
 *  Copyright (C) 2001 by  Richard Moore <rich@kde.org>
 *                         Andreas Schlapbach <schlpbch@iam.unibe.ch>
 *  Copyright (C) 2008-2009 by  Pino Toscano <pino@kde.org>
 *
 *  for information how to write your own plugin see:
 *    http://developer.kde.org/documentation/tutorials/dot/writing-plugins.html
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "plugin_validators.h"

#include "clickiconlabel.h"
#include "settings.h"

#include <qtextdocument.h>

#include <kaboutdata.h>

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kcolorscheme.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <khtml_part.h>

#include <KLocalizedString>
#include <QMenu>
#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <kstatusbar.h>
#include <kparts/browserextension.h>
#include <kparts/statusbarextension.h>
#include <KIconLoader>

#include <config-konq-validator.h>

#ifdef HAVE_TIDY
#include "tidy_validator.h"
#include "reportdialog.h"
#endif

K_PLUGIN_FACTORY(PluginValidatorsFactory, registerPlugin<PluginValidators>();)

const char PluginValidators::s_boundary[] = "KonquerorValidatorPlugin";
const char PluginValidators::s_CRLF[] = "\r\n";

static QByteArray createPostData(const QList<QPair<QByteArray, QByteArray> > &formData)
{
    QByteArray postData;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = formData.constBegin(), itEnd = formData.constEnd();
    const QByteArray dash("--");
    for (; it != itEnd; ++it) {
        postData += dash + PluginValidators::s_boundary + PluginValidators::s_CRLF
                    + "Content-Disposition: form-data; name=\"" + it->first + "\""
                    + PluginValidators::s_CRLF + PluginValidators::s_CRLF
                    + it->second + PluginValidators::s_CRLF;

    }
    postData += dash + PluginValidators::s_boundary + dash;
    return postData;
}

bool acceptHTMLFrame(const QString &name)
{
    // skip Google's advertising (i)frames
    if (name.startsWith(QLatin1String("google_ads_frame"))
            || name.startsWith(QLatin1String("google_ads_iframe"))
       ) {
        return false;
    }

    return true;
}

#ifdef HAVE_TIDY
static void recursiveKHTMLValidation(KHTMLPart *part, QList<ValidationResult *> *results)
{
    const QStringList frameNames = part->frameNames();
    int i = 0;
    Q_FOREACH (KParts::ReadOnlyPart *frame, part->frames()) {
        if (KHTMLPart *khtmlpart = qobject_cast<KHTMLPart *>(frame)) {
            if (acceptHTMLFrame(frameNames.at(i))) {
                ValidationResult *res = new ValidationResult();
                res->frameName = frameNames.at(i);
                {
                    TidyValidator v(part->documentSource().toUtf8());
                    res->errors = v.errors();
                    res->warnings = v.warnings();
                    res->accesswarns = v.accessibilityWarnings();
                }
                results->append(res);

                recursiveKHTMLValidation(khtmlpart, results);
            }
        }
        ++i;
    }
}
#endif

PluginValidators::PluginValidators(QObject *parent,
                                   const QVariantList &)
    : Plugin(parent), m_configDialog(nullptr), m_part(nullptr)
    , m_localValidation(nullptr), m_localValidationReport(nullptr)
    , m_icon(nullptr), m_statusBarExt(nullptr)
{

    m_menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("validators")), i18n("&Validate Web Page"),
                             actionCollection());
    actionCollection()->addAction(QStringLiteral("validateWebpage"), m_menu);
    m_menu->setDelayed(false);

    m_validateHtmlUri = m_menu->menu()->addAction(QIcon::fromTheme(QStringLiteral("htmlvalidator")),
                        i18n("Validate HTML (by URI)"),
                        this, SLOT(slotValidateHtmlByUri()));

    m_validateHtmlUpload = m_menu->menu()->addAction(QIcon::fromTheme(QStringLiteral("htmlvalidator")),
                           i18n("Validate HTML (by Upload)"),
                           this, SLOT(slotValidateHtmlByUpload()));

    m_validateCssUri = m_menu->menu()->addAction(QIcon::fromTheme(QStringLiteral("cssvalidator")),
                       i18n("Validate CSS (by URI)"),
                       this, SLOT(slotValidateCssByUri()));

    m_validateCssUpload = m_menu->menu()->addAction(QIcon::fromTheme(QStringLiteral("cssvalidator")),
                          i18n("Validate CSS (by Upload)"),
                          this, SLOT(slotValidateCssByUpload()));
    // FIXME temporary disabled, as it does not work
    m_validateCssUpload->setVisible(false);

    m_validateLinks = m_menu->menu()->addAction(i18n("Validate &Links"), this, SLOT(slotValidateLinks()));

#ifdef HAVE_TIDY
    m_menu->menu()->addSeparator();

    m_localValidation = m_menu->menu()->addAction(QIcon::fromTheme("validators"),
                        i18n("Validate Page"),
                        this, SLOT(slotTidyValidation()));

    m_localValidationReport = m_menu->menu()->addAction(QIcon::fromTheme("document-properties"),
                              i18n("View Validator Report"),
                              this, SLOT(slotShowTidyValidationReport()));
#endif

    if (parent) {
        m_menu->menu()->addSeparator();

        m_menu->menu()->addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("C&onfigure Validator..."),
                                  this, SLOT(slotConfigure()));

        m_part = qobject_cast<KParts::ReadOnlyPart *>(parent);
        m_configDialog = new ValidatorsDialog(m_part->widget());
        connect(m_configDialog, SIGNAL(configChanged()), this, SLOT(setURLs()));
        setURLs();

        connect(m_part, SIGNAL(started(KIO::Job*)), this,
                SLOT(slotStarted(KIO::Job*)));
        connect(m_part, SIGNAL(completed()), this, SLOT(slotCompleted()));
    }
}

PluginValidators::~PluginValidators()
{
    removeStatusBarIcon();
    delete m_configDialog;
#ifdef HAVE_TIDY
    qDeleteAll(m_lastResults);
#endif
// Dont' delete the action. KActionCollection as parent does the job already
// and not deleting it at this point also ensures that in case we are not unplugged
// from the GUI yet and the ~KXMLGUIClient destructor will do so it won't hit a
// dead pointer. The kxmlgui factory keeps references to the actions, but it does not
// connect to their destroyed() signal, yet (need to find an elegant solution for that
// as it can easily increase the memory usage significantly) . That's why actions must
// persist as long as the plugin is plugged into the GUI.
//  delete m_menu;
}

QString elementOfList(const QStringList &list, uint index)
{
    return static_cast<int>(index) >= list.count() ? QString() : list.at(index);
}

QString getWWWValidatorUrl()
{
    return elementOfList(ValidatorsSettings::wWWValidatorUrl(), ValidatorsSettings::wWWValidatorUrlIndex());
}

QString getWWWValidatorUploadUrl()
{
    return elementOfList(ValidatorsSettings::wWWValidatorUploadUrl(), ValidatorsSettings::wWWValidatorUploadUrlIndex());
}

QString getCSSValidatorUrl()
{
    return elementOfList(ValidatorsSettings::cSSValidatorUrl(), ValidatorsSettings::cSSValidatorUrlIndex());
}

QString getCSSValidatorUploadUrl()
{
    return elementOfList(ValidatorsSettings::cSSValidatorUploadUrl(), ValidatorsSettings::cSSValidatorUploadUrlIndex());
}

QString getLinkValidatorUrl()
{
    return elementOfList(ValidatorsSettings::linkValidatorUrl(), ValidatorsSettings::linkValidatorUrlIndex());
}

void PluginValidators::setURLs()
{
    m_WWWValidatorUrl = QUrl(getWWWValidatorUrl());
    m_CSSValidatorUrl = QUrl(getCSSValidatorUrl());
    m_WWWValidatorUploadUrl = QUrl(getWWWValidatorUploadUrl());
    m_CSSValidatorUploadUrl = QUrl(getCSSValidatorUploadUrl());
    m_linkValidatorUrl = QUrl(getLinkValidatorUrl());
}

void PluginValidators::slotStarted(KIO::Job *)
{
    removeStatusBarIcon();

    const bool byUri = canValidateByUri();
    const bool byUpload = canValidateByUpload();
    m_validateHtmlUri->setEnabled(byUri);
    m_validateHtmlUpload->setEnabled(byUpload);
    m_validateCssUri->setEnabled(byUri);
    m_validateCssUpload->setEnabled(byUpload);
    m_validateLinks->setEnabled(byUri);
#ifdef HAVE_TIDY
    m_localValidation->setEnabled(false);
    m_localValidationReport->setEnabled(false);
#endif
}

void PluginValidators::slotCompleted()
{
    const bool byUri = canValidateByUri();
    const bool byUpload = canValidateByUpload();
    const bool locally = canValidateLocally();
    m_validateHtmlUri->setEnabled(byUri);
    m_validateHtmlUpload->setEnabled(byUpload);
    m_validateCssUri->setEnabled(byUri);
    m_validateCssUpload->setEnabled(byUpload);
    m_validateLinks->setEnabled(byUri);
#ifdef HAVE_TIDY
    m_localValidation->setEnabled(locally);
    m_localValidationReport->setEnabled(false);
#endif

    addStatusBarIcon();

#ifdef HAVE_TIDY
    if (ValidatorsSettings::runAfterLoading() && locally) {
        slotTidyValidation();
    }
#endif
}

void PluginValidators::slotValidateHtmlByUri()
{
    validateByUri(m_WWWValidatorUrl);
}

void PluginValidators::slotValidateHtmlByUpload()
{
    if (!m_WWWValidatorUploadUrl.isValid()) {
        return;
    }

    QList<QPair<QByteArray, QByteArray> > postData;
    postData.append(QPair<QByteArray, QByteArray>("fragment", documentSource().split('\n').join(QLatin1String(s_CRLF)).toUtf8()));
    postData.append(QPair<QByteArray, QByteArray>("prefill", "0"));
    postData.append(QPair<QByteArray, QByteArray>("doctype", "Inline"));
    postData.append(QPair<QByteArray, QByteArray>("prefill_doctype", "html401"));
    postData.append(QPair<QByteArray, QByteArray>("group", "0"));
    validateByUpload(m_WWWValidatorUploadUrl, postData);
}

void PluginValidators::slotValidateCssByUri()
{
    validateByUri(m_CSSValidatorUrl);
}

void PluginValidators::slotValidateCssByUpload()
{
}

void PluginValidators::slotValidateLinks()
{
    validateByUri(m_linkValidatorUrl);
}

void PluginValidators::slotContextMenu()
{
    QMenu menu(m_part->widget());
    menu.setTitle(i18n("Remote Validation"));
    menu.addAction(m_validateHtmlUri);
    menu.addAction(m_validateHtmlUpload);
    menu.addAction(m_validateCssUri);
    menu.addAction(m_validateCssUpload);
    menu.addAction(m_validateLinks);
#ifdef HAVE_TIDY
    menu.setTitle(i18n("Local Validation"));
    menu.addAction(m_localValidation);
    menu.addAction(m_localValidationReport);
#endif
    menu.exec(QCursor::pos());
}

void PluginValidators::slotTidyValidation()
{
#ifdef HAVE_TIDY
    qDeleteAll(m_lastResults);
    m_lastResults.clear();
    if (KHTMLPart *khtmlpart = qobject_cast<KHTMLPart *>(m_part)) {
        ValidationResult *res = new ValidationResult();
        {
            TidyValidator v(khtmlpart->documentSource().toUtf8());
            res->errors = v.errors();
            res->warnings = v.warnings();
            res->accesswarns = v.accessibilityWarnings();
        }
        m_lastResults.append(res);

        recursiveKHTMLValidation(khtmlpart, &m_lastResults);
    } else {
        return;
    }
    QList<ValidationResult *>::ConstIterator vIt = m_lastResults.constBegin(), vItEnd = m_lastResults.constEnd();
    int errorCount = 0;
    int warningCount = 0;
    int a11yWarningCount = 0;
    for (; vIt != vItEnd; ++vIt) {
        ValidationResult *res = *vIt;
        errorCount += res->errors.count();
        warningCount += res->warnings.count();
        a11yWarningCount += res->accesswarns.count();
    }

    const QString errorCountString = i18np("1 error", "%1 errors", errorCount);
    const QString warningCountString = i18np("1 warning", "%1 warnings", warningCount);
    const QString a11yWarningCountString = i18np("1 accessibility warning", "%1 accessibility warnings", a11yWarningCount);
    m_icon->setText(i18nc("%1 is the error count string, %2 the warning count string",
                          "%1, %2", errorCountString, warningCountString));
    QStringList results;
    results.append(i18n("HTML tidy results:") + QLatin1String("\n"));
    if (m_lastResults.count() == 1) {
        results.append(errorCountString);
        results.append(warningCountString);
        if (ValidatorsSettings::accessibilityLevel()) {
            results.append(a11yWarningCountString);
        }
    } else if (m_lastResults.count() > 1) {
        vIt = m_lastResults.constBegin();
        if (ValidatorsSettings::accessibilityLevel())
            results.append(i18nc("%1 is the error count string, %2 the warning count string, "
                                 "%3 the accessibility warning string",
                                 "Page: %1, %2, %3",
                                 i18np("1 error", "%1 errors", (*vIt)->errors.count()),
                                 i18np("1 warning", "%1 warnings", (*vIt)->warnings.count()),
                                 i18np("1 accessibility warning", "%1 accessibility warnings", (*vIt)->accesswarns.count())));
        else
            results.append(i18nc("%1 is the error count string, %2 the warning count string",
                                 "Page: %1, %2",
                                 i18np("1 error", "%1 errors", (*vIt)->errors.count()),
                                 i18np("1 warning", "%1 warnings", (*vIt)->warnings.count())));
        ++vIt;
        for (; vIt != vItEnd; ++vIt) {
            ValidationResult *res = *vIt;
            if (ValidatorsSettings::accessibilityLevel())
                results.append(i18nc("%1 is the HTML frame name, %2 is the error count string, "
                                     "%3 the warning count string, %4 the accessibility warning string",
                                     "Frame '%1': %2, %3, %4",
                                     res->frameName.toHtmlEscaped(),
                                     i18np("1 error", "%1 errors", (*vIt)->errors.count()),
                                     i18np("1 warning", "%1 warnings", (*vIt)->warnings.count()),
                                     i18np("1 accessibility warning", "%1 accessibility warnings", (*vIt)->accesswarns.count())));
            else
                results.append(i18nc("%1 is the HTML frame name, %2 is the error count string, "
                                     "%3 the warning count string",
                                     "Frame '%1': %2, %3",
                                     res->frameName.toHtmlEscaped(),
                                     i18np("1 error", "%1 errors", (*vIt)->errors.count()),
                                     i18np("1 warning", "%1 warnings", (*vIt)->warnings.count())));
        }
    }
    m_icon->setToolTip(results.join(QLatin1String("\n")));
    QPalette pal = m_icon->palette();
    if (errorCount > 0) {
        KColorScheme::adjustBackground(pal, KColorScheme::NegativeBackground, QPalette::Window);
        KColorScheme::adjustForeground(pal, KColorScheme::NegativeText, QPalette::WindowText);
    } else if (warningCount > 0) {
        KColorScheme::adjustBackground(pal, KColorScheme::NeutralBackground, QPalette::Window);
        KColorScheme::adjustForeground(pal, KColorScheme::NeutralText, QPalette::WindowText);
    } else {
        KColorScheme::adjustBackground(pal, KColorScheme::PositiveBackground, QPalette::Window);
        KColorScheme::adjustForeground(pal, KColorScheme::PositiveText, QPalette::WindowText);
    }
    m_icon->setPalette(pal);
    m_localValidationReport->setEnabled(true);
#endif
}

void PluginValidators::slotShowTidyValidationReport()
{
#ifdef HAVE_TIDY
    ReportDialog *reportDialog = new ReportDialog(m_lastResults, nullptr);
    reportDialog->setAttribute(Qt::WA_DeleteOnClose);
    reportDialog->show();
#endif
}

void PluginValidators::slotConfigure()
{
    m_configDialog->show();
}

void PluginValidators::validateByUri(const QUrl &url)
{
    if (!doExternalValidationChecks()) {
        return;
    }

    QUrl partUrl = m_part->url();
    QUrl validatorUrl(url);
    if (!partUrl.password().isEmpty()) {
        KMessageBox::sorry(
            m_part->widget(),
            i18n("<qt>The selected URL cannot be verified because it contains "
                 "a password. Sending this URL to <b>%1</b> would put the security "
                 "of <b>%2</b> at risk.</qt>",
                 validatorUrl.host(), partUrl.host()));
        return;
    }
    // Set entered URL as a parameter
    validatorUrl.addQueryItem(QStringLiteral("uri"), partUrl.url());
    kDebug(90120) << "final URL: " << validatorUrl.url();
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(m_part);
    KParts::OpenUrlArguments urlArgs;
    KParts::BrowserArguments browserArgs;
    browserArgs.setNewTab(true);
    emit ext->openUrlRequest(validatorUrl, urlArgs, browserArgs);
}

void PluginValidators::validateByUpload(const QUrl &validatorUrl, const QList<QPair<QByteArray, QByteArray> > &formData)
{
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(m_part);
    KParts::OpenUrlArguments urlArgs;
    KParts::BrowserArguments browserArgs;
    browserArgs.setNewTab(true);
    browserArgs.setContentType(QStringLiteral("Content-Type: multipart/form-data; Boundary=%1").arg(PluginValidators::s_boundary));
    browserArgs.postData = createPostData(formData);
    browserArgs.setDoPost(true);
    browserArgs.setRedirectedRequest(true);
    emit ext->openUrlRequest(validatorUrl, urlArgs, browserArgs);
}

bool PluginValidators::canValidateByUri() const
{
    return m_part->url().scheme() == QLatin1String("http");
}

bool PluginValidators::canValidateByUpload() const
{
    return canValidateLocally();
}

bool PluginValidators::canValidateLocally() const
{
    // we can track only HTML renderer components
    if (!parent()->inherits("KHTMLPart")) {
        return false;
    }

    static const char *exclude_protocols[] = {
        "about",
        "bookmarks",
        nullptr // keep it as last!
    };
    const QByteArray proto = m_part->url().scheme().toAscii();
    for (const char **protoIt = exclude_protocols; *protoIt; ++protoIt) {
        if (proto == *protoIt) {
            return false;
        }
    }

    return true;
}

QString PluginValidators::documentSource() const
{
    if (KHTMLPart *khtmlpart = qobject_cast<KHTMLPart *>(m_part)) {
        return khtmlpart->documentSource();
    }
    return QString();
}

bool PluginValidators::doExternalValidationChecks()
{
    if (!parent()->inherits("KHTMLPart") && !parent()->inherits("KWebKitPart")) {
        const QString title = i18nc("@title:window", "Cannot Validate Source");
        const QString text = i18n("You cannot validate anything except web pages with "
                                  "this plugin.");

        KMessageBox::sorry(nullptr, text, title);
        return false;
    }

    // Get URL
    QUrl partUrl = m_part->url();
    if (!partUrl.isValid()) { // Just in case ;)
        const QString title = i18nc("@title:window", "Malformed URL");
        const QString text = i18n("The URL you entered is not valid, please "
                                  "correct it and try again.");
        KMessageBox::sorry(nullptr, text, title);
        return false;
    }

    return true;
}

void PluginValidators::addStatusBarIcon()
{
    // already an icon placed
    if (m_icon) {
        return;
    }

    if (!canValidateLocally()) {
        return;
    }

    m_statusBarExt = KParts::StatusBarExtension::childObject(m_part);
    if (!m_statusBarExt) {
        return;
    }

    m_icon = new ClickIconLabel(m_statusBarExt->statusBar());
    m_icon->setFixedHeight(KIconLoader::global()->currentSize(KIconLoader::Small));
    m_icon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_icon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("htmlvalidator"), KIconLoader::Small));
    m_icon->setToolTip(i18n("Validation"));
    m_icon->setAutoFillBackground(true);
    connect(m_icon, SIGNAL(leftClicked()), this, SLOT(slotContextMenu()));
    m_statusBarExt->addStatusBarItem(m_icon, 0, true);
}

void PluginValidators::removeStatusBarIcon()
{
    if (!m_icon) {
        return;
    }

    m_statusBarExt = KParts::StatusBarExtension::childObject(m_part);
    if (!m_statusBarExt) {
        return;
    }

    m_statusBarExt->removeStatusBarItem(m_icon);
    delete m_icon;
    m_icon = nullptr;
}

#include <plugin_validators.moc>

/*
    SPDX-FileCopyrightText: 2001 Andreas Schlapbach <schlpbch@iam.unibe.ch>
    SPDX-FileCopyrightText: 2003 Antonio Larrosa <larrosa@kde.org>
    SPDX-FileCopyrightText: 2008 Matthias Grimrath <maps4711@gmx.de>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivedialog.h"

#include <QLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDialogButtonBox>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QGroupBox>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QTimer>
#include <QRegularExpression>

#include <klocalizedstring.h>
#include <kurlrequester.h>
#include <kmessagebox.h>
#include <ktar.h>
#include <kzip.h>
#include <krecentdirs.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <kstandardguiitem.h>
#include <kprotocolmanager.h>
#include <kconfiggroup.h>
#include <kpluralhandlingspinbox.h>

#include <kio/statjob.h>
#include <kio/deletejob.h>
#include <kio/copyjob.h>

#include "webarchiverdebug.h"
#include "settings.h"


ArchiveDialog::ArchiveDialog(const QUrl &url, QWidget *parent)
    : KMainWindow(parent)
{
    setObjectName("ArchiveDialog");

    // Generate a default name for the web archive.
    // First try the file name of the URL, trimmed of any recognised suffix.
    QString archiveName = url.fileName();
    QMimeDatabase db;
    archiveName.chop(db.suffixForFileName(archiveName).length());
    if (archiveName.isEmpty())
    {
        //The implementation in KF5 constructed the archive name basing on QUrl::topLevelDomain()
        //Since that function doesn't exist anymore, and there's no easy way to replace it,
        //use a simpler algorithm: just replace each dot in the host with an underscore
        archiveName = url.host().replace(".", "_");				// host name from URL

    }

    // Find the last archive save location used
    QString dir = KRecentDirs::dir(":save");
    if (dir.isEmpty()) dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!dir.endsWith('/')) dir += '/';
    // Generate the base path and name for the archive file
    QString fileBase = dir+archiveName.simplified();
    qCDebug(WEBARCHIVERPLUGIN_LOG) << url << "->" << fileBase;

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Close, this);

    m_archiveButton = qobject_cast<QPushButton *>(m_buttonBox->button(QDialogButtonBox::Ok));
    Q_ASSERT(m_archiveButton!=nullptr);
    m_archiveButton->setDefault(true);
    m_archiveButton->setIcon(KStandardGuiItem::save().icon());
    m_archiveButton->setText(i18n("Create Archive"));
    connect(m_archiveButton, &QAbstractButton::clicked, this, &ArchiveDialog::slotCreateButtonClicked);
    m_buttonBox->addButton(m_archiveButton, QDialogButtonBox::ActionRole);

    m_cancelButton =  qobject_cast<QPushButton *>(m_buttonBox->button(QDialogButtonBox::Close));
    Q_ASSERT(m_cancelButton!=nullptr);
    connect(m_cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

    QWidget *w = new QWidget(this);			// main widget
    QVBoxLayout *vbl = new QVBoxLayout(w);		// main vertical layout
    KConfigSkeletonItem *ski;				// config for creating widgets

    m_guiWidget = new QWidget(this);			// the main GUI widget
    QFormLayout *fl = new QFormLayout(m_guiWidget);	// layout for entry form

    m_pageUrlReq = new KUrlRequester(url, this);
    m_pageUrlReq->setToolTip(i18n("The URL of the page that is to be archived"));
    slotSourceUrlChanged(m_pageUrlReq->text());
    connect(m_pageUrlReq, &KUrlRequester::textChanged, this, &ArchiveDialog::slotSourceUrlChanged);
    fl->addRow(i18n("Source &URL:"), m_pageUrlReq);

    fl->addRow(QString(), new QWidget(this));

    ski = Settings::self()->archiveTypeItem();
    Q_ASSERT(ski!=nullptr);
    m_typeCombo = new QComboBox(this);
    m_typeCombo->setSizePolicy(QSizePolicy::Expanding, m_typeCombo->sizePolicy().verticalPolicy());
    m_typeCombo->setToolTip(ski->toolTip());
    m_typeCombo->addItem(QIcon::fromTheme("webarchiver"), i18n("Web archive (*.war)"), "application/x-webarchive");
    m_typeCombo->addItem(QIcon::fromTheme("application-x-archive"), i18n("Tar archive (*.tar)"), "application/x-tar");
    m_typeCombo->addItem(QIcon::fromTheme("application-zip"), i18n("Zip archive (*.zip)"), "application/zip");
    m_typeCombo->addItem(QIcon::fromTheme("folder"), i18n("Directory"), "inode/directory");
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ArchiveDialog::slotArchiveTypeChanged);
    fl->addRow(ski->label(), m_typeCombo);

    m_saveUrlReq = new KUrlRequester(QUrl::fromLocalFile(fileBase), this);
    m_saveUrlReq->setToolTip(i18n("The file or directory where the archived page will be saved"));
    fl->addRow(i18n("&Save to:"), m_saveUrlReq);

    QGroupBox *grp = new QGroupBox(i18n("Options"), this);
    grp->setFlat(true);
    fl->addRow(grp);

    ski = Settings::self()->waitTimeItem();
    Q_ASSERT(ski!=nullptr);
    m_waitTimeSpinbox = new KPluralHandlingSpinBox(this);
    m_waitTimeSpinbox->setMinimumWidth(100);
    m_waitTimeSpinbox->setToolTip(ski->toolTip());
    m_waitTimeSpinbox->setRange(ski->minValue().toInt(), ski->maxValue().toInt());
    m_waitTimeSpinbox->setSuffix(ki18np(" second", " seconds"));
    m_waitTimeSpinbox->setSpecialValueText(i18n("None"));
    connect(m_waitTimeSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int val) { m_randomWaitCheck->setEnabled(val>0); });
    fl->addRow(ski->label(), m_waitTimeSpinbox);

    fl->addRow(QString(), new QWidget(this));

    ski = Settings::self()->noProxyItem();
    Q_ASSERT(ski!=nullptr);
    m_noProxyCheck = new QCheckBox(ski->label(), this);
    m_noProxyCheck->setToolTip(ski->toolTip());
    fl->addRow(QString(), m_noProxyCheck);

    ski = Settings::self()->randomWaitItem();
    Q_ASSERT(ski!=nullptr);
    m_randomWaitCheck = new QCheckBox(ski->label(), this);
    m_randomWaitCheck->setToolTip(ski->toolTip());
    fl->addRow(QString(), m_randomWaitCheck);

    ski = Settings::self()->fixExtensionsItem();
    Q_ASSERT(ski!=nullptr);
    m_fixExtensionsCheck = new QCheckBox(ski->label(), this);
    m_fixExtensionsCheck->setToolTip(ski->toolTip());
    fl->addRow(QString(), m_fixExtensionsCheck);

    fl->addRow(QString(), new QWidget(this));

    ski = Settings::self()->runInTerminalItem();
    Q_ASSERT(ski!=nullptr);
    m_runInTerminalCheck = new QCheckBox(ski->label(), this);
    m_runInTerminalCheck->setToolTip(ski->toolTip());
    fl->addRow(QString(), m_runInTerminalCheck);

    ski = Settings::self()->closeWhenFinishedItem();
    Q_ASSERT(ski!=nullptr);
    m_closeWhenFinishedCheck = new QCheckBox(ski->label(), this);
    m_closeWhenFinishedCheck->setToolTip(ski->toolTip());
    fl->addRow(QString(), m_closeWhenFinishedCheck);

    vbl->addWidget(m_guiWidget);
    vbl->setStretchFactor(m_guiWidget, 1);

    m_messageWidget = new KMessageWidget(this);
    m_messageWidget->setWordWrap(true);
    m_messageWidget->hide();
    connect(m_messageWidget, &KMessageWidget::linkActivated, this, &ArchiveDialog::slotMessageLinkActivated);
    vbl->addWidget(m_messageWidget);

    vbl->addWidget(m_buttonBox);
    setCentralWidget(w);

    setAutoSaveSettings(objectName(), true);
    readSettings();

    m_tempDir = nullptr;
    m_tempFile = nullptr;

    // Check the current system proxy settings.  Being a command line tool,
    // wget(1) can only use proxy environment variables;  if KIO is set to
    // use these also then there is no problem.  Otherwise, warn the user
    // that the settings cannot be used.
    enum ProxySettings {NoProxy, EnvVarProxy, SpecialProxy};
    ProxySettings proxyType = NoProxy;
    int proxyTypeAsInt = KSharedConfig::openConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals)->group("Proxy Settings").readEntry("ProxyType", 0);
    //According to kio-extras/kcms/ksaveioconfig.h, 0 means "No proxy" and 4 means "proxy from environment variable"
    if (proxyTypeAsInt == 4) {
        proxyType = EnvVarProxy;
    } else if (proxyTypeAsInt != 0) {
        proxyType = SpecialProxy;
    }
    if (proxyType == NoProxy)		// no proxy configured.
    {							// we cannot use one either
        m_noProxyCheck->setChecked(true);
        m_noProxyCheck->setEnabled(false);
    }
    else if (proxyType == SpecialProxy)	// special KIO setting,
    {							// but we cannot use it
        m_noProxyCheck->setChecked(true);
        m_noProxyCheck->setEnabled(false);
        showMessage(xi18nc("@info", "The web archive download cannot use the current proxy settings. No proxy will be used."),
                    KMessageWidget::Information);
    }

    slotArchiveTypeChanged(m_typeCombo->currentIndex());
}


ArchiveDialog::~ArchiveDialog()
{
    cleanup();						// process and temporary files
}


void ArchiveDialog::cleanup()
{
    if (!m_archiveProcess.isNull()) m_archiveProcess->deleteLater();

    delete m_tempDir;
    m_tempDir = nullptr;

    if (m_tempFile!=nullptr)
    {
        m_tempFile->setAutoRemove(true);
        delete m_tempFile;
        m_tempFile = nullptr;
    }
}


void ArchiveDialog::slotSourceUrlChanged(const QString &text)
{
    m_archiveButton->setEnabled(QUrl::fromUserInput(text).isValid());
}


void ArchiveDialog::slotArchiveTypeChanged(int idx)
{
    const QString saveType = m_typeCombo->itemData(idx).toString();
    qCDebug(WEBARCHIVERPLUGIN_LOG) << saveType;

    QUrl url = m_saveUrlReq->url();
    url = url.adjusted(QUrl::StripTrailingSlash);
    QString fileName = url.fileName();
    url = url.adjusted(QUrl::RemoveFilename);

    QMimeDatabase db;
    fileName.chop(db.suffixForFileName(fileName).length());
    if (fileName.endsWith('.')) fileName.chop(1);

    if (saveType!="inode/directory")
    {
        const QMimeType mimeType = db.mimeTypeForName(saveType);
        fileName += '.';
        fileName += mimeType.preferredSuffix();
    }

    url.setPath(url.path()+fileName);

    if (saveType=="inode/directory") m_saveUrlReq->setMode(KFile::Directory);
    else m_saveUrlReq->setMode(KFile::File);
    m_saveUrlReq->setMimeTypeFilters(QStringList() << saveType);
    m_saveUrlReq->setUrl(url);
}


bool ArchiveDialog::queryClose()
{
    // If the archive process is not running, the button is "Close"
    // and will just close the window.
    if (m_archiveProcess.isNull()) return (true);

    // Just signal the process here.  slotProcessFinished() will clean up
    // and ask whether to retain a partial download.
    m_archiveProcess->terminate();
    return (false);					// don't close just yet
}


void ArchiveDialog::slotMessageLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}


void ArchiveDialog::setGuiEnabled(bool on)
{
    m_guiWidget->setEnabled(on);
    m_archiveButton->setEnabled(on);
    m_cancelButton->setText((on ? KStandardGuiItem::close() : KStandardGuiItem::cancel()).text());

    if (!on) QGuiApplication::setOverrideCursor(Qt::BusyCursor);
    else QGuiApplication::restoreOverrideCursor();
}


void ArchiveDialog::saveSettings()
{
    Settings::setArchiveType(m_typeCombo->currentData().toString());

    Settings::setWaitTime(m_waitTimeSpinbox->value());

    if (m_noProxyCheck->isEnabled()) Settings::setNoProxy(m_noProxyCheck->isChecked());
    Settings::setRandomWait(m_randomWaitCheck->isChecked());
    Settings::setFixExtensions(m_fixExtensionsCheck->isChecked());
    Settings::setRunInTerminal(m_runInTerminalCheck->isChecked());
    Settings::setCloseWhenFinished(m_closeWhenFinishedCheck->isChecked());

    Settings::self()->save();
}


void ArchiveDialog::readSettings()
{
    const int idx = m_typeCombo->findData(Settings::archiveType());
    if (idx!=-1) m_typeCombo->setCurrentIndex(idx);

    m_waitTimeSpinbox->setValue(Settings::waitTime());

    m_noProxyCheck->setChecked(Settings::noProxy());
    m_randomWaitCheck->setChecked(Settings::randomWait());
    m_fixExtensionsCheck->setChecked(Settings::fixExtensions());
    m_runInTerminalCheck->setChecked(Settings::runInTerminal());
    m_closeWhenFinishedCheck->setChecked(Settings::closeWhenFinished());

    m_randomWaitCheck->setEnabled(m_waitTimeSpinbox->value()>0);
}


void ArchiveDialog::slotCreateButtonClicked()
{
    setGuiEnabled(false);				// while archiving is in progress
    showMessage("");					// clear any existing message

    m_saveUrl = m_saveUrlReq->url();
    qCDebug(WEBARCHIVERPLUGIN_LOG) << m_saveUrl;

    if (!m_saveUrl.isValid())
    {
        showMessageAndCleanup(i18nc("@info", "The save location is not valid."), KMessageWidget::Error);
        return;
    }

    // Remember the archive save location used
    QUrl url = m_saveUrl.adjusted(QUrl::RemoveFilename);
    if (url.isValid()) KRecentDirs::add(":save", url.toString(QUrl::PreferLocalFile));

    // Also save the window size and the other GUI options.
    // From here on we can use Settings to access them.
    saveSettings();

    // Check that the wget(1) command is available before doing too much other work
    if (m_wgetProgram.isEmpty())
    {
        m_wgetProgram = QStandardPaths::findExecutable("wget");
        qCDebug(WEBARCHIVERPLUGIN_LOG) << "wget program" << m_wgetProgram;
        if (m_wgetProgram.isEmpty())
        {
            showMessageAndCleanup(xi18nc("@info",
                                         "Cannot find the wget(1) command,<nl/>see <link>%1</link>.",
                                         "https://www.gnu.org/software/wget"),
                                  KMessageWidget::Error);
            return;
        }
    }

    // Check whether the destination file or directory exists
    KIO::StatJob *statJob = KIO::stat(m_saveUrl, KIO::StatJob::DestinationSide, KIO::StatBasic);
    connect(statJob, &KJob::result, this, &ArchiveDialog::slotCheckedDestination);
}


void ArchiveDialog::slotCheckedDestination(KJob *job)
{
    KIO::StatJob *statJob = qobject_cast<KIO::StatJob *>(job);
    Q_ASSERT(statJob!=nullptr);

    const int err = job->error();
    if (err!=0 && err!=KIO::ERR_DOES_NOT_EXIST)
    {
        showMessageAndCleanup(xi18nc("@info",
                                     "Cannot verify destination<nl/><filename>%1</filename><nl/>%2",
                                     statJob->url().toDisplayString(), job->errorString()),
                              KMessageWidget::Error);
        return;
    }

    if (err==0) m_saveUrl = statJob->mostLocalUrl();	// update to most local form
    m_saveType = m_typeCombo->itemData(m_typeCombo->currentIndex()).toString();
    qCDebug(WEBARCHIVERPLUGIN_LOG) << m_saveUrl << "as" << m_saveType;

    if (err==0)						// destination already exists
    {
        const bool isDir = statJob->statResult().isDir();
        const QString url = m_saveUrl.toDisplayString(QUrl::PreferLocalFile);
        QString message;
        if (m_saveType=="inode/directory")
        {
            if (!isDir) message = xi18nc("@info", "The archive directory<nl/><filename>%1</filename><nl/>already exists as a file.", url);
            else message = xi18nc("@info", "The archive directory<nl/><filename>%1</filename><nl/>already exists.", url);
        }
        else
        {
            if (isDir) message = xi18nc("@info", "The archive file<nl/><filename>%1</filename><nl/>already exists as a directory.", url);
            else message = xi18nc("@info", "The archive file <nl/><filename>%1</filename><nl/>already exists.", url);
        }

        int result = KMessageBox::warningContinueCancel(this, message,
                                                        i18n("Archive Already Exists"),
                                                        KStandardGuiItem::overwrite(),
                                                        KStandardGuiItem::cancel(),
                                                        QString(),
                                                        KMessageBox::Dangerous);
        if (result==KMessageBox::Cancel)
        {
            showMessageAndCleanup("");
            return;
        }

        KIO::DeleteJob *delJob = KIO::del(m_saveUrl);
        connect(delJob, &KJob::result, this, &ArchiveDialog::slotDeletedOldDestination);
        return;
    }

    slotDeletedOldDestination(nullptr);
}


void ArchiveDialog::slotDeletedOldDestination(KJob *job)
{
    if (job!=nullptr)
    {
        KIO::DeleteJob *delJob = qobject_cast<KIO::DeleteJob *>(job);
        Q_ASSERT(delJob!=nullptr);

        if (job->error())
        {
            showMessageAndCleanup(xi18nc("@info",
                                         "Cannot delete original archive<nl/><filename>%1</filename><nl/>%2",
                                         m_saveUrl.toDisplayString(), job->errorString()),
                                  KMessageWidget::Error);
            return;
        }
    }

    startDownloadProcess();
}


void ArchiveDialog::startDownloadProcess()
{
    m_tempDir = new QTemporaryDir;
    if (!m_tempDir->isValid())
    {
        showMessageAndCleanup(xi18nc("@info",
                                     "Cannot create a temporary directory<nl/><filename>%1</filename><nl/>",
                                     m_tempDir->path()),
                              KMessageWidget::Error);
        return;
    }
    qCDebug(WEBARCHIVERPLUGIN_LOG) << "temp dir" << m_tempDir->path();

    QProcess *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::ForwardedChannels);
    proc->setStandardInputFile(QProcess::nullDevice());
    proc->setWorkingDirectory(m_tempDir->path());
    Q_ASSERT(!m_wgetProgram.isEmpty());			// should have found this earlier
    proc->setProgram(m_wgetProgram);

    QStringList args;					// argument list for command
    args << "-p";					// fetch page and all requirements
    args << "-k";					// convert to relative links
    args << "-nH";					// do not create host directories
    // This option is incompatible with '-k'
    //args << "-nc";					// no clobber of existing files
    args << "-H";					// fetch from foreign hosts
    args << "-nv";					// not quite so verbose
    args << "--progress=dot:default";			// progress indication
    args << "-R" << "robots.txt";			// ignore this file

    if (Settings::fixExtensions())			// want standard archive format
    {
        args << "-nd";					// no subdirectory structure
        args << "-E";					// fix up file extensions
    }

    const int waitTime = Settings::waitTime();
    if (waitTime>0)					// wait time requested?
    {
        args << "-w" << QString::number(waitTime);	// wait time between requests
        if (Settings::randomWait())
        {
            args << "--random-wait";			// randomise wait time
        }
    }

    if (Settings::noProxy())				// no proxy requested?
    {
        args << "--no-proxy";				// do not use proxy
    }

    args << m_pageUrlReq->url().toEncoded();		// finally the page URL

    qCDebug(WEBARCHIVERPLUGIN_LOG) << "wget args" << args;
    if (Settings::runInTerminal())			// running in a terminal?
    {
        args.prepend(proc->program());			// prepend existing "wget"
        args.prepend("-e");				// terminal command to execute
        args.prepend("--hold");				// then terminal options

        // from kservice/src/kdeinit/ktoolinvocation_x11.cpp
        KConfigGroup generalGroup(KSharedConfig::openConfig(), "General");
        const QString term = generalGroup.readPathEntry("TerminalApplication", QStringLiteral("konsole"));
        proc->setProgram(term);				// set terminal as program

        qCDebug(WEBARCHIVERPLUGIN_LOG) << "terminal" << term << "args" << args;
    }
    proc->setArguments(args);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ArchiveDialog::slotProcessFinished);

    m_archiveProcess = proc;				// note for cleanup later
    proc->start();					// start the archiver process
    if (!proc->waitForStarted(2000))
    {
        showMessageAndCleanup(xi18nc("@info",
                                     "Cannot start the archiver process <filename>%1</filename>",
                                     m_wgetProgram),
                              KMessageWidget::Error);
        return;
    }
}


void ArchiveDialog::slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qCDebug(WEBARCHIVERPLUGIN_LOG) << "code" << exitCode << "status" << exitStatus;

    QString message;
    if (exitStatus==QProcess::CrashExit)
    {
        // See if we terminated the process ourselves via queryClose()
        if (exitCode==SIGTERM) message = xi18nc("@info", "The download process was interrupted.");
        else message = xi18nc("@info", "<para>The download process <filename>%1</filename> failed with signal %2.</para>",
                              m_archiveProcess->program(), QString::number(exitCode));
    }
    else if (exitCode!=0)
    {
        message = xi18nc("@info", "<para>The download process <filename>%1</filename> failed with status %2.</para>",
                         m_archiveProcess->program(), QString::number(exitCode));
        if (exitCode==8)
        {
            message += xi18nc("@info", "<para>This may simply indicate a 404 error for some of the page elements.</para>");
        }
    }

    if (!message.isEmpty())
    {
        message += xi18nc("@info", "<para>Retain the partial web archive?</para>");
        if (KMessageBox::questionTwoActions(this, message, i18n("Retain Archive?"),
                                       KGuiItem(i18n("Retain"), KStandardGuiItem::save().icon()),
                                       KStandardGuiItem::discard())==KMessageBox::SecondaryAction)
        {
            showMessageAndCleanup(xi18nc("@info",
                                         "Creating the web archive failed."),
                                  KMessageWidget::Warning);
            return;
        }
    }

    finishArchive();
}


void ArchiveDialog::finishArchive()
{
    QDir tempDir(m_tempDir->path());			// where the download now is

    if (Settings::fixExtensions())
    {
        // Look at the names at the top level of the temporary download directory,
        // and see if there is a single HTML file there.  If there is, then depending
        // on whether the original URL ended with a slash (which we cannot check or
        // correct because we cannot know whether it should or not), wget(1) may
        // save the HTML page as "lastcomponent.html" instead of "index.html".
        //
        // If this is the case, then rename the file in question to "index.html".
        // This is so that a web archive will open in Konqueror showing the HTML
        // page as intended.
        //
        // If saving as a directory or as another type of archive file, then
        // this does not apply.  However, do the rename anyway so that the
        // file naming is consistent regardless of what is being saved.

        QRegularExpression rx(QStringLiteral("(?!^index)\\.html?$"), QRegularExpression::CaseInsensitiveOption);		// a negative lookahead assertion!
        QString indexHtml;				// the index file found

        // This listing can simply use QDir::entryList() because only the
        // file names are needed.
        const QStringList entries = tempDir.entryList(QDir::Dirs|QDir::Files|QDir::QDir::NoDotAndDotDot);
        qCDebug(WEBARCHIVERPLUGIN_LOG) << "found" << entries.count() << "entries";

        for (const QString &name : entries)		// first pass, check file names
        {
            if (name.contains(rx))			// matches "anythingelse.html"
            {						// but not "index.html"
                if (!indexHtml.isEmpty())		// already have found something
                {
                    qCDebug(WEBARCHIVERPLUGIN_LOG) << "multiple HTML files at top level";
                    indexHtml.clear();			// forget trying to rename
                    break;
                }

                qCDebug(WEBARCHIVERPLUGIN_LOG) << "identified index file" << name;
                indexHtml = name;
            }
        }

        if (!indexHtml.isEmpty())			// have identified index file
        {
            tempDir.rename(indexHtml, "index.html");	// rename it to standard name
        }
    }

    QString sourcePath;					// archive to be copied

    // The archived web page is now ready in the temporary directory.
    // If it is required to be saved as a file, then create a
    // temporary archive file in the same place.
    if (m_saveType!="inode/directory")			// saving as archive file
    {
        QMimeDatabase db;
        const QString ext = db.mimeTypeForName(m_saveType).preferredSuffix();
        m_tempFile = new QTemporaryFile(QDir::tempPath()+'/'+
                                        QCoreApplication::applicationName()+
                                        "-XXXXXX."+ext);
        m_tempFile->setAutoRemove(false);
        m_tempFile->open();
        QString tempArchive = m_tempFile->fileName();
        qCDebug(WEBARCHIVERPLUGIN_LOG) << "temp archive" << tempArchive;
        m_tempFile->close();				// only want the name

        KArchive *archive;
        if (m_saveType=="application/zip")
        {
            archive = new KZip(tempArchive);
        }
        else
        {
            if (m_saveType=="application/x-webarchive")
            {
                // A web archive is a gzip-compressed tar file
                archive = new KTar(tempArchive, "application/x-gzip");
            }
            else archive = new KTar(tempArchive);
        }
        archive->open(QIODevice::WriteOnly);

        // Read each entry in the temporary directory and add it to the archive.
        // Cannnot simply use addLocalDirectory(m_tempDir->path()) here, because
        // that would add an extra directory level within the archive having the
        // random name of the temporary directory.
        //
        // This listing needs to use QDir::entryInfoList() so that adding to the
        // archive can distinguish between files and directories.  The list needs
        // to be refreshed because the page HTML file could have been renamed above.
        const QFileInfoList entries = tempDir.entryInfoList(QDir::Dirs|QDir::Files|QDir::QDir::NoDotAndDotDot);
        qCDebug(WEBARCHIVERPLUGIN_LOG) << "adding" << entries.count() << "entries";

        for (const QFileInfo &fi : entries)		// second pass, write out entries
        {
            if (fi.isFile())
            {
                qCDebug(WEBARCHIVERPLUGIN_LOG) << "  adding file" << fi.absoluteFilePath();
                archive->addLocalFile(fi.absoluteFilePath(), fi.fileName());
            }
            else if (fi.isDir())
            {
                qCDebug(WEBARCHIVERPLUGIN_LOG) << "  adding dir" << fi.absoluteFilePath();
                archive->addLocalDirectory(fi.absoluteFilePath(), fi.fileName());
            }
            else qCDebug(WEBARCHIVERPLUGIN_LOG) << "unrecognised entry type for" << fi.fileName();
        }

        archive->close();				// finished with archive file
        sourcePath = tempArchive;			// source path to copy
    }
    else						// saving as a directory
    {
        sourcePath = tempDir.absolutePath();		// source path to copy
    }

    // Finally copy the temporary file or directory to the requested save location
    KIO::CopyJob *copyJob = KIO::copyAs(QUrl::fromLocalFile(sourcePath), m_saveUrl);
    connect(copyJob, &KJob::result, this, &ArchiveDialog::slotCopiedArchive);
}


void ArchiveDialog::slotCopiedArchive(KJob *job)
{
    KIO::CopyJob *copyJob = qobject_cast<KIO::CopyJob *>(job);
    Q_ASSERT(copyJob!=nullptr);
    const QUrl destUrl = copyJob->destUrl();

    if (job->error())
    {
        showMessageAndCleanup(xi18nc("@info",
                                     "Cannot copy archive to<nl/><filename>%1</filename><nl/>%2",
                                     destUrl.toDisplayString(), job->errorString()),
                              KMessageWidget::Error);
        return;
    }

    // Explicitly set permissions on the saved archive file or directory,
    // to honour the user's umask(2) setting.  This is needed because
    // both QTemporaryFile and QTemporaryDir create them with restrictive
    // permissions (as indeed they should) by default.  The files within
    // the temporary directory will have been written by wget(1) with
    // standard creation permissions, so it does not need to be done
    // recursively.
    const mode_t perms = (m_saveType=="inode/directory") ? 0777 : 0666;
    const mode_t mask = umask(0); umask(mask);

    KIO::SimpleJob *chmodJob = KIO::chmod(destUrl, (perms & ~mask));
    connect(chmodJob, &KJob::result, this, &ArchiveDialog::slotFinishedArchive);
}


void ArchiveDialog::slotFinishedArchive(KJob *job)
{
    KIO::SimpleJob *chmodJob = qobject_cast<KIO::SimpleJob *>(job);
    Q_ASSERT(chmodJob!=nullptr);
    const QUrl destUrl = chmodJob->url();

    if (job->error())
    {
        showMessageAndCleanup(xi18nc("@info",
                                     "Cannot set permissions on<nl/><filename>%1</filename><nl/>%2",
                                     destUrl.toDisplayString(), job->errorString()),
                              KMessageWidget::Warning);
        return;						// do not close even if requested
    }
    else
    {
        showMessageAndCleanup(xi18nc("@info",
                                     "Web archive saved as<nl/><filename><link>%1</link></filename>",
                                     destUrl.toDisplayString()),
                              KMessageWidget::Positive);
    }

    // Now the archiving task is finished.
    if (Settings::closeWhenFinished())
    {
        // Let the user briefly see the completion message.
        QTimer::singleShot(1000, qApp, &QCoreApplication::quit);
    }
}


void ArchiveDialog::showMessageAndCleanup(const QString &text, KMessageWidget::MessageType type)
{
    showMessage(text, type);
    cleanup();
    setGuiEnabled(true);
}


void ArchiveDialog::showMessage(const QString &text, KMessageWidget::MessageType type)
{
    if (text.isEmpty())					// remove existing message
    {
        m_messageWidget->hide();
        return;
    }

    QString iconName;
    switch (type)
    {
case KMessageWidget::Positive:		iconName = "dialog-ok";			break;
case KMessageWidget::Information:	iconName = "dialog-information";	break;
default:
case KMessageWidget::Warning:		iconName = "dialog-warning";		break;
case KMessageWidget::Error:		iconName = "dialog-error";		break;
    }

    m_messageWidget->setCloseButtonVisible(type!=KMessageWidget::Positive && type!=KMessageWidget::Information);
    m_messageWidget->setIcon(QIcon::fromTheme(iconName));
    m_messageWidget->setMessageType(type);
    m_messageWidget->setText(text);
    m_messageWidget->show();
}

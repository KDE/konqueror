/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef URLLLOADER_H
#define URLLLOADER_H

#include "konqopenurlrequest.h"

#include <QObject>
#include <QUrl>

#include <KService>
#include <KParts/BrowserOpenOrSaveQuestion>

namespace KParts {
    class ReadOnlyPart;
};

class KJob;
namespace KIO {
    class OpenUrlJob;
    class ApplicationLauncherJob;
}

class KonqMainWindow;
class KonqView;

class UrlLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param parent TODO
     */
    UrlLoader(KonqMainWindow *mainWindow, KonqView *view, const QUrl &url, const QString &mimeType, const KonqOpenURLRequest &req, bool trustedSource, bool forceOpen=false);
    ~UrlLoader();

    /** @brief Enum describing the possible actions to be taken*/
    enum class OpenUrlAction{
        UnknwonAction, /**< The action hasn't been decided yet */
        DoNothing, /**< No action should be taken */
        Save, /**< Save the URL */
        Embed, /**< Display the URL in an embedded viewer */
        Open, /**< Display the URL in a separate viewer*/
        Execute /**< Execute the URL */
    };

    /** @brief Enum describing the view to use to embed an URL*/
    enum class ViewToUse{
        View, /**< Use the view passed as argument to the constructor */
        CurrentView, /**< Use the current view */
        NewTab /**< Create a new tab and use its view */
    };

    void start();
    void performAction();
    void goOn();

    void abort();

    QString mimeType() const;
    bool isReady() const {return m_ready;}
    ViewToUse viewToUse() const;
    QUrl url() const {return m_url;}
    KonqOpenURLRequest request() const {return m_request;}
    KonqView* view() const {return m_view;}
    void setView(KonqView *view);
    bool isAsync() const {return m_isAsync;}
    void setOldLocationBarUrl(const QString &old);
    QString oldLocationBarUrl() const {return m_oldLocationBarUrl;}
    bool hasError() const {return m_jobHadError;}
    void setNewTab(bool newTab);
    static bool isExecutable(const QString &mimeType);
    QString suggestedFileName() const {return m_request.suggestedFileName;}

    static QString partForLocalFile(const QString &path);

signals:
    void finished(UrlLoader *self);

private slots:

    void mimetypeDeterminedByJob(const QString &mimeType);
    void jobFinished(KJob* job);
    void done(KJob *job=nullptr);

private:

    void embed();
    void open();
    void execute();
    void save();

    bool shouldEmbedThis() const;
    void saveUrlUsingKIO(const QUrl &orig, const QUrl &dest);
    void detectSettingsForLocalFiles();
    void detectSettingsForRemoteFiles();
    void launchOpenUrlJob(bool pauseOnMimeTypeDetermined);
    static bool isTextExecutable(const QString &mimeType);
    void openExternally();
    void killOpenUrlJob();
    static bool serviceIsKonqueror(KService::Ptr service);
    static bool isMimeTypeKnown(const QString &mimeType);
    bool decideEmbedOrSave();
    void decideOpenOrSave();
    bool embedWithoutAskingToSave(const QString &mimeType);
    bool shouldUseDefaultHttpMimeype() const;
    void decideAction();
    bool isViewLocked() const;

    typedef QPair<OpenUrlAction, KService::Ptr> OpenSaveAnswer;

    enum class OpenEmbedMode{Open, Embed};
    OpenSaveAnswer askSaveOrOpen(OpenEmbedMode mode) const;
    OpenUrlAction decideExecute() const;

private:
    QPointer<KonqMainWindow> m_mainWindow;
    QUrl m_url;
    QString m_mimeType;
    KonqOpenURLRequest m_request;
    KonqView *m_view;
    bool m_trustedSource;
    bool m_dontEmbed;
    bool m_ready = false;
    bool m_isAsync = false;
    OpenUrlAction m_action = OpenUrlAction::UnknwonAction;
    KService::Ptr m_service;
    QPointer<KIO::OpenUrlJob> m_openUrlJob;
    QPointer<KIO::ApplicationLauncherJob> m_applicationLauncherJob;
    QString m_oldLocationBarUrl;
    bool m_jobHadError;
    bool m_dontPassToWebEnginePart;
};

QDebug operator<<(QDebug dbg, UrlLoader::OpenUrlAction action);

#endif // URLLLOADER_H

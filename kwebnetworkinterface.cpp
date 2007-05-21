#include "kwebnetworkinterface.h"

#include <kio/job.h>
#include <qdebug.h>

Q_DECLARE_METATYPE(QWebNetworkJob *);
Q_DECLARE_METATYPE(KJob *);

KWebNetworkInterface::KWebNetworkInterface()
{
}

void KWebNetworkInterface::addJob(QWebNetworkJob *job)
{
    KIO::Job *kioJob = 0;
    QByteArray postData = job->postData();
    if (postData.isEmpty())
        kioJob = KIO::get(job->url());
    else
        kioJob = KIO::http_post(job->url(), postData);

    kioJob->addMetaData("PropagateHttpHeader", "true");

    kioJob->setProperty("qwebnetworkjob", QVariant::fromValue(job));
    m_jobs.insert(job, kioJob);

    connect(kioJob, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(forwardJobData(KIO::Job *, const QByteArray &)));
    connect(kioJob, SIGNAL(result(KJob *)),
            this, SLOT(forwardJobResult(KJob *)));
}

void KWebNetworkInterface::cancelJob(QWebNetworkJob *job)
{
    KJob *kjob = m_jobs.take(job);
    if (kjob)
        kjob->kill();
}

void KWebNetworkInterface::forwardJobData(KIO::Job *kioJob, const QByteArray &data)
{
    QWebNetworkJob *job = kioJob->property("qwebnetworkjob").value<QWebNetworkJob *>();
    if (!job)
        return;

    QString headers = kioJob->queryMetaData("HTTP-Headers");
    if (!headers.isEmpty()) {
        job->setResponse(QHttpResponseHeader(kioJob->queryMetaData("HTTP-Headers")));
        emit started(job);
    }

    emit this->data(job, data);
}

void KWebNetworkInterface::forwardJobResult(KJob *kjob)
{
    QWebNetworkJob *job = kjob->property("qwebnetworkjob").value<QWebNetworkJob *>();
    if (!job)
        return;
    m_jobs.remove(job);
    emit finished(job, kjob->error());
}

#include "kwebnetworkinterface.moc"


/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WEBENGINEPARTKIOHANDLER_H
#define WEBENGINEPARTKIOHANDLER_H

#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineUrlRequestJob>
#include <QPointer>
#include <QMimeType>

namespace KIO {
  class StoredTransferJob;  
};

class WebEnginePartHtmlEmbedder;

/**
 * @brief Class which allows QWebEngine to access URLs provided by KIO
 * 
 * The class assumes that the data can be obtained from KIO using `KIO::storedGet` and
 * passes it to `QWebEngineUrlRequestJob::reply`. The mime type is determined using 
 * `QMimeDatabase::mimeTypeForData`.If the data is HTML or XHTML, 
 * WebEnginePartHtmlEmbedder is used to embed the contents of local URLs inside the code,
 * so that it can be displayed by QWebEngine without breaking cross-origin rules.
 * 
 * This class provides a virtual function, processSlaveOutput() which allows to further process KIO output before
 * using it as reply. Derived classes can use it to change the data, its mimetype and to set 
 * an error code and error message.
 * 
 * @internal
 * 
 * Since requestStarted() can be called multiple times, requests are stored in a list and 
 * processed one by one. This is hidden to derived classes, which only work with the current request.
 */
class WebEnginePartKIOHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param parent the parent object
     */
    WebEnginePartKIOHandler(QObject* parent);
    
    /**
     * @brief Override of `QWebEngineUrlSchemeHandler::requestStarted`
     * 
     * It adds the new request to the list and start processing it if there aren't
     * other queued requests
     *
     * @param  req: the request job
     */
    void requestStarted(QWebEngineUrlRequestJob* req) override;
    
protected slots:
    
    /**
    * @brief Slot called in response to the WebEnginePartHtmlEmbedder::finished() signal
    * 
    * The base class implementation calls setData() with the resulting HTML code, then
    * emits the ready() signal. You can override this function to do something else with
    * the HTML code. In this case, most likely you won't want to call the base class version
    * but just call setData() and emit the ready() when you're done processing the HTML code
    * 
    * @param html: the HTML code produced by the embedder 
    */
    virtual void embedderFinished(const QString &html);
    
protected:
    
    /**
    * @brief Creates and returns the html embedder object
    * 
    * This is the object used to replace local URLs with their content as `data` URLs. 
    *
    * The embedder is only created on the first use of this function. When this happens, its
    * WebEnginePartHtmlEmbedder::finished() signal is connected to the embedderFinished(). If you
    * don't want this, either disconnect the signal or reimplement embedderFinished() so that it
    * does nothing, then connect WebEnginePartHtmlEmbedder::finished() with the appropriate slot
    * yourself
    * 
    * @return the html embedder
    */
    WebEnginePartHtmlEmbedder* htmlEmbedder();
    
    /**
     * @brief The request object
     * 
     * @return The request object
     */
    inline QPointer<QWebEngineUrlRequestJob> request() const {return m_currentRequest;}
    
    /**
     * @brief The error code to pass to `QWebEngineUrlRequestJob::fail`
     * 
     * In the base class implementation, this can be either `QWebEngineUrlRequestJob::NoError` or
     * `QWebEngineUrlRequestJob::RequestFailed`.
     * 
     * @return the error code to pass to `QWebEngineUrlRequestJob::fail`
     */
    inline QWebEngineUrlRequestJob::Error error() const {return m_error;}
    
    /**
     * @brief Sets the error code passed to `QWebEngineUrlRequestJob::fail`
     * 
     * Changes the error code. This function can be called by derived class in their
     * implementation of processSlaveOutput(). It should always be set to `QWebEngineUrlRequestJob::NoError`
     * if `QWebEngineUrlRequestJob::reply` should be called.
     * 
     * @see isSuccessful()
     * 
     * @param error: the new error code
     */
    inline void setError(QWebEngineUrlRequestJob::Error error) {m_error = error;}
    
    /**
     * @brief Whether an error has occurred or not
     * 
     * @return `true` if error() is `QWebEngineUrlRequestJob::NoError` and false otherwise
     */
    inline bool isSuccessful() const {return m_error == QWebEngineUrlRequestJob::NoError;}
    
    /**
     * @brief The error message
     * 
     * Currently, this is only used for debugging purpose
     * 
     * @return The error message given by `KJob::errorString` (or set by derived classes using setErrorMessage())
     *  or an empty string if KIO produced no error.
     */
    inline QString errorMessage() const {return m_errorMessage;}
    
    /**
     * @brief Changes the error message
     * 
     * This function can be called by derived classes from their implementation of
     * processSlaveOutput().
     * 
     * @param message the new error message. If error() is not `QWebEngineUrlRequestJob::NoError`, this
     *  should not be empty
     */
    inline void setErrorMessage(const QString &message) {m_errorMessage = message;}
    
    /**
     * @brief The data to pass to `QWebEngineUrlRequestJob::reply`
     * 
     * This is the data returned by the ioslave, but can be changed in processSlaveOutput() using setData()
     * 
     * @return The data to pass to QWebEngineUrlRequestJob::reply
     */
    inline QByteArray data() const {return m_data;}
    
    /**
     * @brief Changes the data to pass to `QWebEngineUrlRequestJob::reply`
     * 
     * This function can be used by derived classes in their implementation of processSlaveOutput(). The base 
     * class implementation calls it for (X)HTML code to embed the content of local files in the code itself.
     * 
     * @param data: the new data
     */
    inline void setData(const QByteArray& data) {m_data = data;}
    
    /**
     * @brief The mime type of the data
     * 
     * This value (actually, it's `name`) will be passed to QWebEngineUrlRequestJob::reply
     * @return The mime type of the data
     */
    inline QMimeType mimeType() const {return m_mimeType;}
    
    /**
     * @brief Changes the mime type of the data
     * 
     * This function can be used by derived classes in their implementation of processSlaveOutput()
     * if the mime type according to `QMimeDatabase::mimeTypeForData` is not correct or if they change
     * the data so that its mimetype changes from what the ioslave returned.
     */
    inline void setMimeType(const QMimeType &mime) {m_mimeType = mime;}
    
    /**
     * @brief Processes the output from the ioslave and replies to the request
     * 
     * In the base class implementation, if the data is (X)HTML code (according to mimetype()), the
     * htmlEmbedder() is used to embed the content of local files in the code. In response to 
     * WebEnginePartHtmlEmbedder::finished(), the ready() signal is emitted. For other mimetypes, the
     * ready() signal is emitted directly from here.
     * 
     * Derived classes can reimplement this function to do their own processing, of the data produced
     * by the ioslave. At the beginning of this function, data(), mimetype(), and errorMessage() are 
     * those produced by the ioslave, while error is `QWebEngineUrlRequestJob::NoError` if the ioslave 
     * was successful andd `QWebEngineUrlRequestJob::RequestFailed` if there were errors. Derived classes can
     * change these values using setData(), setMimeType(), setError() and setErrorMessage().
     * 
     * It is important that, after having changed the values as desired, the ready() signal is emitted.
     */
    virtual void processSlaveOutput();
    
signals:
    
    /**
    * @brief Signal emitted when processing of data produced by the ioslave has finished
    * 
    * In response to this signal, sendReply() is called, sending the response to the original request
    */
    void ready();
    
private slots:
    
    /**
     * Slot called when the ioslave finishes
     * 
     * It reads the data, mimetype and error state from the job and stores it.
     * 
     * @param job: the finished ioslave job
     */
    
    void kioJobFinished(KIO::StoredTransferJob *job);
    
    /**
     * @brief Sends a reply to the `QWebEngineUrlRequestJob`
     * 
     * The reply is sent using the values returned by data(), mimeType(), error() and errorMessage().
     * 
     * If error() is something else from `QWebEngineUrlRequestJob::NoError`, this function calls
     * `QWebEngineUrlRequestJob::fail` with the corresponding error code, otherwise it calls 
     * `QWebEngineUrlRequestJob::reply()` passing data() and mimetype() as arguments.
     * 
     * This slot is called in response to the ready() signal
     */
    void sendReply();
    
private:
    
    /**
     * @brief Starts processing the next `QWebEngineUrlRequestJob` in queue
     * 
     * @internal
     * This function removes the first (valid) request from #m_queuedRequests,
     * stores it in #m_currentRequest and starts a `KIO::storedGet` for its URL
     */
    void processNextRequest();
    
    using RequestJobPointer = QPointer<QWebEngineUrlRequestJob>;
    
    /**
     * @brief A list of requests to be processed
     */
    QList<RequestJobPointer> m_queuedRequests;
    
    /**
     * @brief The request currently being processed
     */
    RequestJobPointer m_currentRequest;
    
    /**
     * @brief The error code to use for `QWebEngineUrlRequestJob::fail` for the current request
     * 
     * This is valid only after the call to kioJobFinished()
     */
    QWebEngineUrlRequestJob::Error m_error;
    
    /**
     * @brief The error message produced by KIO for the current request
     * 
     * Currently, this is only used for debugging purposes
     * 
     * This is valid only after the call to kioJobFinished()
     */
    QString m_errorMessage;
    
    /**
     * @brief The data produced by KIO for the current request
     * 
     * This is valid only after the call to kioJobFinished()
     */ 
    QByteArray m_data;
    
    /**
     * @brief The mime type of the data produced by KIO for the current request
     * 
     * This is valid only after the call to kioJobFinished()
     */ 
    QMimeType m_mimeType;
    
    /**
     * @brief The object to use for embedding `file` URLs in (X)HTML code
     * 
     * This empty until the first time htmlEmbedder() is called
     */
    WebEnginePartHtmlEmbedder *m_embedder;
};

#endif // WEBENGINEPARTKIOHANDLER_H

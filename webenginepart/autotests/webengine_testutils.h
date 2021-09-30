/*
    SPDX-FileCopyrightText: 2016 The Qt Company Ltd. <https://www.qt.io/licensing/>

    This file is part of the QtWebEngine module of the Qt Toolkit.
    SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0 OR LicenseRef-Qt-Commercial
*/
#ifndef WEBENGINE_TESTUTILS_H
#define WEBENGINE_TESTUTILS_H

#include <QEventLoop>
#include <QTimer>
#include <QWebEnginePage>

template<typename T, typename R>
struct RefWrapper {
    R &ref;
    void operator()(const T& result) {
        ref(result);
    }
};

template<typename T>
class CallbackSpy {
public:
    CallbackSpy() : called(false) {
        timeoutTimer.setSingleShot(true);
        QObject::connect(&timeoutTimer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    }

    T waitForResult() {
        if (!called) {
            timeoutTimer.start(10000);
            eventLoop.exec();
        }
        return result;
    }

    bool wasCalled() const {
        return called;
    }

    void operator()(const T &result) {
        this->result = result;
        called = true;
        eventLoop.quit();
    }

    // Cheap rip-off of boost/std::ref
    RefWrapper<T, CallbackSpy<T> > ref()
    {
        RefWrapper<T, CallbackSpy<T> > wrapper = {*this};
        return wrapper;
    }

private:
    Q_DISABLE_COPY(CallbackSpy)
    bool called;
    QTimer timeoutTimer;
    QEventLoop eventLoop;
    T result;
};

// taken from the qwebengine unittests
static inline QVariant evaluateJavaScriptSync(QWebEnginePage *page, const QString &script)
{
    CallbackSpy<QVariant> spy;
    page->runJavaScript(script, spy.ref());
    return spy.waitForResult();
}

// Taken from QtWebEngine's tst_qwebenginepage.cpp
static QPoint elementCenter(QWebEnginePage *page, const QString &id)
{
    QVariantList rectList = evaluateJavaScriptSync(page,
            "(function(){"
            "var elem = document.getElementById('" + id + "');"
            "var rect = elem.getBoundingClientRect();"
            "return [rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top];"
            "})()").toList();

    if (rectList.count() != 4) {
        qWarning("elementCenter failed.");
        return QPoint();
    }
    const QRect rect(rectList.at(0).toInt(), rectList.at(1).toInt(),
                     rectList.at(2).toInt(), rectList.at(3).toInt());
    return rect.center();
}
#endif

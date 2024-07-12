#ifndef __konq_aboutpage_h__
#define __konq_aboutpage_h__

#include <QWebEngineUrlSchemeHandler>


class KonqAboutPageSingleton
{
public:
    QString launch();
    QString intro();
    QString tips();
    QString plugins();

private:
    QString m_launch_html, m_intro_html, m_specs_html, m_tips_html, m_plugins_html;
};

class KonqUrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT
public:
    KonqUrlSchemeHandler(QObject *parent=nullptr);
    ~KonqUrlSchemeHandler() Q_DECL_OVERRIDE;
    
    void requestStarted(QWebEngineUrlRequestJob *req) Q_DECL_OVERRIDE;

private:

    QString m_htmlDoc;
    QString m_what;
};

#endif

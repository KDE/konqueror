#ifndef __konq_aboutpage_h__
#define __konq_aboutpage_h__

#include <QWebEngineUrlSchemeHandler>

#include <KIconLoader>

class KonqAboutPageSingleton
{
public:
    QString launch();
    QString intro();
    QString tips();
    QString plugins();

private:

    /**
     * @brief The URL to use in the html page to refer to a given icon
     *
     * `KIconLoader::iconPath()` can return either path corresponding to a Qt resource (starting with `:`)
     * or a file path. This function detects the situation and creates the correct URL.
     * @param iconName the name of the icon
     * @param group the group the icon belongs to
     * @return the string representation of an URL with a `qrc` scheme or a `file` scheme depending on the type of
     * path returned by `KIconLoader::iconPath()`
     */
    static QString urlStringForIconName(const QString &iconName, KIconLoader::Group group = KIconLoader::Desktop);

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

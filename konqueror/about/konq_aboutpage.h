#ifndef __konq_aboutpage_h__
#define __konq_aboutpage_h__

#include <khtml_part.h>

class KonqAboutPageSingleton
{
public:
    KonqAboutPageSingleton();
    ~KonqAboutPageSingleton();

    QString launch();
    QString intro();
    QString specs();
    QString tips();
    QString plugins();

private:
    static QString loadFile( const QString& file );

    QString m_launch_html, m_intro_html, m_specs_html, m_tips_html, m_plugins_html;
};

class KonqAboutPage : public KHTMLPart
{
    Q_OBJECT
public:
    KonqAboutPage(QWidget *parentWidget, QObject *parent, const QVariantList& args);
    ~KonqAboutPage();

    virtual bool openUrl( const KUrl &url );

    virtual bool openFile();

    virtual void saveState( QDataStream &stream );
    virtual void restoreState( QDataStream &stream );

protected:
    virtual bool urlSelected( const QString &url, int button, int state, const QString &target,
                              const KParts::OpenUrlArguments& args = KParts::OpenUrlArguments(),
                              const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments() );

private:
    void serve( const QString&, const QString& );

    KHTMLPart *m_doc;
    QString m_htmlDoc;
    QString m_what;
};

#endif

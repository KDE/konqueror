#ifndef NETPREF_H
#define NETPREF_H

#define KDE3_SUPPORT
#include <kcmodule.h>
#undef KDE3_SUPPORT

class Q3GroupBox;
class QCheckBox;

class KIntNumInput;

class KIOPreferences : public KCModule
{
    Q_OBJECT

public:
    KIOPreferences(QWidget *parent, const QStringList &args);
    ~KIOPreferences();

    void load();
    void save();
    void defaults();

    QString quickHelp() const;

protected Q_SLOTS:
    void configChanged() { emit changed(true); }

private:
    Q3GroupBox* gb_Ftp;
    Q3GroupBox* gb_Timeout;
    QCheckBox* cb_ftpEnablePasv;
    QCheckBox* cb_ftpMarkPartial;

    KIntNumInput* sb_socketRead;
    KIntNumInput* sb_proxyConnect;
    KIntNumInput* sb_serverConnect;
    KIntNumInput* sb_serverResponse;
};

#endif // NETPREF_H

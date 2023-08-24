/*
    SPDX-FileCopyrightText: 1996 Martin R. Jones
    SPDX-FileCopyrightText: 1998 Bernd Wuebben

    SPDX-FileCopyrightText: 1998 Torben Weis
    KControl port & modifications

    SPDX-FileCopyrightText: 1998 David Faure
    End of the KControl port, added 'kfmclient configure' call.

    SPDX-FileCopyrightText: 2000 Daniel Molkentin
    Cleanup and modifications for KDE 2.1

*/

#ifndef APPEARANCE_H
#define APPEARANCE_H

#include <QWidget>
#include <QMap>

#include <kcmodule.h>
#include <KSharedConfig>

class QSpinBox;
class QSpinBox;
class QFontComboBox;
class QComboBox;
class QCheckBox;
class CSSConfig;

class KAppearanceOptions : public KCModule
{
    Q_OBJECT
public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    KAppearanceOptions(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KAppearanceOptions() override;

    void load() override;
    void save() override;
    void defaults() override;

#if QT_VERSION_MAJOR < 6
    void setNeedsSave(bool needs) {emit changed(needs);}
#endif

public Q_SLOTS:
    void slotFontSize(int);
    void slotMinimumFontSize(int);
    void slotStandardFont(const QFont &n);
    void slotFixedFont(const QFont &n);
    void slotSerifFont(const QFont &n);
    void slotSansSerifFont(const QFont &n);
    void slotCursiveFont(const QFont &n);
    void slotFantasyFont(const QFont &n);
    void slotEncoding(const QString &n);
    void slotFontSizeAdjust(int value);

private:
    void updateGUI();

private:
    CSSConfig *cssConfig;

    QCheckBox *m_pAutoLoadImagesCheckBox;
    QCheckBox *m_pUnfinishedImageFrameCheckBox;
    QComboBox *m_pAnimationsCombo;
    QComboBox *m_pUnderlineCombo;
    QComboBox *m_pSmoothScrollingCombo;

    KSharedConfig::Ptr m_pConfig;
    QString m_groupname;

    QSpinBox *m_minSize;
    QSpinBox *m_MedSize;
    QSpinBox *m_pageDPI;
    QFontComboBox *m_pFonts[6];
    QComboBox *m_pEncoding;
    QSpinBox *m_pFontSizeAdjust;

    int fSize;
    int fMinSize;
    QStringList encodings;
    QStringList fonts;
    QStringList defaultFonts;
    QString encodingName;
};

#endif // APPEARANCE_H

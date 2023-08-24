/* "Misc Options" Tab for KFM configuration

    SPDX-FileCopyrightText: 1998 Sven Radej
    SPDX-FileCopyrightText: 1998 David Faure

*/

#ifndef HTMLOPTS_H
#define HTMLOPTS_H

#include <QCheckBox>

//-----------------------------------------------------------------------------
// The "Misc Options" Tab for the HTML view contains :

// Change cursor over links
// Underline links
// AutoLoad Images
// ... there is room for others :))

#include <kcmodule.h>
#include <ksharedconfig.h>
class QSpinBox;
class QGroupBox;

class KMiscHTMLOptions : public KCModule
{
    Q_OBJECT

public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    KMiscHTMLOptions(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~KMiscHTMLOptions() override;
    void load() override;
    void save() override;
    void defaults() override;

private:
    KSharedConfig::Ptr m_pConfig;
    QString  m_groupname;

    QCheckBox *m_cbCursor;
    QCheckBox *m_pAutoRedirectCheckBox;
    QCheckBox *m_pOpenMiddleClick;
    QCheckBox *m_pBackRightClick;
    QGroupBox *m_pFormCompletionCheckBox;
    QCheckBox *m_pAdvancedAddBookmarkCheckBox;
    QCheckBox *m_pOnlyMarkedBookmarksCheckBox;
    QCheckBox *m_pAccessKeys;
    QCheckBox *m_pDoNotTrack;
    QCheckBox *m_pOfferToSaveWebsitePassword;
    QSpinBox *m_pMaxFormCompletionItems;
    QCheckBox *m_pdfViewer;
};

#endif // HTMLOPTS_H

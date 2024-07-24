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
    KMiscHTMLOptions(QObject *parent, const KPluginMetaData &md={});
    ~KMiscHTMLOptions() override;
    void load() override;
    void save() override;
    void defaults() override;

private:
    /**
     * @brief Loads only the settings from konquerorrc
     *
     * This allows sharing code between lod() and defaults(), using the same code to load settings
     * from konquerorrc and different code for settings from other configuration files
     */
    void loadFromKonquerorrc();

private:
    KSharedConfig::Ptr m_pConfig;
    QString  m_groupname;

    //TODO Settings: uncomment if the corresponding settings can be implemented in WebEnginePart and
    //remove them if they can't
    // QCheckBox *m_pOpenMiddleClick;
    // QCheckBox *m_pBackRightClick;

    //TODO Settings: uncomment when form completion is enabled with WebEnginePart
    //QGroupBox *m_pFormCompletionCheckBox;
    QCheckBox *m_pAdvancedAddBookmarkCheckBox;
    QCheckBox *m_pOnlyMarkedBookmarksCheckBox;
    //TODO Settings: uncomment if the corresponding settings can be implemented in WebEnginePart and
    //remove them if they can't
    // QCheckBox *m_pAccessKeys;
    QCheckBox *m_pDoNotTrack;
    QCheckBox *m_pOfferToSaveWebsitePassword;
    //TODO Settings: uncomment when form completion is enabled with WebEnginePart
    // QSpinBox *m_pMaxFormCompletionItems;
    QCheckBox *m_pdfViewer;
    QCheckBox *m_embedDownloadedNewTab;
};

#endif // HTMLOPTS_H

/* This file is part of the KDE project

    SPDX-FileCopyrightText: 2001, 2003 Lukas Tinkl <lukas@kde.org>
    SPDX-FileCopyrightText: Andreas Schlapbach <schlpbch@iam.unibe.ch>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KONQ_PLUGINS_IMGALLERYDIALOG_H
#define KONQ_PLUGINS_IMGALLERYDIALOG_H

#include <kpagedialog.h>

class QSpinBox;
class QCheckBox;
class QLineEdit;
class KUrlRequester;
class QSpinBox;
class KColorButton;
class KConfig;
class QComboBox;

typedef QMap<QString, QString> CommentMap;

class KIGPDialog : public KPageDialog
{
    Q_OBJECT

public:
    explicit KIGPDialog(QWidget *parent, const QString &path);
    ~KIGPDialog() override;

    bool isDialogOk() const;
    bool printImageName() const;
    bool printImageSize() const;
    bool printImageProperty() const;
    bool copyOriginalFiles() const;
    bool useCommentFile() const;
    bool recurseSubDirectories() const;
    int recursionLevel() const;
    bool colorDepthSet() const;

    int getImagesPerRow() const;
    int getThumbnailSize() const;
    int getColorDepth() const;

    const QString getTitle() const;
    const QUrl getImageUrl() const;
    const QString getCommentFile() const;
    const QString getFontName() const;
    const QString getFontSize() const;

    const QColor getBackgroundColor() const;
    const QColor getForegroundColor() const;

    const QString getImageFormat() const;

    void writeConfig();
protected slots:
    void imageUrlChanged(const QString &);
    void slotDefault();

private:
    KColorButton *m_foregroundColor;
    KColorButton *m_backgroundColor;

    QLineEdit *m_title;
    QString m_path;

    QSpinBox *m_imagesPerRow;
    QSpinBox *m_thumbnailSize;
    QSpinBox *m_recursionLevel;
    QSpinBox *m_fontSize;

    QCheckBox *m_copyOriginalFiles;
    QCheckBox *m_imageName;
    QCheckBox *m_imageSize;
    QCheckBox *m_imageProperty;
    QCheckBox *m_useCommentFile;
    QCheckBox *m_recurseSubDir;
    QCheckBox *m_colorDepthSet;

    QComboBox *m_fontName;
    QComboBox *m_imageFormat;
    QComboBox *m_colorDepth;

    KUrlRequester *m_imageNameReq;
    KUrlRequester *m_commentFileReq;

    KConfig *m_config;

private:
    void setupLookPage(const QString &path);
    void setupDirectoryPage(const QString &path);
    void setupThumbnailPage(const QString &path);
};

#endif // KONQ_PLUGINS_IMGALLERYDIALOG_H

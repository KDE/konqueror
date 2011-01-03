/* This file is part of the KDE project

   Copyright (C) 2001, 2003 Lukas Tinkl <lukas@kde.org>
                      Andreas Schlapbach <schlpbch@iam.unibe.ch>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_PLUGINS_IMGALLERYDIALOG_H
#define KONQ_PLUGINS_IMGALLERYDIALOG_H

#include <kpagedialog.h>

class KUrl;
class KIntNumInput;
class QCheckBox;
class QLineEdit;
class KUrlRequester;
class QSpinBox;
class KColorButton;
class KConfig;
class QComboBox;

typedef QMap<QString,QString> CommentMap;

class KIGPDialog : public KPageDialog
{
 Q_OBJECT

 public:
  explicit KIGPDialog(QWidget *parent, const QString& path );
  ~KIGPDialog();

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
  const KUrl getImageUrl() const;
  const QString getCommentFile() const;
  const QString getFontName() const;
  const QString getFontSize() const;

  const QColor getBackgroundColor() const;
  const QColor getForegroundColor() const;

  const QString getImageFormat() const;

  void writeConfig();
 protected slots:
  void imageUrlChanged(const QString & );
  void slotDefault();

 private:
  KColorButton *m_foregroundColor;
  KColorButton *m_backgroundColor;

  QLineEdit *m_title;
  QString m_path;

  KIntNumInput *m_imagesPerRow;
  KIntNumInput *m_thumbnailSize;
  KIntNumInput *m_recursionLevel;
  QSpinBox *m_fontSize;

  QCheckBox *m_copyOriginalFiles;
  QCheckBox *m_imageName;
  QCheckBox *m_imageSize;
  QCheckBox *m_imageProperty;
  QCheckBox *m_useCommentFile;
  QCheckBox *m_recurseSubDir;
  QCheckBox *m_colorDepthSet;

  QComboBox* m_fontName;
  QComboBox* m_imageFormat;
  QComboBox* m_colorDepth;

  KUrlRequester *m_imageNameReq;
  KUrlRequester *m_commentFileReq;

  KConfig *m_config;

 private:
  void setupLookPage(const QString& path);
  void setupDirectoryPage(const QString& path);
  void setupThumbnailPage(const QString& path);
};

#endif // KONQ_PLUGINS_IMGALLERYDIALOG_H

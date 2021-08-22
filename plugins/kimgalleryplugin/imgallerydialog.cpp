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

#include "imgallerydialog.h"

#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QFormLayout>
#include <QFontDatabase>
#include <QSpinBox>

#include <klocalizedstring.h>
#include <kcolorbutton.h>
#include <kurlrequester.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kguiitem.h>
#include <kfontchooser.h>
#include <kwidgetsaddons_version.h>

KIGPDialog::KIGPDialog(QWidget *parent, const QString &path)
    : KPageDialog(parent)
{
    setStandardButtons(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox()->button(QDialogButtonBox::Ok)->setDefault(true);
    setModal(true);
    setFaceType(List);

    m_path = path;
    setWindowTitle(i18nc("@title:window", "Create Image Gallery"));
    KGuiItem::assign(buttonBox()->button(QDialogButtonBox::Ok), KGuiItem(i18n("Create"), QStringLiteral("imagegallery")));
    m_config = new KConfig(QStringLiteral("kimgallerypluginrc"), KConfig::NoGlobals);
    setupLookPage(path);
    setupDirectoryPage(path);
    setupThumbnailPage(path);
    connect(buttonBox()->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &KIGPDialog::slotDefault);
}

void KIGPDialog::slotDefault()
{
    m_title->setText(i18n("Image Gallery for %1", m_path));
    m_imagesPerRow->setValue(4);
    m_imageName->setChecked(true);
    m_imageSize->setChecked(false);
    m_imageProperty->setChecked(false);
    m_fontName->setItemText(m_fontName->currentIndex(), QFontDatabase::systemFont(QFontDatabase::GeneralFont).family());
    m_fontSize->setValue(14);
    m_foregroundColor->setColor(QColor(QStringLiteral("#d0ffd0")));
    m_backgroundColor->setColor(QColor(QStringLiteral("#333333")));

    m_imageNameReq->setUrl(QUrl::fromLocalFile(m_path + "images.html"));
    m_recurseSubDir->setChecked(false);
    m_recursionLevel->setEnabled(false);
    m_recursionLevel->setValue(0);
    m_copyOriginalFiles->setChecked(false);
    m_useCommentFile->setChecked(false);
    m_commentFileReq->setUrl(QUrl::fromLocalFile(m_path + "comments"));
    m_commentFileReq->setEnabled(false);

    m_imageFormat->setItemText(m_imageFormat->currentIndex(), QStringLiteral("JPEG"));
    m_thumbnailSize->setValue(140);
    m_colorDepthSet->setChecked(false);
    m_colorDepth->setItemText(m_colorDepth->currentIndex(), QStringLiteral("8"));
}

void KIGPDialog::setupLookPage(const QString &path)
{
    QFrame *page = new QFrame();
    KPageWidgetItem *pageItem = new KPageWidgetItem(page, i18n("Look"));
    pageItem->setHeader(i18n("Page Look"));
    pageItem->setIcon(QIcon::fromTheme("fill-color"));
    addPage(pageItem);

    KConfigGroup look = m_config->group("Look");
    QFormLayout *form = new QFormLayout(page);

    m_title = new QLineEdit(i18n("Image Gallery for %1", path), page);
    form->addRow(i18n("&Page title:"), m_title);

    m_imagesPerRow = new QSpinBox(page);
    m_imagesPerRow->setMinimumWidth(100);
    m_imagesPerRow->setRange(1, 8);
    m_imagesPerRow->setSingleStep(1);
    m_imagesPerRow->setValue(look.readEntry("ImagesPerRow", 4));
    //m_imagesPerRow->setSliderEnabled(true);
    form->addRow(i18n("I&mages per row:"), m_imagesPerRow);

    m_imageName = new QCheckBox(i18n("Show image file &name"), page);
    m_imageName->setChecked(look.readEntry("ImageName", true));
    form->addRow(QString(), m_imageName);

    m_imageSize = new QCheckBox(i18n("Show image file &size"), page);
    m_imageSize->setChecked(look.readEntry("ImageSize", false));
    form->addRow(QString(), m_imageSize);

    m_imageProperty = new QCheckBox(i18n("Show image &dimensions"), page);
    m_imageProperty->setChecked(look.readEntry("ImageProperty", false));
    form->addRow(QString(), m_imageProperty);

    m_fontName = new QComboBox(page);
    m_fontName->setSizePolicy(QSizePolicy::MinimumExpanding, m_fontName->sizePolicy().verticalPolicy());

#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 86, 0)
    const QStringList standardFonts = KFontChooser::createFontList(KFontChooser::NoDisplayFlags);
#else
    QStringList standardFonts;
    KFontChooser::getFontList(standardFonts, 0);
#endif

    m_fontName->addItems(standardFonts);
    m_fontName->setItemText(m_fontName->currentIndex(), look.readEntry("FontName", QFontDatabase::systemFont(QFontDatabase::GeneralFont).family()));
    form->addRow(i18n("Fon&t name:"), m_fontName);

    m_fontSize = new QSpinBox(page);
    m_fontSize->setMinimumWidth(100);
    m_fontSize->setMinimum(6);
    m_fontSize->setMaximum(15);
    m_fontSize->setSingleStep(1);
    m_fontSize->setValue(look.readEntry("FontSize", 14));
    form->addRow(i18n("Font si&ze:"), m_fontSize);

    m_foregroundColor = new KColorButton(page);
    m_foregroundColor->setColor(QColor(look.readEntry("ForegroundColor", "#d0ffd0")));
    form->addRow(i18n("&Foreground color:"), m_foregroundColor);

    m_backgroundColor = new KColorButton(page);
    m_backgroundColor->setColor(QColor(look.readEntry("BackgroundColor", "#333333")));
    form->addRow(i18n("&Background color:"), m_backgroundColor);
}

void KIGPDialog::setupDirectoryPage(const QString &path)
{
    QFrame *page = new QFrame();
    KPageWidgetItem *pageItem = new KPageWidgetItem(page, i18n("Folders"));
    pageItem->setHeader(i18n("Folders"));
    pageItem->setIcon(QIcon::fromTheme("folder"));
    addPage(pageItem);

    KConfigGroup group =  m_config->group("Directory");
    QFormLayout *form = new QFormLayout(page);

    m_imageNameReq = new KUrlRequester(QUrl::fromLocalFile(QString(path + "images.html")), page);
    form->addRow(i18n("&Save to HTML file:"), m_imageNameReq);
    connect(m_imageNameReq, &KUrlRequester::textChanged, this, &KIGPDialog::imageUrlChanged);
    m_imageNameReq->setToolTip(i18n("<p>The name of the HTML file this gallery will be saved to.</p>"));

    const bool recurseSubDir = group.readEntry("RecurseSubDirectories", false);
    m_recurseSubDir = new QCheckBox(i18n("&Recurse subfolders"), page);
    m_recurseSubDir->setChecked(recurseSubDir);
    m_recurseSubDir->setToolTip(i18n("<p>Whether subfolders should be included for the "
                                       "image gallery creation or not.</p>"));
    form->addRow(QString(), m_recurseSubDir);

    const int recursionLevel = group.readEntry("RecursionLevel", 0);
    m_recursionLevel = new QSpinBox(page);
    m_recursionLevel->setMinimumWidth(100);
    m_recursionLevel->setRange(0, 99);
    m_recursionLevel->setSingleStep(1);
    m_recursionLevel->setValue(recursionLevel);
    //m_recursionLevel->setSliderEnabled(true);
    m_recursionLevel->setSpecialValueText(i18n("Endless"));
    m_recursionLevel->setEnabled(recurseSubDir);
    m_recursionLevel->setToolTip(i18n("<p>You can limit the number of folders the "
                                        "image gallery creator will traverse to by setting an "
                                        "upper bound for the recursion depth.</p>"));
    form->addRow(i18n("Rec&ursion depth:"), m_recursionLevel);

    connect(m_recurseSubDir, &QAbstractButton::toggled, m_recursionLevel, &QWidget::setEnabled);

    m_copyOriginalFiles = new QCheckBox(i18n("Copy or&iginal files"), page);
    m_copyOriginalFiles->setChecked(group.readEntry("CopyOriginalFiles", false));
    m_copyOriginalFiles->setToolTip(i18n("<p>This makes a copy of all images and the gallery will refer "
                                           "to these copies instead of the original images.</p>"));
    form->addRow(QString(), m_copyOriginalFiles);

    const bool useCommentFile = group.readEntry("UseCommentFile", false);
    m_useCommentFile = new QCheckBox(i18n("Use &comment file"), page);
    m_useCommentFile->setChecked(useCommentFile);
    m_useCommentFile->setToolTip(i18n("<p>If you enable this option you can specify "
                                        "a comment file which will be used for generating "
                                        "subtitles for the images.</p>"
                                        "<p>For details about the file format please see "
                                        "the \"What's This?\" help below.</p>"));
    form->addRow(QString(), m_useCommentFile);

    m_commentFileReq = new KUrlRequester(QUrl::fromLocalFile(QString(path + "comments")), page);
    m_commentFileReq->setEnabled(useCommentFile);
    m_commentFileReq->setToolTip(i18n("<p>You can specify the name of the comment file here. "
                                        "The comment file contains the subtitles for the images. "
                                        "The format of this file is:</p>"
                                        "<p>FILENAME1:"
                                        "<br />Description"
                                        "<br />"
                                        "<br />FILENAME2:"
                                        "<br />Description"
                                        "<br />"
                                        "<br />and so on</p>"));
    form->addRow(i18n("Comments &file:"), m_commentFileReq);

    connect(m_useCommentFile, &QAbstractButton::toggled, m_commentFileReq, &QWidget::setEnabled);
}

void KIGPDialog::setupThumbnailPage(const QString &path)
{
    Q_UNUSED(path);

    QFrame *page = new QFrame();
    KPageWidgetItem *pageItem = new KPageWidgetItem(page, i18n("Thumbnails"));
    pageItem->setHeader(i18n("Thumbnails"));
    pageItem->setIcon(QIcon::fromTheme("view-preview"));
    addPage(pageItem);

    KConfigGroup group =  m_config->group("Thumbnails");
    QFormLayout *form = new QFormLayout(page);

    m_imageFormat = new QComboBox(page);
    m_imageFormat->setSizePolicy(QSizePolicy::MinimumExpanding, m_imageFormat->sizePolicy().verticalPolicy());
    QStringList lstImgageFormat;
    lstImgageFormat << QStringLiteral("JPEG") << QStringLiteral("PNG");
    m_imageFormat->addItems(lstImgageFormat);
    m_imageFormat->setItemText(m_imageFormat->currentIndex(), group.readEntry("ImageFormat", "JPEG"));
    form->addRow(i18n("Image f&ormat:"), m_imageFormat);

    m_thumbnailSize = new QSpinBox(page);
    m_thumbnailSize->setMinimumWidth(100);
    m_thumbnailSize->setRange(10, 1000);
    m_thumbnailSize->setSingleStep(1);
    m_thumbnailSize->setValue(group.readEntry("ThumbnailSize", 140));
    //m_thumbnailSize->setSliderEnabled(true);
    form->addRow(i18n("Thumbnail size:"), m_thumbnailSize);

    const bool colorDepthSet = group.readEntry("ColorDepthSet", false);
    m_colorDepthSet = new QCheckBox(i18n("&Set different color depth:"), page);
    m_colorDepthSet->setChecked(colorDepthSet);
    form->addRow(QString(), m_colorDepthSet);

    m_colorDepth = new QComboBox(page);
    m_colorDepth->setSizePolicy(QSizePolicy::MinimumExpanding, m_colorDepth->sizePolicy().verticalPolicy());
    QStringList lst;
    lst << QStringLiteral("1") << QStringLiteral("8") << QStringLiteral("16") << QStringLiteral("32");
    m_colorDepth->addItems(lst);
    m_colorDepth->setCurrentIndex(m_colorDepth->findText(group.readEntry("ColorDepth", "8")));
    m_colorDepth->setEnabled(colorDepthSet);
    form->addRow(i18n("Color depth:"), m_colorDepth);

    connect(m_colorDepthSet, &QAbstractButton::toggled, m_colorDepth, &QWidget::setEnabled);
}

void KIGPDialog::writeConfig()
{
    KConfigGroup group = m_config->group("Look");
    group.writeEntry("ImagesPerRow", getImagesPerRow());
    group.writeEntry("ImageName", printImageName());
    group.writeEntry("ImageSize", printImageSize());
    group.writeEntry("ImageProperty", printImageProperty());
    group.writeEntry("FontName", getFontName());
    group.writeEntry("FontSize", getFontSize());
    group.writeEntry("ForegroundColor", getForegroundColor().name());
    group.writeEntry("BackgroundColor", getBackgroundColor().name());

    group = m_config->group("Directory");
    group.writeEntry("RecurseSubDirectories", recurseSubDirectories());
    group.writeEntry("RecursionLevel", recursionLevel());
    group.writeEntry("CopyOriginalFiles", copyOriginalFiles());
    group.writeEntry("UseCommentFile", useCommentFile());

    group = m_config->group("Thumbnails");
    group.writeEntry("ThumbnailSize", getThumbnailSize());
    group.writeEntry("ColorDepth", getColorDepth());
    group.writeEntry("ColorDepthSet", colorDepthSet());
    group.writeEntry("ImageFormat", getImageFormat());
    group.sync();
}

KIGPDialog::~KIGPDialog()
{
    delete m_config;
}

void KIGPDialog::imageUrlChanged(const QString &url)
{
    buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(!url.isEmpty());
}

bool  KIGPDialog::printImageName()  const
{
    return m_imageName->isChecked();
}

bool  KIGPDialog::printImageSize() const
{
    return m_imageSize->isChecked();
}

bool  KIGPDialog::printImageProperty() const
{
    return m_imageProperty->isChecked();
}

bool KIGPDialog::recurseSubDirectories() const
{
    return m_recurseSubDir->isChecked();
}

int KIGPDialog::recursionLevel() const
{
    return m_recursionLevel->value();
}

bool KIGPDialog::copyOriginalFiles() const
{
    return m_copyOriginalFiles->isChecked();
}

bool KIGPDialog::useCommentFile() const
{
    return m_useCommentFile->isChecked();
}

int KIGPDialog::getImagesPerRow() const
{
    return m_imagesPerRow->value();
}

int KIGPDialog::getThumbnailSize() const
{
    return m_thumbnailSize->value();
}

int KIGPDialog::getColorDepth() const
{
    return m_colorDepth->currentText().toInt();
}

bool KIGPDialog::colorDepthSet() const
{
    return m_colorDepthSet->isChecked();
}

const QString KIGPDialog::getTitle() const
{
    return m_title->text();
}

const QUrl KIGPDialog::getImageUrl() const
{
    return m_imageNameReq->url();
}

const QString KIGPDialog::getCommentFile() const
{
    return m_commentFileReq->url().toLocalFile();
}

const QString KIGPDialog::getFontName() const
{
    return m_fontName->currentText();
}

const QString KIGPDialog::getFontSize() const
{
    return m_fontSize->text();
}

const QColor KIGPDialog::getBackgroundColor() const
{
    return m_backgroundColor->color();
}

const QColor KIGPDialog::getForegroundColor() const
{
    return m_foregroundColor->color();
}

const QString KIGPDialog::getImageFormat() const
{
    return m_imageFormat->currentText();
}

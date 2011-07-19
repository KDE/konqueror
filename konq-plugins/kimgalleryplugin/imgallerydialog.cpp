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

#include <qlabel.h>
#include <qlayout.h>
#include <qcombobox.h>

#include <qcheckbox.h>
#include <qlineedit.h>
#include <qspinbox.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>


#include <klocale.h>
#include <kurl.h>
#include <kfontdialog.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kcolorbutton.h>
#include <kurlrequester.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <kicon.h>
#include "imgallerydialog.h"
#include "imgallerydialog.moc"

KIGPDialog::KIGPDialog(QWidget *parent, const QString& path )
    : KPageDialog( parent)
{
    setCaption(i18nc("@title:window", "Configure"));
    setButtons(Default|Ok|Cancel);
    setDefaultButton(Ok);
    setModal(true);
    showButtonSeparator(true);
    setFaceType(List);

    m_path = path;
    setCaption(i18nc("@title:window", "Create Image Gallery"));
    setButtonGuiItem( KDialog::Ok, KGuiItem(i18n("Create"),"imagegallery") );
    m_config = new KConfig("kimgallerypluginrc", KConfig::NoGlobals);
    setupLookPage(path);
    setupDirectoryPage(path);
    setupThumbnailPage(path);
    connect(this,SIGNAL(defaultClicked()),this,SLOT(slotDefault()));
}

void KIGPDialog::slotDefault()
{
    m_title->setText(i18n("Image Gallery for %1", m_path));
    m_imagesPerRow->setValue(4);
    m_imageName->setChecked(true);
    m_imageSize->setChecked(false);
    m_imageProperty->setChecked(false);
    m_fontName->setItemText(m_fontName->currentIndex(), KGlobalSettings::generalFont().family() );
    m_fontSize->setValue(14);
    m_foregroundColor->setColor( QColor( "#d0ffd0") );
    m_backgroundColor->setColor( QColor("#333333") );

    m_imageNameReq->setUrl(QString(m_path + "images.html"));
    m_recurseSubDir->setChecked( false );
    m_recursionLevel->setEnabled( false );
    m_recursionLevel->setValue(0);
    m_copyOriginalFiles->setChecked( false );
    m_useCommentFile->setChecked( false );
    m_commentFileReq->setUrl(QString(m_path + "comments"));
    m_commentFileReq->setEnabled( false );

    m_imageFormat->setItemText(m_imageFormat->currentIndex(), "JPEG");
    m_thumbnailSize->setValue(140);
    m_colorDepthSet->setChecked(false);
    m_colorDepth->setItemText(m_colorDepth->currentIndex(), "8");
}

void KIGPDialog::setupLookPage(const QString& path) {
    QFrame *page = new QFrame();
	KPageWidgetItem *pageItem = new KPageWidgetItem( page, i18n("Look") );
	pageItem->setHeader(i18n("Page Look"));
	pageItem->setIcon(KIcon(BarIcon("fill-color", KIconLoader::SizeMedium )) );
	addPage(pageItem);

        KConfigGroup look = m_config->group("Look");
    QVBoxLayout *vlay = new QVBoxLayout( page );
    vlay->setMargin( 0 );

    QLabel *label;

    label = new QLabel( i18n("&Page title:"), page);
    vlay->addWidget(label);

    m_title = new QLineEdit(i18n("Image Gallery for %1", path), page);
    vlay->addWidget( m_title );
    label->setBuddy(m_title);

    m_imagesPerRow = new KIntNumInput(look.readEntry("ImagesPerRow", 4), page);
    m_imagesPerRow->setRange( 1, 8, 1 );
    m_imagesPerRow->setSliderEnabled( true );
    m_imagesPerRow->setLabel( i18n("I&mages per row:") );
    vlay->addWidget( m_imagesPerRow );

    QGridLayout *grid = new QGridLayout();
    grid->setMargin( 2 );
    grid->setSpacing( 2 );
    vlay->addLayout( grid );

    m_imageName = new QCheckBox( i18n("Show image file &name"), page);
    m_imageName->setChecked( look.readEntry("ImageName", true) );
    grid->addWidget( m_imageName, 0, 0 );

    m_imageSize = new QCheckBox( i18n("Show image file &size"), page);
    m_imageSize->setChecked( look.readEntry("ImageSize", false) );
    grid->addWidget( m_imageSize, 0, 1 );

    m_imageProperty = new QCheckBox( i18n("Show image &dimensions"), page);
    m_imageProperty->setChecked( look.readEntry("ImageProperty", false) );
    grid->addWidget( m_imageProperty, 1, 0 );

    QHBoxLayout *hlay11  = new QHBoxLayout( );
    vlay->addLayout( hlay11 );

    m_fontName = new QComboBox( page );
    QStringList standardFonts;
    KFontChooser::getFontList(standardFonts, 0);
    m_fontName->addItems( standardFonts );
    m_fontName->setItemText( m_fontName->currentIndex(), look.readEntry("FontName", KGlobalSettings::generalFont().family() ) );

    label = new QLabel( i18n("Fon&t name:"), page );
    label->setBuddy( m_fontName );
    hlay11->addWidget( label );
    hlay11->addStretch( 1 );
    hlay11->addWidget( m_fontName );

    QHBoxLayout *hlay12  = new QHBoxLayout( );
    vlay->addLayout( hlay12 );

    m_fontSize = new QSpinBox( page );
    m_fontSize->setMinimum(6);
    m_fontSize->setMaximum(15);
    m_fontSize->setSingleStep(1);
    m_fontSize->setValue( look.readEntry("FontSize", 14) );

    label = new QLabel( i18n("Font si&ze:"), page );
    label->setBuddy( m_fontSize );
    hlay12->addWidget( label );
    hlay12->addStretch( 1 );
    hlay12->addWidget( m_fontSize );

    QHBoxLayout *hlay1  = new QHBoxLayout();
    vlay->addLayout( hlay1 );

    m_foregroundColor = new KColorButton(page);
    m_foregroundColor->setColor( QColor( look.readEntry("ForegroundColor", "#d0ffd0") ) );

    label = new QLabel( i18n("&Foreground color:"), page);
    label->setBuddy( m_foregroundColor );
    hlay1->addWidget( label );
    hlay1->addStretch( 1 );
    hlay1->addWidget(m_foregroundColor);

    QHBoxLayout *hlay2 = new QHBoxLayout();
    vlay->addLayout( hlay2 );

    m_backgroundColor = new KColorButton(page);
    m_backgroundColor->setColor( QColor(look.readEntry("BackgroundColor", "#333333") ) );

    label = new QLabel( i18n("&Background color:"), page);
    hlay2->addWidget( label );
    label->setBuddy( m_backgroundColor );
    hlay2->addStretch( 1 );
    hlay2->addWidget(m_backgroundColor);

    vlay->addStretch(1);
}

void KIGPDialog::setupDirectoryPage(const QString& path) {
    QFrame *page = new QFrame();
    KPageWidgetItem *pageItem = new KPageWidgetItem( page, i18n("Folders") );
    pageItem->setHeader(i18n("Folders"));
    pageItem->setIcon(KIcon(BarIcon("folder", KIconLoader::SizeMedium )) );
    addPage(pageItem);

    KConfigGroup group =  m_config->group("Directory");
    QVBoxLayout *dvlay = new QVBoxLayout( page );
    dvlay->setMargin( 0 );

    QLabel *label;
    label = new QLabel(i18n("&Save to HTML file:"), page);
    dvlay->addWidget( label );
    QString whatsThis;
    whatsThis = i18n("<p>The name of the HTML file this gallery will be saved to.</p>");
    label->setWhatsThis( whatsThis );

    m_imageNameReq = new KUrlRequester(QString(path + "images.html"), page);
    label->setBuddy( m_imageNameReq );
    dvlay->addWidget(m_imageNameReq);
    connect( m_imageNameReq, SIGNAL(textChanged(const QString&)),
             this, SLOT(imageUrlChanged(const QString&)) );
    m_imageNameReq->setWhatsThis( whatsThis );

    const bool recurseSubDir = group.readEntry("RecurseSubDirectories", false);
    m_recurseSubDir = new QCheckBox(i18n("&Recurse subfolders"), page);
    m_recurseSubDir->setChecked( recurseSubDir );
    whatsThis = i18n("<p>Whether subfolders should be included for the "
                     "image gallery creation or not.</p>");
    m_recurseSubDir->setWhatsThis( whatsThis );

    const int recursionLevel = group.readEntry("RecursionLevel", 0);
    m_recursionLevel = new KIntNumInput( recursionLevel, page );
    m_recursionLevel->setRange( 0, 99, 1);
    m_recursionLevel->setSliderEnabled(true);
    m_recursionLevel->setLabel( i18n("Rec&ursion depth:") );
    if ( recursionLevel == 0 )
      m_recursionLevel->setSpecialValueText( i18n("Endless"));
    m_recursionLevel->setEnabled(recurseSubDir);
    whatsThis = i18n("<p>You can limit the number of folders the "
                     "image gallery creator will traverse to by setting an "
                     "upper bound for the recursion depth.</p>");
    m_recursionLevel->setWhatsThis( whatsThis );


    connect(m_recurseSubDir, SIGNAL( toggled(bool) ),
            m_recursionLevel, SLOT( setEnabled(bool) ) );

    dvlay->addWidget(m_recurseSubDir);
    dvlay->addWidget(m_recursionLevel);

    m_copyOriginalFiles = new QCheckBox(i18n("Copy or&iginal files"), page);
    m_copyOriginalFiles->setChecked(group.readEntry("CopyOriginalFiles", false) );
    dvlay->addWidget(m_copyOriginalFiles);
    whatsThis = i18n("<p>This makes a copy of all images and the gallery will refer "
                     "to these copies instead of the original images.</p>");
    m_copyOriginalFiles->setWhatsThis( whatsThis );


    const bool useCommentFile = group.readEntry("UseCommentFile", false);
    m_useCommentFile = new QCheckBox(i18n("Use &comment file"), page);
    m_useCommentFile->setChecked(useCommentFile);
    dvlay->addWidget(m_useCommentFile);

    whatsThis = i18n("<p>If you enable this option you can specify "
                     "a comment file which will be used for generating "
                     "subtitles for the images.</p>"
                     "<p>For details about the file format please see "
                     "the \"What's This?\" help below.</p>");
    m_useCommentFile->setWhatsThis( whatsThis );

    label = new QLabel(i18n("Comments &file:"), page);
    label->setEnabled( useCommentFile );
    dvlay->addWidget( label );
    whatsThis = i18n("<p>You can specify the name of the comment file here. "
                     "The comment file contains the subtitles for the images. "
                     "The format of this file is:</p>"
                     "<p>FILENAME1:"
                     "<br />Description"
                     "<br />"
                     "<br />FILENAME2:"
                     "<br />Description"
                     "<br />"
                     "<br />and so on</p>");
    label->setWhatsThis( whatsThis );

    m_commentFileReq = new KUrlRequester(QString(path + "comments"), page);
    m_commentFileReq->setEnabled(useCommentFile);
    label->setBuddy( m_commentFileReq );
    dvlay->addWidget(m_commentFileReq);
    m_commentFileReq->setWhatsThis( whatsThis );

    connect(m_useCommentFile, SIGNAL(toggled(bool)),
            label, SLOT(setEnabled(bool)));
    connect(m_useCommentFile, SIGNAL(toggled(bool)),
            m_commentFileReq, SLOT(setEnabled(bool)));

    dvlay->addStretch(1);
}

void KIGPDialog::setupThumbnailPage(const QString& path) {
    Q_UNUSED (path);

    QFrame *page = new QFrame();
    KPageWidgetItem *pageItem = new KPageWidgetItem( page, i18n("Thumbnails") );
    pageItem->setHeader(i18n("Thumbnails"));
    pageItem->setIcon(KIcon(BarIcon("view-preview", KIconLoader::SizeMedium )) );
    addPage(pageItem);

    KConfigGroup group =  m_config->group("Thumbnails");
    QLabel *label;

    QVBoxLayout *vlay = new QVBoxLayout( page );
    vlay->setMargin( 0 );

    QHBoxLayout *hlay3 = new QHBoxLayout();
    vlay->addLayout( hlay3 );

    m_imageFormat = new QComboBox(page);
    QStringList lstImgageFormat;
    lstImgageFormat<<"JPEG"<<"PNG";
    m_imageFormat->addItems(lstImgageFormat);
    m_imageFormat->setItemText( m_imageFormat->currentIndex(), group.readEntry("ImageFormat", "JPEG") );

    label = new QLabel( i18n("Image format f&or the thumbnails:"), page);
    hlay3->addWidget( label );
    label->setBuddy( m_imageFormat );
    hlay3->addStretch( 1 );
    hlay3->addWidget(m_imageFormat);

    m_thumbnailSize = new KIntNumInput(group.readEntry("ThumbnailSize", 140), page);
    m_thumbnailSize->setRange(10, 1000, 1 );
    m_thumbnailSize->setLabel( i18n("Thumbnail size:") );
    m_thumbnailSize->setSliderEnabled(true);
    vlay->addWidget( m_thumbnailSize );

    QGridLayout *grid = new QGridLayout();
    grid->setMargin( 2 );
    grid->setSpacing( 2 );
    vlay->addLayout( grid );

    QHBoxLayout *hlay4 = new QHBoxLayout();
    vlay->addLayout( hlay4 );
    const bool colorDepthSet = group.readEntry("ColorDepthSet", false);
    m_colorDepthSet = new QCheckBox(i18n("&Set different color depth:"), page);
    m_colorDepthSet->setChecked(colorDepthSet);
    hlay4->addWidget( m_colorDepthSet );

    m_colorDepth = new QComboBox(page);
    QStringList lst;
    lst<<"1"<<"8"<<"16"<<"32";
    m_colorDepth->addItems( lst );
    m_colorDepth->setItemText(m_colorDepth->currentIndex(), group.readEntry("ColorDepth", "8"));
    m_colorDepth->setEnabled(colorDepthSet);
    hlay4->addWidget( m_colorDepth );

    connect(m_colorDepthSet, SIGNAL( toggled(bool) ),
            m_colorDepth, SLOT( setEnabled(bool) ) );

    vlay->addStretch(1);

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
 group.writeEntry("ForegroundColor", getForegroundColor().name() );
 group.writeEntry("BackgroundColor", getBackgroundColor().name());

 group =m_config->group("Directory");
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

void KIGPDialog::imageUrlChanged(const QString &url )
{
    enableButtonOk( !url.isEmpty());
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

const KUrl KIGPDialog::getImageUrl() const
{
    return m_imageNameReq->url();
}

const QString KIGPDialog::getCommentFile() const
{
    return m_commentFileReq->url().path();
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

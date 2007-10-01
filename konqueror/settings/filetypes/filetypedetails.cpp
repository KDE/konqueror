
// Own
#include "filetypedetails.h"

// Qt
#include <QtGui/QBoxLayout>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLayout>
#include <QtGui/QListWidget>
#include <QtGui/QRadioButton>
#include <fixx11h.h>

// KDE
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kicondialog.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <klocale.h>

// Local
#include "kservicelistwidget.h"
#include "typeslistitem.h"

FileTypeDetails::FileTypeDetails( QWidget * parent )
  : QTabWidget( parent ), m_item( 0L )
{
  QString wtstr;
  // First tab - General
  QWidget * firstWidget = new QWidget(this);
  QVBoxLayout *firstLayout = new QVBoxLayout(firstWidget);
  firstLayout->setMargin(KDialog::marginHint());
  firstLayout->setSpacing(KDialog::spacingHint());

  QHBoxLayout *hBox = new QHBoxLayout((QWidget*)0);
  hBox->setSpacing(KDialog::spacingHint());
  firstLayout->addLayout(hBox, 1);

  iconButton = new KIconButton(firstWidget);
  iconButton->setIconType(KIconLoader::Desktop, KIconLoader::MimeType);
  connect(iconButton, SIGNAL(iconChanged(QString)), SLOT(updateIcon(QString)));

  iconButton->setFixedSize(70, 70);
  hBox->addWidget(iconButton);

  iconButton->setWhatsThis( i18n("This button displays the icon associated"
    " with the selected file type. Click on it to choose a different icon.") );

  QGroupBox *gb = new QGroupBox(i18n("Filename Patterns"), firstWidget);
  hBox->addWidget(gb);

  QGridLayout *grid = new QGridLayout(gb);
  grid->addItem(new QSpacerItem(0,fontMetrics().lineSpacing()), 0, 0);

  extensionLB = new QListWidget(gb);
  connect(extensionLB, SIGNAL(itemSelectionChanged()), SLOT(enableExtButtons()));
  grid->addWidget(extensionLB, 1, 0, 2, 1);
  grid->setRowStretch(0, 0);
  grid->setRowStretch(1, 1);
  grid->setRowStretch(2, 0);

  extensionLB->setWhatsThis( i18n("This box contains a list of patterns that can be"
    " used to identify files of the selected type. For example, the pattern *.txt is"
    " associated with the file type 'text/plain'; all files ending in '.txt' are recognized"
    " as plain text files.") );

  addExtButton = new QPushButton(i18n("Add..."), gb);
  addExtButton->setEnabled(false);
  connect(addExtButton, SIGNAL(clicked()),
          this, SLOT(addExtension()));
  grid->addWidget(addExtButton, 1, 1);

  addExtButton->setWhatsThis( i18n("Add a new pattern for the selected file type.") );

  removeExtButton = new QPushButton(i18n("Remove"), gb);
  removeExtButton->setEnabled(false);
  connect(removeExtButton, SIGNAL(clicked()),
          this, SLOT(removeExtension()));
  grid->addWidget(removeExtButton, 2, 1);

  removeExtButton->setWhatsThis( i18n("Remove the selected filename pattern.") );

  gb = new QGroupBox(i18n("Description"), firstWidget);
  firstLayout->addWidget(gb);

  description = new KLineEdit(gb);
  connect(description, SIGNAL(textChanged(const QString &)),
          SLOT(updateDescription(const QString &)));

  QVBoxLayout *descriptionBox = new QVBoxLayout;
  descriptionBox->addWidget(description);
  gb->setLayout(descriptionBox);

  wtstr = i18n("You can enter a short description for files of the selected"
    " file type (e.g. 'HTML Page'). This description will be used by applications"
    " like Konqueror to display directory content.");
  gb->setWhatsThis( wtstr );
  description->setWhatsThis( wtstr );

  serviceListWidget = new KServiceListWidget( KServiceListWidget::SERVICELIST_APPLICATIONS, firstWidget );
  connect( serviceListWidget, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  firstLayout->addWidget(serviceListWidget, 5);

  // Second tab - Embedding
  QWidget * secondWidget = new QWidget(this);
  QVBoxLayout *secondLayout = new QVBoxLayout(secondWidget);
  secondLayout->setMargin(KDialog::marginHint());
  secondLayout->setSpacing(KDialog::spacingHint());

  m_autoEmbedBox = new QGroupBox( i18n("Left Click Action"), secondWidget );
  secondLayout->setSpacing( KDialog::spacingHint() );
  secondLayout->addWidget( m_autoEmbedBox, 1 );

  m_autoEmbedBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

  QRadioButton *embViewerRadio = new QRadioButton( i18n("Show file in embedded viewer") );
  QRadioButton *sepViewerRadio = new QRadioButton( i18n("Show file in separate viewer") );
  QButtonGroup *buttonGroup = new QButtonGroup;
  buttonGroup->addButton(embViewerRadio,0);
  buttonGroup->addButton( sepViewerRadio,1);
  m_rbGroupSettings = new QRadioButton( QString("Use settings for '%1' group") );
  buttonGroup->addButton(m_rbGroupSettings,2);
  connect(buttonGroup, SIGNAL( buttonClicked( int ) ), SLOT( slotAutoEmbedClicked( int ) ));

  m_chkAskSave = new QCheckBox( i18n("Ask whether to save to disk instead") );
  connect(m_chkAskSave, SIGNAL( toggled(bool) ), SLOT( slotAskSaveToggled(bool) ));

  m_autoEmbedGroup = new QButtonGroup(this);
  m_autoEmbedGroup->addButton(embViewerRadio, 0);
  m_autoEmbedGroup->addButton(sepViewerRadio, 1);
  m_autoEmbedGroup->addButton(m_rbGroupSettings, 2);

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(embViewerRadio);
  vbox->addWidget(sepViewerRadio);
  vbox->addWidget(m_rbGroupSettings);
  vbox->addWidget(m_chkAskSave);
  m_autoEmbedBox->setLayout(vbox);

  m_autoEmbedBox->setWhatsThis( i18n("Here you can configure what the Konqueror file manager"
    " will do when you click on a file of this type. Konqueror can display the file in"
    " an embedded viewer or start up a separate application. If set to 'Use settings for G group',"
    " Konqueror will behave according to the settings of the group G this type belongs to,"
    " for instance 'image' if the current file type is image/png.") );

  secondLayout->addSpacing(10);

  embedServiceListWidget = new KServiceListWidget( KServiceListWidget::SERVICELIST_SERVICES, secondWidget );
  embedServiceListWidget->setMinimumHeight( serviceListWidget->sizeHint().height() );
  connect( embedServiceListWidget, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  secondLayout->addWidget(embedServiceListWidget, 3);

  addTab( firstWidget, i18n("&General") );
  addTab( secondWidget, i18n("&Embedding") );
}

void FileTypeDetails::updateRemoveButton()
{
    removeExtButton->setEnabled(extensionLB->count()>0);
}

void FileTypeDetails::updateIcon(const QString &icon)
{
  if (!m_item)
    return;

  m_item->setIcon(icon);

  emit changed(true);
}

void FileTypeDetails::updateDescription(const QString &desc)
{
  if (!m_item)
    return;

  m_item->setComment(desc);

  emit changed(true);
}

void FileTypeDetails::addExtension()
{
  if ( !m_item )
    return;

  bool ok;
  QString ext = KInputDialog::getText( i18n( "Add New Extension" ),
    i18n( "Extension:" ), "*.", &ok, this );
  if (ok) {
    extensionLB->addItem(ext);
    QStringList patt = m_item->patterns();
    patt += ext;
    m_item->setPatterns(patt);
    updateRemoveButton();
    emit changed(true);
  }
}

void FileTypeDetails::removeExtension()
{
  if (extensionLB->currentRow() == -1)
    return;
  if ( !m_item )
    return;
  QStringList patt = m_item->patterns();
  patt.removeAll(extensionLB->currentItem()->text());
  m_item->setPatterns(patt);
  delete extensionLB->takeItem(extensionLB->currentRow());
  updateRemoveButton();
  emit changed(true);
}

void FileTypeDetails::slotAutoEmbedClicked( int button )
{
  if ( !m_item || (button > 2))
    return;

  m_item->setAutoEmbed( button );

  updateAskSave();

  emit changed(true);
}

void FileTypeDetails::updateAskSave()
{
  if ( !m_item )
    return;

  int button = m_item->autoEmbed();
  if (button == 2)
  {
    bool embedParent = TypesListItem::defaultEmbeddingSetting(m_item->majorType());
    emit embedMajor(m_item->majorType(), embedParent);
    button = embedParent ? 0 : 1;
  }

  QString mimeType = m_item->name();

  QString dontAskAgainName;

  if (button == 0) // Embedded
    dontAskAgainName = "askEmbedOrSave"+mimeType;
  else
    dontAskAgainName = "askSave"+mimeType;

  KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
  bool ask = config->group("Notification Messages").readEntry(dontAskAgainName, QString()).isEmpty();
  m_item->getAskSave(ask);

  bool neverAsk = false;

  if (button == 0)
  {
    KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
    // Don't ask for:
    // - html (even new tabs would ask, due to about:blank!)
    // - dirs obviously (though not common over HTTP :),
    // - images (reasoning: no need to save, most of the time, because fast to see)
    // e.g. postscript is different, because takes longer to read, so
    // it's more likely that the user might want to save it.
    // - multipart/* ("server push", see kmultipart)
    // - other strange 'internal' mimetypes like print/manager...
    if ( mime->is( "text/html" ) ||
         mime->is( "application/xml" ) ||
         mime->is( "inode/directory" ) ||
         mimeType.startsWith( "image" ) ||
         mime->is( "multipart/x-mixed-replace" ) ||
         mime->is( "multipart/replace" ) ||
         mimeType.startsWith( "print" ) )
    {
        neverAsk = true;
    }
  }

  m_chkAskSave->blockSignals(true);
  m_chkAskSave->setChecked(ask && !neverAsk);
  m_chkAskSave->setEnabled(!neverAsk);
  m_chkAskSave->blockSignals(false);
}

void FileTypeDetails::slotAskSaveToggled(bool askSave)
{
  if (!m_item)
    return;

  m_item->setAskSave(askSave);
  emit changed(true);
}

void FileTypeDetails::setTypeItem( TypesListItem * tlitem )
{
  m_item = tlitem;
  Q_ASSERT(tlitem);
  iconButton->setIcon(tlitem->icon());
  description->setText(tlitem->comment());
  m_rbGroupSettings->setText( i18n("Use settings for '%1' group", tlitem->majorType() ) );
  extensionLB->clear();
  addExtButton->setEnabled(true);
  removeExtButton->setEnabled(false);

  serviceListWidget->setTypeItem( tlitem );
  embedServiceListWidget->setTypeItem( tlitem );
  m_autoEmbedGroup->button(tlitem->autoEmbed())->setChecked(true);
  m_rbGroupSettings->setEnabled( tlitem->canUseGroupSetting() );

  extensionLB->addItems(tlitem->patterns());

  updateAskSave();
}

void FileTypeDetails::enableExtButtons()
{
  removeExtButton->setEnabled(true);
}

#include "filetypedetails.moc"

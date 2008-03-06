/*
   Copyright (C) 2005 Ivor Hewitt <ivor@ivor.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Own
#include "filteropts.h"

// Qt
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtDBus/QtDBus>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QGroupBox>

// KDE
#include <kaboutdata.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khbox.h>
#include <klocale.h>
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY_DECLARATION(KcmKonqHtmlFactory)

KCMFilter::KCMFilter( QWidget *parent, const QVariantList& )
    : KCModule( KcmKonqHtmlFactory::componentData(), parent ),
      mGroupname( "Filter Settings" ),
      mSelCount(0)
{
    mConfig = KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals);
    setButtons(Default|Apply);

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setSpacing(KDialog::spacingHint());

    mEnableCheck = new QCheckBox(i18n("Enable filters"), this);
    topLayout->addWidget( mEnableCheck );

    mKillCheck = new QCheckBox(i18n("Hide filtered images"), this);
    topLayout->addWidget( mKillCheck );

    QGroupBox *topBox = new QGroupBox( i18n("URL Expressions to Filter") );
    topLayout->addWidget( topBox );

    QVBoxLayout *vbox = new QVBoxLayout;

    mListBox = new QListWidget;
    mListBox->setSelectionMode(QListWidget::ExtendedSelection);
    vbox->addWidget(mListBox);
    vbox->addWidget(new QLabel( i18n("Expression (e.g. http://www.site.com/ad/*):")));
    mString = new QLineEdit;
    vbox->addWidget(mString);

    KHBox *buttonBox = new KHBox;
    vbox->addWidget(buttonBox);
    buttonBox->setSpacing( KDialog::spacingHint() );

    topBox->setLayout(vbox);
    mInsertButton = new QPushButton( i18n("Insert"), buttonBox );
    connect( mInsertButton, SIGNAL( clicked() ), SLOT( insertFilter() ) );
    mUpdateButton = new QPushButton( i18n("Update"), buttonBox );
    connect( mUpdateButton, SIGNAL( clicked() ), SLOT( updateFilter() ) );
    mRemoveButton = new QPushButton( i18n("Remove"), buttonBox );
    connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeFilter() ) );

    mImportButton = new QPushButton(i18n("Import..."),buttonBox);
    connect( mImportButton, SIGNAL( clicked() ), SLOT( importFilters() ) );
    mExportButton = new QPushButton(i18n("Export..."),buttonBox);
    connect( mExportButton, SIGNAL( clicked() ), SLOT( exportFilters() ) );

    connect( mEnableCheck, SIGNAL( toggled(bool)), this, SLOT( slotEnableChecked()));
    connect( mKillCheck, SIGNAL( clicked()), this, SLOT( slotKillChecked()));
    connect( mListBox, SIGNAL(itemSelectionChanged()),this, SLOT(slotItemSelected()));
    connect( mString, SIGNAL(textChanged(const QString& ) ), this, SLOT( updateButton() ) );
/*
 * Whats this items
 */
    mEnableCheck->setWhatsThis( i18n("Enable or disable AdBlocK filters. When enabled a set of expressions "
                                        "to be blocked should be defined in the filter list for blocking to "
                                        "take effect."));
    mKillCheck->setWhatsThis( i18n("When enabled blocked images will be removed from the page completely "
                                      "otherwise a placeholder 'blocked' image will be used."));
    mListBox->setWhatsThis( i18n("This is the list of URL filters that will be applied to all linked "
                                    "images and frames. The filters are processed in order so place "
                                    "more generic filters towards the top of the list."));
    mString->setWhatsThis( i18n("Enter an expression to filter. Expressions can be defined as either "
                                   "a filename style wildcard e.g. http://www.site.com/ads* or as a full "
                                   "regular expression by surrounding the string with '/' e.g. "
                                   " //(ad|banner)\\./"));
    load();
    updateButton();
}

KCMFilter::~KCMFilter()
{
}

void KCMFilter::slotKillChecked()
{
    emit changed( true );
}

void KCMFilter::slotEnableChecked()
{
    updateButton();
    emit changed( true );
}

void KCMFilter::slotItemSelected()
{
    int currentId=-1;
    int i;
    for( i=0,mSelCount=0; i < mListBox->count() && mSelCount<2; ++i )
    {
        if (mListBox->item(i)->isSelected())
        {
            currentId=i;
            mSelCount++;
        }
    }

    if ( currentId >= 0 )
    {
        mString->setText(mListBox->item(currentId)->text());
    }
    updateButton();
}

void KCMFilter::updateButton()
{
    bool state = mEnableCheck->isChecked();
    bool expressionIsNotEmpty = !mString->text().isEmpty();
    mUpdateButton->setEnabled(state && (mSelCount == 1) && expressionIsNotEmpty );
    mRemoveButton->setEnabled(state && (mSelCount > 0));
    mInsertButton->setEnabled(state && expressionIsNotEmpty);
    mImportButton->setEnabled(state);
    mExportButton->setEnabled(state);

    mListBox->setEnabled(state);
    mString->setEnabled(state);
    mKillCheck->setEnabled(state);
}

void KCMFilter::importFilters()
{
    QString inFile = KFileDialog::getOpenFileName(KUrl(), QString(), this);
    if (inFile.length() > 0)
    {
        QFile f(inFile);
        if ( f.open( QIODevice::ReadOnly ) )
        {
            QTextStream ts( &f );
            QStringList paths;
            QString line;
            while (!ts.atEnd())
            {
                line = ts.readLine();
                if (line.toLower().compare("[adblock]") == 0)
                    continue;

                // Treat leading ! as filter comment, otherwise check expressions
                // are valid.
                if (!line.startsWith("!"))	//krazy:exclude=doublequote_chars
                {
                    if (line.length()>2 && line[0]=='/' && line[line.length()-1] == '/')
                    {
                        QString inside = line.mid(1, line.length()-2);
                        QRegExp rx(inside);
                        if (!rx.isValid())
                            continue;
                    }
                    else
                    {
                        QRegExp rx(line);
                        rx.setPatternSyntax(QRegExp::Wildcard);
                        if (!rx.isValid())
                            continue;
                    }
                }

                if (!line.isEmpty() &&
                     mListBox->findItems(line,
                                        Qt::MatchCaseSensitive|Qt::MatchExactly).isEmpty()) {
                    paths.append(line);
                }
            }
            f.close();

            mListBox->addItems( paths );
            emit changed(true);
        }
    }
}

void KCMFilter::exportFilters()
{
  QString outFile = KFileDialog::getSaveFileName(KUrl(), QString(), this);
  if (outFile.length() > 0)
  {
    QFile f(outFile);
    if ( f.open( QIODevice::WriteOnly ) )
    {
      QTextStream ts( &f );
      ts.setCodec( "UTF-8" );
      ts << "[AdBlock]" << endl;

      int i;
      for( i = 0; i < mListBox->count(); ++i )
        ts << mListBox->item(i)->text() << endl;

      f.close();
    }
  }
}

void KCMFilter::defaults()
{
    mListBox->clear();
    mEnableCheck->setChecked(false);
    mKillCheck->setChecked(false);
    updateButton();
}

void KCMFilter::save()
{
    KConfigGroup cg(mConfig, mGroupname);
    cg.deleteGroup();
    cg = KConfigGroup(mConfig, mGroupname);

    cg.writeEntry("Enabled",mEnableCheck->isChecked());
    cg.writeEntry("Shrink",mKillCheck->isChecked());

    int i;
    for( i = 0; i < mListBox->count(); ++i )
    {
        QString key = "Filter-" + QString::number(i);
        cg.writeEntry(key, mListBox->item(i)->text());
    }
    cg.writeEntry("Count",mListBox->count());

    cg.sync();

    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);
}

void KCMFilter::load()
{
    QStringList paths;

    KConfigGroup cg(mConfig, mGroupname);
    mEnableCheck->setChecked( cg.readEntry("Enabled", false));
    mKillCheck->setChecked( cg.readEntry("Shrink", false));

    QMap<QString,QString> entryMap = cg.entryMap();
    QMap<QString,QString>::ConstIterator it;
    int num = cg.readEntry("Count",0);
    for (int i=0; i<num; ++i)
    {
        QString key = "Filter-" + QString::number(i);
        it = entryMap.find(key);
        if (it != entryMap.end())
            paths.append(it.value());
    }

    mListBox->addItems( paths );
}

void KCMFilter::insertFilter()
{
    if ( !mString->text().isEmpty() )
    {
        mListBox->addItem( mString->text() );
        int id=mListBox->count()-1;
        mListBox->clearSelection();
        mListBox->item(id)->setSelected(true);
        mListBox->setCurrentRow(id);

#ifdef __GNUC__
#warning "KDE 4 - Port ensureCurrentVisible() call"
#endif
        //mListBox->ensureCurrentVisible();
        emit changed( true );
    }
    updateButton();
}

void KCMFilter::removeFilter()
{
    for( int i = mListBox->count(); i >= 0; --i )
    {
        if (mListBox->item(i) && mListBox->item(i)->isSelected())
            delete mListBox->takeItem(i);
    }
    emit changed( true );
    updateButton();
}

void KCMFilter::updateFilter()
{
    if ( !mString->text().isEmpty() )
    {
        int index = mListBox->currentRow();
        if ( index >= 0 )
        {
            mListBox->item(index)->setText(mString->text());
            emit changed( true );
        }
    }
    updateButton();
}

QString KCMFilter::quickHelp() const
{
    return i18n("<h1>Konqueror AdBlocK</h1> Konqueror AdBlocK allows you to create a list of filters"
                " that are checked against linked images and frames. URL's that match are either discarded or"
                " replaced with a placeholder image. ");
}


#include "filteropts.moc"


/*
    SPDX-FileCopyrightText: 2005 Ivor Hewitt <ivor@ivor.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "filteropts.h"

// Qt
#include <QRegExp>
#include <QTextStream>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QCheckBox>
#include <QLabel>
#include <QTreeView>
#include <QWhatsThis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QUrl>
#include <QFileDialog>

// KDE
#include <kaboutdata.h>
#include <kconfig.h>
#include <KLocalizedString>
#include <KPluginFactory>
#include <klistwidgetsearchline.h>
#include <klineedit.h>
#include <kpluralhandlingspinbox.h>
#include <KConfigGroup>
#include <KSharedConfig>

KCMFilter::KCMFilter(QWidget *parent, const QVariantList &)
    : KCModule(parent),
      mGroupname(QStringLiteral("Filter Settings")),
      mSelCount(0),
      mOriginalString(QString())
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::NoGlobals);
    setButtons(Default | Apply | Help);

    QVBoxLayout *topLayout = new QVBoxLayout(this);

    mEnableCheck = new QCheckBox(i18n("Enable filters"), this);
    topLayout->addWidget(mEnableCheck);

    mKillCheck = new QCheckBox(i18n("Hide filtered images"), this);
    topLayout->addWidget(mKillCheck);

    mFilterWidget = new QTabWidget(this);
    topLayout->addWidget(mFilterWidget);

    QWidget *container = new QWidget(mFilterWidget);
    mFilterWidget->addTab(container, i18n("Manual Filter"));

    QVBoxLayout *vbox = new QVBoxLayout;

    mListBox = new QListWidget;
    mListBox->setSelectionMode(QListWidget::ExtendedSelection);

    // If the filter list were sensitive to ordering, then we would need to
    // preserve the order of items inserted or arranged by the user (and the
    // GUI would need "Move Up" and "Move Down" buttons to reorder the filters).
    // However, now the filters are applied in an unpredictable order because
    // of the new hashed matching algorithm.  So the list can stay sorted.
    mListBox->setSortingEnabled(true);

    QWidget *searchBox = new QWidget;
    QHBoxLayout *searchBoxHBoxLayout = new QHBoxLayout(searchBox);
    searchBoxHBoxLayout->setContentsMargins(0, 0, 0, 0);
    searchBoxHBoxLayout->setSpacing(-1);
    new QLabel(i18n("Search:"), searchBox);

    mSearchLine = new KListWidgetSearchLine(searchBox, mListBox);

    vbox->addWidget(searchBox);

    vbox->addWidget(mListBox);

    QLabel *exprLabel = new QLabel(i18n("<qt>Filter expression (e.g. <tt>http://www.example.com/ad/*</tt>, <a href=\"filterhelp\">more information</a>):"), this);
    connect(exprLabel, &QLabel::linkActivated, this, &KCMFilter::slotInfoLinkActivated);
    vbox->addWidget(exprLabel);

    mString = new KLineEdit;
    vbox->addWidget(mString);

    QWidget *buttonBox = new QWidget;
    QHBoxLayout *buttonBoxHBoxLayout = new QHBoxLayout(buttonBox);
    buttonBoxHBoxLayout->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(buttonBox);

    container->setLayout(vbox);

    /** tab for automatic filter lists */
    container = new QWidget(mFilterWidget);
    mFilterWidget->addTab(container, i18n("Automatic Filter"));
    QGridLayout *grid = new QGridLayout;
    grid->setColumnStretch(2, 1);
    container->setLayout(grid);

    mAutomaticFilterList = new QTreeView(container);
    mAutomaticFilterList->setModel(&mAutomaticFilterModel);
    grid->addWidget(mAutomaticFilterList, 0, 0, 1, 3);

    QLabel *label = new QLabel(i18n("Automatic update interval:"), container);
    grid->addWidget(label, 1, 0);
    mRefreshFreqSpinBox = new KPluralHandlingSpinBox(container);
    grid->addWidget(mRefreshFreqSpinBox, 1, 1);
    mRefreshFreqSpinBox->setRange(1, 365);
    mRefreshFreqSpinBox->setSuffix(ki18np(" day", " days"));

    /** connect signals and slots */
    connect(&mAutomaticFilterModel, &AutomaticFilterModel::changed, this, QOverload<bool>::of(&KCModule::changed));
    connect(mRefreshFreqSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &KCMFilter::spinBoxChanged);

    mInsertButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Insert"), buttonBox);
    buttonBoxHBoxLayout->addWidget(mInsertButton);
    connect(mInsertButton, &QAbstractButton::clicked, this, &KCMFilter::insertFilter);
    mUpdateButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Update"), buttonBox);
    buttonBoxHBoxLayout->addWidget(mUpdateButton);
    connect(mUpdateButton, &QAbstractButton::clicked, this, &KCMFilter::updateFilter);
    mRemoveButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove"), buttonBox);
    buttonBoxHBoxLayout->addWidget(mRemoveButton);
    connect(mRemoveButton, &QAbstractButton::clicked, this, &KCMFilter::removeFilter);

    mImportButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-import")), i18n("Import..."), buttonBox);
    buttonBoxHBoxLayout->addWidget(mImportButton);
    connect(mImportButton, &QAbstractButton::clicked, this, &KCMFilter::importFilters);
    mExportButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-export")), i18n("Export..."), buttonBox);
    buttonBoxHBoxLayout->addWidget(mExportButton);
    connect(mExportButton, &QAbstractButton::clicked, this, &KCMFilter::exportFilters);

    QWidget *impexpBox = new QWidget;
    QHBoxLayout *impexpBoxHBoxLayout = new QHBoxLayout(impexpBox);
    impexpBoxHBoxLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *impexpLabel = new QLabel(i18n("<qt>More information on "
                                          "<a href=\"importhelp\">import format</a>, "
                                          "<a href=\"exporthelp\">export format</a>"), impexpBox);
    connect(impexpLabel, &QLabel::linkActivated, this, &KCMFilter::slotInfoLinkActivated);
    impexpBoxHBoxLayout->addWidget(impexpLabel);
    vbox->addWidget(impexpBox, 0, Qt::AlignRight);

    connect(mEnableCheck, &QAbstractButton::toggled, this, &KCMFilter::slotEnableChecked);
    connect(mKillCheck, &QAbstractButton::clicked, this, &KCMFilter::slotKillChecked);
    connect(mListBox, &QListWidget::itemSelectionChanged, this, &KCMFilter::slotItemSelected);
    connect(mString, &QLineEdit::textChanged, this, &KCMFilter::updateButton);
    /*
     * Whats this items
     */
    mEnableCheck->setToolTip(i18n("Enable or disable AdBlocK filters. When enabled, a set of URL expressions "
                                    "should be defined in the filter list for blocking to take effect."));
    mKillCheck->setToolTip(i18n("When enabled blocked images will be removed from the page completely, "
                                  "otherwise a placeholder 'blocked' image will be used."));

    // The list is no longer sensitive to order, because of the new hashed
    // matching.  So this tooltip doesn't imply that.
    //
    // FIXME: blocking of frames is not currently implemented by KHTML
    mListBox->setToolTip(i18n("This is the list of URL filters that will be applied to all embedded "
                                "images and media objects."));
    //                              "images, objects and frames.") );

    mString->setToolTip(i18n("<qt><p>Enter an expression to filter. Filters can be defined as either:"
                               "<ul><li>a shell-style wildcard, e.g. <tt>http://www.example.com/ads*</tt>, the wildcards <tt>*?[]</tt> may be used</li>"
                               "<li>a full regular expression by surrounding the string with '<tt>/</tt>', e.g. <tt>/\\/(ad|banner)\\./</tt></li></ul>"
                               "<p>Any filter string can be preceded by '<tt>@@</tt>' to whitelist (allow) any matching URL, "
                               "which takes priority over any blacklist (blocking) filter."));
}

KCMFilter::~KCMFilter()
{
}

void KCMFilter::slotInfoLinkActivated(const QString &url)
{
    if (url == QLatin1String("filterhelp")) {
        QWhatsThis::showText(QCursor::pos(), mString->toolTip());
    } else if (url == QLatin1String("importhelp"))
        QWhatsThis::showText(QCursor::pos(), i18n("<qt><p>The filter import format is a plain text file. "
                             "Blank lines, comment lines starting with '<tt>!</tt>' "
                             "and the header line <tt>[AdBlock]</tt> are ignored. "
                             "Any other line is added as a filter expression."));
    else if (url == QLatin1String("exporthelp"))
        QWhatsThis::showText(QCursor::pos(), i18n("<qt><p>The filter export format is a plain text file. "
                             "The file begins with a header line <tt>[AdBlock]</tt>, then all of "
                             "the filters follow each on a separate line."));
}

void KCMFilter::slotKillChecked()
{
    emit changed(true);
}

void KCMFilter::slotEnableChecked()
{
    updateButton();
    emit changed(true);
}

void KCMFilter::slotItemSelected()
{
    int currentId = -1;
    int i;
    for (i = 0, mSelCount = 0; i < mListBox->count() && mSelCount < 2; ++i) {
        if (mListBox->item(i)->isSelected()) {
            currentId = i;
            mSelCount++;
        }
    }

    if (currentId >= 0) {
        mOriginalString = mListBox->item(currentId)->text();
        mString->setText(mOriginalString);
        mString->setFocus(Qt::OtherFocusReason);
    }
    updateButton();
}

void KCMFilter::updateButton()
{
    bool state = mEnableCheck->isChecked();
    bool expressionIsNotEmpty = !mString->text().isEmpty();
    bool filterEdited = expressionIsNotEmpty && mOriginalString != mString->text();

    mInsertButton->setEnabled(state && expressionIsNotEmpty && filterEdited);
    mUpdateButton->setEnabled(state && (mSelCount == 1) && expressionIsNotEmpty && filterEdited);
    mRemoveButton->setEnabled(state && (mSelCount > 0));
    mImportButton->setEnabled(state);
    mExportButton->setEnabled(state && mListBox->count() > 0);

    mListBox->setEnabled(state);
    mString->setEnabled(state);
    mKillCheck->setEnabled(state);

    if (filterEdited) {
        if (mSelCount == 1 && mUpdateButton->isEnabled()) {
            mUpdateButton->setDefault(true);
        } else if (mInsertButton->isEnabled()) {
            mInsertButton->setDefault(true);
        }
    } else {
        mInsertButton->setDefault(false);
        mUpdateButton->setDefault(false);
    }

    mAutomaticFilterList->setEnabled(state);
    mRefreshFreqSpinBox->setEnabled(state);
}

void KCMFilter::importFilters()
{
    QString inFile = QFileDialog::getOpenFileName(this, i18n("Import Filters"));
    if (!inFile.isEmpty()) {
        QFile f(inFile);
        if (f.open(QIODevice::ReadOnly)) {
            QTextStream ts(&f);
            QStringList paths;
            QString line;
            while (!ts.atEnd()) {
                line = ts.readLine();
                if (line.isEmpty() || line.compare(QLatin1String("[adblock]"), Qt::CaseInsensitive) == 0) {
                    continue;
                }

                // Treat leading ! as filter comment, otherwise check expressions
                // are valid.
                if (!line.startsWith(QLatin1String("!"))) { //krazy:exclude=doublequote_chars
                    if (line.length() > 2 && line[0] == '/' && line[line.length() - 1] == '/') {
                        QString inside = line.mid(1, line.length() - 2);
                        QRegExp rx(inside);
                        if (!rx.isValid()) {
                            continue;
                        }
                    } else {
                        QRegExp rx(line);
                        rx.setPatternSyntax(QRegExp::Wildcard);
                        if (!rx.isValid()) {
                            continue;
                        }
                    }

                    if (mListBox->findItems(line, Qt::MatchCaseSensitive | Qt::MatchExactly).isEmpty()) {
                        paths.append(line);
                    }
                }
            }
            f.close();

            mListBox->addItems(paths);
            emit changed(true);
        }
    }
}

void KCMFilter::exportFilters()
{
    QString outFile = QFileDialog::getSaveFileName(this, i18n("Export Filters"));
    if (!outFile.isEmpty()) {

        QFile f(outFile);
        if (f.open(QIODevice::WriteOnly)) {
            QTextStream ts(&f);
            ts.setCodec("UTF-8");
            ts << "[AdBlock]" << Qt::endl;

            int nbLine =  mListBox->count();
            for (int i = 0; i < nbLine; ++i) {
                ts << mListBox->item(i)->text() << Qt::endl;
            }

            f.close();
        }
    }
}

void KCMFilter::defaults()
{
    mAutomaticFilterModel.defaults();

    mListBox->clear();
    mEnableCheck->setChecked(false);
    mKillCheck->setChecked(false);
    mString->clear();
    updateButton();
}

void KCMFilter::save()
{
    KConfigGroup cg(mConfig, mGroupname);
    cg.deleteGroup();
    cg = KConfigGroup(mConfig, mGroupname);

    cg.writeEntry("Enabled", mEnableCheck->isChecked());
    cg.writeEntry("Shrink", mKillCheck->isChecked());

    int i;
    for (i = 0; i < mListBox->count(); ++i) {
        QString key = "Filter-" + QString::number(i);
        cg.writeEntry(key, mListBox->item(i)->text());
    }
    cg.writeEntry("Count", mListBox->count());

    mAutomaticFilterModel.save(cg);
    cg.writeEntry("HTMLFilterListMaxAgeDays", mRefreshFreqSpinBox->value());

    cg.sync();

    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void KCMFilter::load()
{
    QStringList paths;

    KConfigGroup cg(mConfig, mGroupname);
    mAutomaticFilterModel.load(cg);
    mAutomaticFilterList->resizeColumnToContents(0);
    int refreshFreq = cg.readEntry("HTMLFilterListMaxAgeDays", 7);
    mRefreshFreqSpinBox->setValue(refreshFreq < 1 ? 1 : refreshFreq);

    mEnableCheck->setChecked(cg.readEntry("Enabled", false));
    mKillCheck->setChecked(cg.readEntry("Shrink", false));

    QMap<QString, QString> entryMap = cg.entryMap();
    QMap<QString, QString>::ConstIterator it;
    int num = cg.readEntry("Count", 0);
    for (int i = 0; i < num; ++i) {
        QString key = "Filter-" + QString::number(i);
        it = entryMap.constFind(key);
        if (it != entryMap.constEnd()) {
            paths.append(it.value());
        }
    }

    mListBox->addItems(paths);
    updateButton();
}

void KCMFilter::insertFilter()
{
    QString newFilter = mString->text();

    if (!newFilter.isEmpty() && mListBox->findItems(newFilter, Qt::MatchCaseSensitive | Qt::MatchExactly).isEmpty()) {
        mListBox->clearSelection();
        mListBox->addItem(newFilter);

        // The next line assumed that the new item would be added at the end
        // of the list, but that may not be the case if sorting is enabled.
        // So we search again to locate the just-added item.
        //int id = mListBox->count()-1;
        QListWidgetItem *newItem = mListBox->findItems(newFilter, Qt::MatchCaseSensitive | Qt::MatchExactly).first();
        if (newItem != nullptr) {
            int id = mListBox->row(newItem);

            mListBox->item(id)->setSelected(true);
            mListBox->setCurrentRow(id);
        }

        updateButton();
        emit changed(true);
    }
}

void KCMFilter::removeFilter()
{
    for (int i = mListBox->count(); i >= 0; --i) {
        if (mListBox->item(i) && mListBox->item(i)->isSelected()) {
            delete mListBox->takeItem(i);
        }
    }
    mString->clear();
    emit changed(true);
    updateButton();
}

void KCMFilter::updateFilter()
{
    if (!mString->text().isEmpty()) {
        int index = mListBox->currentRow();
        if (index >= 0) {
            mListBox->item(index)->setText(mString->text());
            emit changed(true);
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

void KCMFilter::spinBoxChanged(int)
{
    emit changed(true);
}

AutomaticFilterModel::AutomaticFilterModel(QObject *parent)
    : QAbstractItemModel(parent),
      mGroupname(QStringLiteral("Filter Settings"))
{
    //mConfig = KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals);
    mConfig = KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::IncludeGlobals);
}

void AutomaticFilterModel::load(KConfigGroup &cg)
{
    beginResetModel();
    mFilters.clear();
    const int maxNumFilters = 1024;
    const bool defaultHTMLFilterListEnabled = false;

    for (int numFilters = 1; numFilters < maxNumFilters; ++numFilters) {
        struct FilterConfig filterConfig;
        filterConfig.filterName = cg.readEntry(QStringLiteral("HTMLFilterListName-") + QString::number(numFilters), "");
        if (filterConfig.filterName == QLatin1String("")) {
            break;
        }

        filterConfig.enableFilter = cg.readEntry(QStringLiteral("HTMLFilterListEnabled-") + QString::number(numFilters), defaultHTMLFilterListEnabled);
        filterConfig.filterURL = cg.readEntry(QStringLiteral("HTMLFilterListURL-") + QString::number(numFilters), "");
        filterConfig.filterLocalFilename = cg.readEntry(QStringLiteral("HTMLFilterListLocalFilename-") + QString::number(numFilters), "");

        mFilters << filterConfig;
    }
    endResetModel();
}

void AutomaticFilterModel::save(KConfigGroup &cg)
{
    for (int i = mFilters.count() - 1; i >= 0; --i) {
        cg.writeEntry(QStringLiteral("HTMLFilterListLocalFilename-") + QString::number(i + 1), mFilters[i].filterLocalFilename);
        cg.writeEntry(QStringLiteral("HTMLFilterListURL-") + QString::number(i + 1), mFilters[i].filterURL);
        cg.writeEntry(QStringLiteral("HTMLFilterListName-") + QString::number(i + 1), mFilters[i].filterName);
        cg.writeEntry(QStringLiteral("HTMLFilterListEnabled-") + QString::number(i + 1), mFilters[i].enableFilter);
    }
}

void AutomaticFilterModel::defaults()
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::IncludeGlobals);
    KConfigGroup cg(mConfig, mGroupname);
    load(cg);
}

QModelIndex AutomaticFilterModel::index(int row, int column, const QModelIndex & /*parent*/) const
{
    return createIndex(row, column, (void *)nullptr);
}

QModelIndex AutomaticFilterModel::parent(const QModelIndex & /*index*/) const
{
    return QModelIndex();
}

bool AutomaticFilterModel::hasChildren(const QModelIndex &parent) const
{
    return parent == QModelIndex();
}

int AutomaticFilterModel::rowCount(const QModelIndex & /*parent*/) const
{
    return mFilters.count();
}

int AutomaticFilterModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant AutomaticFilterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole && index.row() < mFilters.count())
        switch (index.column()) {
        case 0: return QVariant(mFilters[index.row()].filterName);
        case 1: return QVariant(mFilters[index.row()].filterURL);
        default: return QVariant("?");
        }
    else if (role == Qt::CheckStateRole && index.column() == 0 && index.row() < mFilters.count()) {
        return mFilters[index.row()].enableFilter ? Qt::Checked : Qt::Unchecked;
    } else {
        return QVariant();
    }

}

bool AutomaticFilterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && index.column() == 0 && index.row() < mFilters.count()) {
        mFilters[index.row()].enableFilter = static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked;
        emit dataChanged(index, index);
        emit changed(true);
        return true;
    }

    return false;
}

QVariant AutomaticFilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case 0: return QVariant(i18n("Name"));
    case 1: return QVariant(i18n("URL"));
    default: return QVariant("?");
    }
}

Qt::ItemFlags AutomaticFilterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == 0) {
        rc |= Qt::ItemIsUserCheckable;
    }
    return rc;
}

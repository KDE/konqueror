/*
    SPDX-FileCopyrightText: 2009, 2010 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "downloadactionquestion.h"
#include "pluginmetadatautils.h"
#include "konqembedsettings.h"

#include <KConfigGroup>
#include <KFileItemActions>
#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KSqueezedTextLabel>
#include <KStandardGuiItem>
#include <KParts/PartLoader>

#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMenu>
#include <QMimeDatabase>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>
#include <QListWidget>

Q_DECLARE_METATYPE(KService::Ptr)

using MaybeAction = std::optional<DownloadActionQuestion::Action>;
using Actions = DownloadActionQuestion::Actions;
using Action = DownloadActionQuestion::Action;
using PartList = QList<KPluginMetaData>;

class DownloadActionQuestionPrivate : public QDialog
{
    Q_OBJECT
public:

    DownloadActionQuestionPrivate(QWidget *parent, const QUrl &url, const QString &mimeType);

    void showService(KService::Ptr selectedService);
    void disableAction(Action action);
    DownloadActionQuestion::Action ask(Actions actions);

    bool isDontAskAgainChosen() const;
    void setSuggestedFileName(const QString &suggestedFileName);

    KSharedConfig::Ptr dontAskConfig;
    QUrl url;
    QString mimeType;
    QMimeType mime;
    Actions availableActions;
    KService::Ptr selectedService;
    KPluginMetaData selectedPart;
    PartList parts;
    PartList otherParts;
    KService::List apps;
    bool shown = false;

    //Currently, the two std::optional arguments are only used by tests
    void setup(Actions actions);

    bool isActionAvailable(Action action) const;

    void setChosenAction(Action action);

private:
    void setupOpenButtons();
    void setupEmbedButtons();
    void setupSaveButton();
    QMenu* createOpenWithMenu(const KService::List &apps, const QString &openWithText);
    QMenu* createEmbedWithMenu(const PartList &parts, const PartList &otherParts);

private:
    QString suggestedFileName;
    KSqueezedTextLabel *questionLabel;
    QLabel *fileNameLabel;
    QDialogButtonBox *buttonBox;
    QPushButton *saveButton;
    QPushButton *openDefaultButton;
    QPushButton *openWithButton;
    QPushButton *embedDefaultButton;
    QPushButton *embedWithButton;
    QCheckBox *dontAskAgainCheckBox;

public Q_SLOTS:
    void reject() override;
    void slotYesClicked();
    void slotOpenDefaultClicked();
    void slotOpenWithClicked();
    void slotAppSelected(QAction *action);
    void slotEmbedDefaultClicked();
    void slotPartSelected(QAction *action);
    void doneWithBtn(DownloadActionQuestion::Action action){done(static_cast<int>(action));}

#ifdef BUILD_DOWNLOAD_ACTION_QUESTION_TESTS
private:
    KService::List forcedServices;
    PartList forcedParts;
    void setParts(const PartList &forcedParts);
    void setServices(const KService::List& forcedServices);

    friend class DownloadActionQuestionTest;
    friend class DownloadActionQuestion;
#endif
};

DownloadActionQuestionPrivate::DownloadActionQuestionPrivate(QWidget *parent, const QUrl &url, const QString &mimeType)
    : QDialog(parent)
    , url(url)
    , mimeType(mimeType)
{
    // Use askSave or askEmbedOrSave from filetypesrc
    dontAskConfig = KSharedConfig::openConfig(QStringLiteral("filetypesrc"), KConfig::NoGlobals);

    setWindowTitle(url.host());
    setObjectName(QStringLiteral("questionYesNoCancel"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    const int verticalSpacing = style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    mainLayout->setSpacing(verticalSpacing * 2); // provide extra spacing

    QHBoxLayout *hLayout = new QHBoxLayout();
    mainLayout->addLayout(hLayout, 5);

    QLabel *iconLabel = new QLabel(this);
    QStyleOption option;
    option.initFrom(this);
    QIcon icon = QIcon::fromTheme(QStringLiteral("dialog-information"));
    iconLabel->setPixmap(icon.pixmap(style()->pixelMetric(QStyle::PM_MessageBoxIconSize, &option, this)));

    hLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    const int horizontalSpacing = style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    hLayout->addSpacing(horizontalSpacing);

    QVBoxLayout *textVLayout = new QVBoxLayout;
    questionLabel = new KSqueezedTextLabel(this);
    textVLayout->addWidget(questionLabel);

    fileNameLabel = new QLabel(this);
    fileNameLabel->hide();
    textVLayout->addWidget(fileNameLabel);

    QMimeDatabase db;
    mime = db.mimeTypeForName(mimeType);
    QString mimeDescription(mimeType);
    if (mime.isValid()) {
        // Always prefer the mime-type comment over the raw type for display
        mimeDescription = (mime.comment().isEmpty() ? mime.name() : mime.comment());
    }
    QLabel *mimeTypeLabel = new QLabel(this);
    mimeTypeLabel->setText(i18nc("@label Type of file", "Type: %1", mimeDescription));
    mimeTypeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    textVLayout->addWidget(mimeTypeLabel);

    hLayout->addLayout(textVLayout, 5);

    mainLayout->addStretch(15);
    dontAskAgainCheckBox = new QCheckBox(this);
    dontAskAgainCheckBox->setText(i18nc("@label:checkbox", "Remember action for files of this type"));
    mainLayout->addWidget(dontAskAgainCheckBox);

    buttonBox = new QDialogButtonBox(this);

    saveButton = buttonBox->addButton(QDialogButtonBox::Yes);
    saveButton->setObjectName(QStringLiteral("saveButton"));
    KGuiItem::assign(saveButton, KStandardGuiItem::saveAs());
    saveButton->setDefault(true);

    openDefaultButton = new QPushButton;
    openDefaultButton->setObjectName(QStringLiteral("openDefaultButton"));
    buttonBox->addButton(openDefaultButton, QDialogButtonBox::ActionRole);

    openWithButton = new QPushButton;
    openWithButton->setObjectName(QStringLiteral("openWithButton"));
    buttonBox->addButton(openWithButton, QDialogButtonBox::ActionRole);

    embedDefaultButton = new QPushButton;
    embedDefaultButton->setObjectName(QStringLiteral("embedDefaultButton"));
    buttonBox->addButton(embedDefaultButton, QDialogButtonBox::ActionRole);

    embedWithButton = new QPushButton;
    embedWithButton->setObjectName(QStringLiteral("embedWithButton"));
    buttonBox->addButton(embedWithButton, QDialogButtonBox::ActionRole);

    QPushButton *cancelButton = buttonBox->addButton(QDialogButtonBox::Cancel);
    cancelButton->setObjectName(QStringLiteral("cancelButton"));

    connect(saveButton, &QPushButton::clicked, this, &DownloadActionQuestionPrivate::slotYesClicked);
    connect(openDefaultButton, &QPushButton::clicked, this, &DownloadActionQuestionPrivate::slotOpenDefaultClicked);
    connect(openWithButton, &QPushButton::clicked, this, &DownloadActionQuestionPrivate::slotOpenWithClicked);
    connect(embedDefaultButton, &QPushButton::clicked, this, &DownloadActionQuestionPrivate::slotEmbedDefaultClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DownloadActionQuestionPrivate::reject);

    mainLayout->addWidget(buttonBox);
}

void DownloadActionQuestionPrivate::setup(Actions actions)
{

    // I thought about using KFileItemActions, but we don't want a submenu, nor the slots....
    // and we want no menu at all if there's only one offer.
    apps = KFileItemActions::associatedApplications(QStringList{mimeType});

    parts = KParts::PartLoader::partsForMimeType(mimeType);

    //This is a hack to allow unit tests to use a known list of services:
    //in case we're building the BUILD_DOWNLOAD_ACTION_QUESTION_TESTS, ovverride
    //the lists returned by KFileItemActions::associatedApplications and
    //KParts::PartLoader::partsForMimeType and force the use of those chosen by
    //the tests. This is to allow the tests to work with a known list of parts and services
#ifdef BUILD_DOWNLOAD_ACTION_QUESTION_TESTS
    apps = forcedServices;
    parts = forcedParts;
#endif

    if (!apps.isEmpty()) {
        selectedService = apps.first();
    }

    if (!parts.isEmpty()) {
        selectedPart = parts.first();
    }
    otherParts = findParts([this](const KPluginMetaData &md){
        return !parts.contains(md) && md.value(QStringLiteral("X-KDE-BrowserView-AllowAsDefault"), true);
    });

    availableActions = actions;

    if (parts.isEmpty()) {
        disableAction(Action::Embed);
    }

    //If for whatever reason there's no available action, force the Save action
    //The most likely situation is when actions only include Embed but there are no
    //parts for this mimetype
    if (availableActions == Action::Cancel) {
        availableActions = Action::Save;
    }
}

bool DownloadActionQuestionPrivate::isActionAvailable(Action action) const
{
    return availableActions & action;
}

void DownloadActionQuestionPrivate::setChosenAction(Action action)
{
    if (action != Action::Open) {
        selectedService = {};
    }

    if (action != Action::Embed) {
        selectedPart = {};
    }
}

DownloadActionQuestion::Action DownloadActionQuestionPrivate::ask(Actions actions)
{

    setupSaveButton();
    setupOpenButtons();
    setupEmbedButtons();

    //Ensure that the "Don't ask again" check box is the last widget in tab order
    //instead of the first
    QPushButton *cancelBtn = buttonBox->button(QDialogButtonBox::Cancel);
    setTabOrder(cancelBtn, dontAskAgainCheckBox);

    QString urlString = url.toDisplayString(QUrl::PreferLocalFile);
    questionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (actions == Action::Save) {
        questionLabel->setText(i18nc("@info", "Save '%1'?", urlString));
    } else {
        questionLabel->setText(i18nc("@info", "Open '%1'?", urlString));
    }

    Action res = static_cast<DownloadActionQuestion::Action>(exec());
    shown = true;
    setChosenAction(res);
    return res;
}

void DownloadActionQuestionPrivate::setupOpenButtons()
{
    if (!isActionAvailable(Action::Open)) {
        openDefaultButton->hide();
        openWithButton->hide();
        disableAction(Action::Open);
        return;
    }

    KGuiItem openWithDialogItem(i18nc("@label:button", "&Open with..."), QStringLiteral("document-open"));

    if (apps.isEmpty()) {
        KGuiItem::assign(openDefaultButton, openWithDialogItem);
        openWithButton->hide();
        return;
    }
    KService::Ptr offer = apps.first();
    KGuiItem openItem(i18nc("@label:button", "&Open with %1", offer->name()), offer->icon());
    KGuiItem::assign(openDefaultButton, openItem);
    // openWithButton->show();
    QMenu *menu = createOpenWithMenu(apps, openWithDialogItem.text());
    if (!menu) {
        KGuiItem::assign(openWithButton, openWithDialogItem);
        return;
    }
    // Provide an additional button with a menu of associated apps
    KGuiItem openWithItem(i18nc("@label:button", "&Open with"), QStringLiteral("document-open"));
    KGuiItem::assign(openWithButton, openWithItem);
    openWithButton->setMenu(menu);
}

void DownloadActionQuestionPrivate::setupEmbedButtons()
{
    if (!isActionAvailable(Action::Embed)) {
        embedDefaultButton->hide();
        embedWithButton->hide();
        disableAction(Action::Embed);
        return;
    }

    if (!parts.isEmpty()) {
        selectedPart = parts.takeFirst();
        KGuiItem embedItem(i18nc("@label:button", "&Display in %1", selectedPart.name()), selectedPart.iconName());
        KGuiItem::assign(embedDefaultButton, embedItem);
    } else {
        selectedPart = {};
        embedDefaultButton->hide();
    }

    QMenu *menu = createEmbedWithMenu(parts, otherParts);
    if (!menu) {
        embedWithButton->hide();
        return;
    }
    KGuiItem embedWithItem(i18nc("@label:button", "&Display in"), QStringLiteral("document-preview"));
    KGuiItem::assign(embedWithButton, embedWithItem);
    embedWithButton->setMenu(menu);
}

void DownloadActionQuestionPrivate::setupSaveButton()
{
    bool saveEnabled = isActionAvailable(Action::Save);
    saveButton->setVisible(saveEnabled);
    if (!saveEnabled) {
        disableAction(Action::Save);
    }
}

QMenu* DownloadActionQuestionPrivate::createOpenWithMenu(const KService::List &apps, const QString &openWithText)
{
    if (apps.count() < 2) {
        return nullptr;
    }

    auto createAppAction = [](const KService::Ptr &service, QObject *parent) {
        QString actionName(service->name().replace(QLatin1Char('&'), QLatin1String("&&")));
        actionName = i18nc("@action:inmenu", "Open &with %1", actionName);

        QAction *act = new QAction(parent);
        act->setIcon(QIcon::fromTheme(service->icon()));
        act->setText(actionName);
        act->setData(QVariant::fromValue(service));
        return act;
    };

    QMenu *menu = new QMenu(this);
    // Provide an additional button with a menu of associated apps
    QObject::connect(menu, &QMenu::triggered, this, &DownloadActionQuestionPrivate::slotAppSelected);
    for (const auto &app : apps) {
        QAction *act = createAppAction(app, this);
        menu->addAction(act);
    }
    QAction *openWithDialogAction = new QAction(this);
    openWithDialogAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    openWithDialogAction->setText(openWithText);
    menu->addAction(openWithDialogAction);
    return menu;
}

QMenu* DownloadActionQuestionPrivate::createEmbedWithMenu(const PartList &parts, const PartList &otherParts)
{
    if (parts.isEmpty() && otherParts.isEmpty()) {
        return nullptr;
    }

    auto createPartAction = [] (const KPluginMetaData &part, QObject *parent, bool partNameAsText = false) {
        QString actionName(part.name().replace(QLatin1Char('&'), QLatin1String("&&")));
        if (partNameAsText) {
            actionName = i18nc("@action:inmenu", "Display &in %1", actionName);
        }

        QAction *act = new QAction(parent);
        act->setIcon(QIcon::fromTheme(part.iconName()));
        act->setText(actionName);
        act->setData(QVariant::fromValue(part));
        return act;
    };

    QMenu *menu = new QMenu(this);
    QObject::connect(menu, &QMenu::triggered, this, &DownloadActionQuestionPrivate::slotPartSelected);
    for (const auto &part : parts) {
        QAction *act = createPartAction(part, this);
        menu->addAction(act);
    }
    if (otherParts.isEmpty()) {
        return menu;
    }

    QAction *embedWithDialogAction = new QAction(this);
    embedWithDialogAction->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));
    embedWithDialogAction->setText(i18nc("@action:inmenu", "Viewers not supporting this file"));
    menu->addAction(embedWithDialogAction);

    QMenu *otherPartsSubmenu = new QMenu(menu);
    QAction *section = otherPartsSubmenu->addSection(i18nc("@action:inmenu", "Viewers unlikely to display the file"));
    QString fileName = suggestedFileName.isEmpty() ? i18nc("The name of the file the user wants to open", "this file") : suggestedFileName;
    section->setWhatsThis(i18n("<p>Viewers which aren't supposed to be able to display %1. Using them will most likely fail.</p>"
    "<p>These choices are provided in case the detected file type is incorrect or if a viewer can display %1 even if it doesn't officially support it.</p>", fileName));

    embedWithDialogAction->setMenu(otherPartsSubmenu);

    for (const KPluginMetaData &part: otherParts) {
        QAction *act = createPartAction(part, this);
        otherPartsSubmenu->addAction(act);
    }

    return menu;
}

bool DownloadActionQuestionPrivate::isDontAskAgainChosen() const
{
    return dontAskAgainCheckBox->isChecked();
}

void DownloadActionQuestionPrivate::setSuggestedFileName(const QString& suggestedName)
{
    suggestedFileName = suggestedName;
    fileNameLabel->setText(i18nc("@label File name", "Name: %1", suggestedName));
    fileNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fileNameLabel->setWhatsThis(i18nc("@info:whatsthis", "This is the file name suggested by the server"));
    fileNameLabel->show();
}

void DownloadActionQuestionPrivate::disableAction(Action action)
{
    availableActions = availableActions & ~static_cast<int>(action);
}

void DownloadActionQuestionPrivate::showService(KService::Ptr selectedService)
{
    KGuiItem openItem(i18nc("@label:button", "&Open with %1", selectedService->name()), selectedService->icon());
    KGuiItem::assign(openWithButton, openItem);
}

void DownloadActionQuestionPrivate::reject()
{
    selectedService = nullptr;
    QDialog::reject();
}

void DownloadActionQuestionPrivate::slotYesClicked()
{
    selectedService = nullptr;
    doneWithBtn(Action::Save);
}

void DownloadActionQuestionPrivate::slotOpenDefaultClicked()
{
    doneWithBtn(Action::Open);
}

void DownloadActionQuestionPrivate::slotOpenWithClicked()
{
    if (!openWithButton->menu()) {
        selectedService = nullptr;
        doneWithBtn(Action::Open);
    }
}

void DownloadActionQuestionPrivate::slotEmbedDefaultClicked()
{
    doneWithBtn(Action::Embed);
}

void DownloadActionQuestionPrivate::slotAppSelected(QAction *action)
{
    selectedService = action->data().value<KService::Ptr>();
    // showService(selectedService);
    doneWithBtn(Action::Open);
}

void DownloadActionQuestionPrivate::slotPartSelected(QAction* action)
{
    selectedPart = action->data().value<KPluginMetaData>();
    doneWithBtn(selectedPart.isValid() ? Action::Embed : Action::Cancel);
}

#ifdef BUILD_DOWNLOAD_ACTION_QUESTION_TESTS
void DownloadActionQuestionPrivate::setParts(const PartList& _forcedParts)
{
    forcedParts = _forcedParts;
}

void DownloadActionQuestionPrivate::setServices(const KService::List& _forcedServices)
{
    forcedServices = _forcedServices;
}
#endif

DownloadActionQuestion::DownloadActionQuestion(QWidget *parent, const QUrl &url, const QString &mimeType)
    : d(new DownloadActionQuestionPrivate(parent, url, mimeType))
{
}

DownloadActionQuestion::~DownloadActionQuestion() = default;

QString DownloadActionQuestion::dontAskAgainKey(DownloadActionQuestion::Action action) const
{
    switch (action) {
        case Action::Embed:
            return QLatin1String("askEmbedOrSave") + d->mimeType;
        case Action::Open:
            return QLatin1String("askSave") + d->mimeType;
        case Action::Cancel:
        case Action::Save:
            return {};
    }
    return {}; //We can't get here, but it's needed to avoid a compiler warning
}

std::optional<Action> DownloadActionQuestion::readDontAskAgainEntry(const KConfigGroup& cg, Action action) const
{
    //It shouldn't happen
    if (action == Action::Save || action == Action::Cancel) {
        return std::nullopt;
    }
    const QString key = dontAskAgainKey(action);
    const QString value = cg.readEntry(key, QString());
    if (value == QLatin1String("yes") || value == QLatin1String("true")) {
        return std::make_optional(Action::Save);
    } else if (value == QLatin1String("no") || value == QLatin1String("false")) {
        return std::make_optional(action == Action::Embed ? Action::Embed : Action::Open);
    } else {
        return std::nullopt;
    }
}

int operator~(DownloadActionQuestion::Action action)
{
    return ~static_cast<int>(action);
}

std::optional< DownloadActionQuestion::Action > DownloadActionQuestion::determineAction(Actions requestedActions, EmbedFlags flag, const KConfigGroup& cg)
{
    //If the caller only allowed the Save action, do it regardless of everything else
    if (requestedActions == Action::Save) {
        return Action::Save;
    }

    Actions actions = d->availableActions;
    if (flag == DownloadActionQuestion::ForceDialog || actions == Action::Cancel) {
        return std::nullopt;
    }

    //Don't automatically attempt to open the URL if we don't have an application which can handle it
    //NOTE: unlike for embed, actions can include Open if there aren't applications for this mimetype
    //because of the "Open with..." button which allows selecting another application
    if (!d->selectedService) {
        actions &= ~Action::Open;
    }

    //We don't want to save local files, except in the case Save is the only available action
    if (d->url.isLocalFile() && actions != Action::Save) {
        actions &= ~Action::Save;
    } else if (actions == Action::Cancel) {
        //If no action is available, ask the user what to do
        return std::nullopt;
    }

    bool shouldEmbed = KonqFMSettings::settings()->shouldEmbed(d->mimeType);
    Action preferredDisplayAction = shouldEmbed ? Action::Embed : Action::Open;
    //Read the user settings to determine if the user wants to be asked what to do
    MaybeAction autoAction = readDontAskAgainEntry(cg, preferredDisplayAction);

    //We can't use d->isActionAvailable because it uses d->availableActions instead of
    //actions
    auto isActionAvailable = [actions](Action action) {return actions & action;};

    //If the only available action is Save, return nullopt unless the user chose
    //to save without being asked. The setting is read from the Embed or Open setting
    //depending on preferredDisplayAction
    if (actions == Action::Save) {
        return autoAction == Action::Save ? autoAction : std::nullopt;
    } else if (!isActionAvailable(Action::Save)) {
        //If Save is not available return the preferred action if available and nullopt otherwise
        // return isActionAvailable(preferredDisplayAction) ? std::make_optional(preferredDisplayAction) : std::nullopt;
        if (isActionAvailable(preferredDisplayAction)) {
            return preferredDisplayAction;
        } else {
            return std::nullopt;
        }
    } else if (autoAction && isActionAvailable(autoAction.value())) {
        //If the user has chosen an automatic action and that action is available, return it
        return autoAction;
    } else {
        //In all other cases, return nullopt, so that the user is asked how to proceed
        return std::nullopt;
    }

}

Action DownloadActionQuestion::ask(Actions actions, EmbedFlags flag)
{
    d->setup(actions);

    if (d->isActionAvailable(Action::Embed)) {
        if (autoEmbedMimeType(flag)) {
            d->setChosenAction(Action::Embed);
            return Action::Embed;
        }
    }

    KConfigGroup cg(d->dontAskConfig, "Notification Messages"); // group name comes from KMessageBox

    if (MaybeAction ans = determineAction(actions, flag, cg); ans.has_value()) {
        d->setChosenAction(ans.value());
        return ans.value();
    }

    const Action action = d->ask(actions);

    if (d->isDontAskAgainChosen()) {
        if (action == Action::Embed) {
            cg.writeEntry(dontAskAgainKey(Action::Embed), false);
        } else if (action == Action::Open) {
            cg.writeEntry(dontAskAgainKey(Action::Open), false);
        } else if (action == Action::Save) {
            cg.writeEntry(dontAskAgainKey(actions & Action::Embed ? Action::Embed : Action::Open), true);
        }
        cg.sync();
    }

    return action;
}

bool DownloadActionQuestion::autoEmbedMimeType(EmbedFlags flags)
{
    // SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC SYNC
    // NOTE: Keep this function in sync with
    // kdebase/runtime/keditfiletype/filetypedetails.cpp
    //       FileTypeDetails::updateAskSave()

    // Don't ask for:
    // - html (even new tabs would ask, due to about:blank!)
    // - dirs obviously (though not common over HTTP :),
    // - images (reasoning: no need to save, most of the time, because fast to see)
    // e.g. postscript is different, because takes longer to read, so
    // it's more likely that the user might want to save it.
    // - multipart/* ("server push", see kmultipart)
    // - local files: no need to save
    // KEEP IN SYNC!!!
    if (flags != DownloadActionQuestion::InlineDisposition) {
        return false;
    }

    if (!d->mime.isValid()) {
        return false;
    }
    if (d->mimeType.startsWith(QLatin1String("image"))) {
        return true;
    }
    const QStringList autoEmbed = {
            QStringLiteral("text/html"),
            QStringLiteral("inode/directory"),
            QStringLiteral("multipart/x-mixed-replace"),
            QStringLiteral("multipart/replace")
    };
    return std::any_of(autoEmbed.constBegin(), autoEmbed.constEnd(), [this](const QString &str){return d->mime.inherits(str);});
}

DownloadActionQuestion::Action DownloadActionQuestion::ask(EmbedFlags flag)
{
    return ask(Action::Embed|Action::Open|Action::Save, flag);
}

DownloadActionQuestion::Action DownloadActionQuestion::askOpenOrSave()
{
    return ask(Action::Open | Action::Save);
}

KService::Ptr DownloadActionQuestion::selectedService() const
{
    return d->selectedService;
}

KPluginMetaData DownloadActionQuestion::selectedPart() const
{
    return d->selectedPart;
}

DownloadActionQuestion::Action DownloadActionQuestion::askEmbedOrSave(EmbedFlags flags)
{
    return ask(Action::Embed|Action::Save, flags);
}

void DownloadActionQuestion::setSuggestedFileName(const QString &suggestedFileName)
{
    if (!suggestedFileName.isEmpty()) {
        d->setSuggestedFileName(suggestedFileName);
    }
}

bool DownloadActionQuestion::dialogShown() const
{
    return d->shown;
}

QDebug operator<<(QDebug dbg, DownloadActionQuestion::Action action)
{
    QDebugStateSaver saver(dbg);
    switch (action) {
        case DownloadActionQuestion::Action::Cancel:
            return dbg << "Cancel";
        case DownloadActionQuestion::Action::Save:
            return dbg << "Save";
        case DownloadActionQuestion::Action::Open:
            return dbg << "Open";
        case DownloadActionQuestion::Action::Embed:
            return dbg << "Embed";
    }
    return dbg;
}

#ifdef BUILD_DOWNLOAD_ACTION_QUESTION_TESTS

void DownloadActionQuestion::setupDialog(Actions actions)
{
    d->setup(actions);
}

QDialog* DownloadActionQuestion::dialog()
{
    return d.get();
}

void DownloadActionQuestion::setApps(const KService::List& apps)
{
    d->setServices(apps);
}

void DownloadActionQuestion::setParts(const QList<KPluginMetaData>& parts)
{
    d->setParts(parts);
}

#endif

#include "downloadactionquestion.moc"

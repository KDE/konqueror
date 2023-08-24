/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "useragent.h"
#include "ui_useragent.h"
#include "interfaces/browser.h"

#include <KConfigGroup>
#include <KMessageWidget>

#include <QDBusMessage>
#include <QDBusConnection>
#include <QInputDialog>
#include <QWebEngineProfile>
#include <QDir>


UserAgent::UserAgent(QObject *parent, const KPluginMetaData &md, const QVariantList &): KCModule(parent, md),
    m_ui(new Ui::UserAgent),
    m_config(KSharedConfig::openConfig(QString(), KConfig::NoGlobals)),
    m_templatesConfig(KSharedConfig::openConfig("useragenttemplatesrc"))
{
    m_ui->setupUi(widget());
    fillTemplateWidget(m_templatesConfig->group("Templates").entryMap());
    connect(m_ui->useTemplateBtn, &QPushButton::clicked, this, &UserAgent::useSelectedTemplate);
    connect(m_ui->templates, &QTreeWidget::itemDoubleClicked, this, &UserAgent::useDblClickedTemplate);
    connect(m_ui->templates, &QTreeWidget::itemSelectionChanged, this, &UserAgent::templateSelectionChanged);
    connect(m_ui->useDefaultUA, &QCheckBox::toggled, this, [this](bool on){toggleCustomUA(!on);});
    connect(m_ui->userAgentString, &QLineEdit::textChanged, this, [this](){setNeedsSave(true);});
    connect(m_ui->editTemplateBtn, &QPushButton::clicked, this, &UserAgent::editTemplate);
    connect(m_ui->newTemplateBtn, &QPushButton::clicked, this, &UserAgent::createNewTemplate);
    connect(m_ui->duplicateTemplateBtn, &QPushButton::clicked, this, &UserAgent::duplicateTemplate);
    connect(m_ui->renameTemplateBtn, &QPushButton::clicked, this, &UserAgent::renameTemplate);
    connect(m_ui->deleteTemplateBtn, &QPushButton::clicked, this, &UserAgent::deleteTemplate);
    connect(m_ui->templates, &QTreeWidget::itemChanged, this, &UserAgent::templateChanged);
}

UserAgent::~UserAgent()
{
}

bool UserAgent::useCustomUserAgent() const
{
    return !m_ui->useDefaultUA->isChecked();
}

void UserAgent::fillTemplateWidget(const UserAgent::TemplateMap& templates)
{
    m_ui->templates->clear();
    for (auto it = templates.constBegin(); it != templates.constEnd(); ++it) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->templates, {it.key(), it.value()});
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setToolTip(1, it.value());
        m_ui->templates->addTopLevelItem(item);
    }
}

void UserAgent::defaults()
{
    //Find the default templates. As far as I know, KConfig doesn't provide a way to only open the global config
    //so we use QStandardPaths::locateAll to find all useragenttemplatesrc files. We assume that, as for QStandardPaths::standardLocations
    //the files are sorted from highest to lowest priority, so we use the last one.
    QStringList files = QStandardPaths::locateAll(QStandardPaths::ConfigLocation, "useragenttemplatesrc");
    if (!files.isEmpty()) {
        KConfigGroup grp = KSharedConfig::openConfig(files.constLast(), KConfig::SimpleConfig)->group("Templates");
        fillTemplateWidget(grp.entryMap());
    }

    m_ui->useDefaultUA->setChecked(true);
    m_ui->userAgentString->setText(QString());
    setNeedsSave(true);
}

void UserAgent::load()
{
    KConfigGroup grp = m_config->group("UserAgent");
    m_ui->userAgentString->setText(grp.readEntry("CustomUserAgent", QString()));
    m_ui->useDefaultUA->setChecked(grp.readEntry("UseDefaultUserAgent", true));
    toggleCustomUA(useCustomUserAgent());
    m_ui->invalidTemplateNameWidget->hide(); //There can't be problems when loading
    setNeedsSave(false);
}

void UserAgent::save()
{
    KConfigGroup grp = m_config->group("UserAgent");
    grp.writeEntry("CustomUserAgent", m_ui->userAgentString->text());
    grp.writeEntry("UseDefaultUserAgent", m_ui->useDefaultUA->isChecked());
    grp.sync();
    saveTemplates();
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"),
                                                      QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
    setNeedsSave(false);
}

void UserAgent::saveTemplates()
{
    KConfigGroup grp = m_templatesConfig->group("Templates");
    TemplateMap oldTemplates = grp.entryMap();
    TemplateMap newTemplates = templatesFromUI();
    for (TemplateMap::const_iterator it = oldTemplates.constBegin(); it!= oldTemplates.constEnd(); ++it) {
        if (!newTemplates.contains(it.key())) {
            grp.deleteEntry(it.key());
        }
    }
    for (TemplateMap::const_iterator it = newTemplates.constBegin(); it!= newTemplates.constEnd(); ++it) {
        grp.writeEntry(it.key(), it.value());
    }
    grp.sync();
}

void UserAgent::enableDisableUseSelectedTemplateBtn()
{
    m_ui->useTemplateBtn->setEnabled(useCustomUserAgent() && selectedTemplate() != nullptr);
}

void UserAgent::toggleCustomUA(bool on)
{
    m_ui->userAgentString->setEnabled(on);
    m_ui->customUABox->setEnabled(on);
    enableDisableUseSelectedTemplateBtn();
    setNeedsSave(true);
}

QTreeWidgetItem * UserAgent::selectedTemplate() const
{
    QList<QTreeWidgetItem*> items = m_ui->templates->selectedItems();
    return !items.isEmpty() ? items.at(0) : nullptr;
}

void UserAgent::useSelectedTemplate()
{
    QTreeWidgetItem *it = selectedTemplate();
    if (it) {
        useTemplate(it->text(1));
    }
}

void UserAgent::useDblClickedTemplate(QTreeWidgetItem *it, int)
{
    if (it) {
        useTemplate(it->text(1));
    }
}

void UserAgent::useTemplate(const QString &templ)
{
    m_ui->userAgentString->setText(templ);
}

void UserAgent::templateSelectionChanged()
{
    bool selectionExists = !m_ui->templates->selectedItems().isEmpty();
    std::vector<QPushButton*> btns {m_ui->deleteTemplateBtn, m_ui->editTemplateBtn, m_ui->renameTemplateBtn, m_ui->duplicateTemplateBtn};
    for (QPushButton *btn : btns) {
        btn->setEnabled(selectionExists);
    }
    enableDisableUseSelectedTemplateBtn();
}

void UserAgent::createNewTemplate()
{
    QTreeWidgetItem *it = createNewTemplateInternal();
    if (it) {
        m_ui->templates->editItem(it, 1);
    }
}

void UserAgent::duplicateTemplate()
{
    QTreeWidgetItem *sel = selectedTemplate();
    if (!sel) {
        return;
    }
    QTreeWidgetItem *it = createNewTemplateInternal();
    if (it) {
        it->setText(1, sel->text(1));
    }
}

QTreeWidgetItem* UserAgent::createNewTemplateInternal()
{
    bool ok = false;
    QString name = QInputDialog::getText(widget(), i18nc("@title:window Title of dialog to choose name to given to new User Agent", "Choose User Agent name"),
                                         i18nc("Name of the new User Agent", "User Agent name"), QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return nullptr;
    }
    KonqInterfaces::Browser* browser = KonqInterfaces::Browser::browser(qApp);
    QString defaultUserAgent = browser ? browser->konqUserAgent() : QString();
    QTreeWidgetItem *it = new QTreeWidgetItem({name, defaultUserAgent});
    it->setFlags(it->flags() | Qt::ItemIsEditable);
    m_ui->templates->addTopLevelItem(it);
    checkTemplatesValidity();
    m_ui->templates->selectionModel()->clearSelection();
    it->setSelected(true);
    return it;
}


void UserAgent::deleteTemplate()
{
    QTreeWidgetItem *it = selectedTemplate();
    if (it) {
        delete it;
        setNeedsSave(true);
    }
}

void UserAgent::editTemplate()
{
    QTreeWidgetItem *it = selectedTemplate();
    if (it) {
        m_ui->templates->editItem(it, 1);
    }
}

void UserAgent::renameTemplate()
{
    QTreeWidgetItem *it = selectedTemplate();
    if (it) {
        m_ui->templates->editItem(it, 0);
    }
}

void UserAgent::templateChanged(QTreeWidgetItem*, int col)
{
    if (col == 0) {
        checkTemplatesValidity();
    }
    setNeedsSave(true);
}

UserAgent::TemplateMap UserAgent::templatesFromUI() const
{
    TemplateMap map;
    for (int i = 0; i < m_ui->templates->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = m_ui->templates->topLevelItem(i);
        map[it->text(0)] = it->text(1);
    }
    return map;
}

QString emptyTemplateNameMsg() {
    static QString s_msg(i18n("there are templates with empty names"));
    return s_msg;
}

QString duplicateTemplateNamesMsg() {
    static QString s_msg(i18n("there are multiple templates with the same name"));
    return s_msg;
}

void UserAgent::checkTemplatesValidity()
{
    QStringList names;
    names.reserve(m_ui->templates->topLevelItemCount());
    bool emptyNames = false;
    for (int i =0; i < m_ui->templates->topLevelItemCount(); ++i) {
        QString name = m_ui->templates->topLevelItem(i)->text(0);
        names.append(name);
        if (name.isEmpty()) {
            emptyNames = true;
        }
    }
    names.sort();
    bool hasDuplicates = std::unique(names.begin(), names.end()) != names.end();
    if (!emptyNames && !hasDuplicates) {
        m_ui->invalidTemplateNameWidget->animatedHide();
        return;
    }

    QString invalidTemplateNamesHeader(i18nc("The user gave a custom user agent string an empty or duplicate name", "Invalid user agent names:"));
    QString invalidTemplatesText;
    if (emptyNames && hasDuplicates) {
        invalidTemplatesText = QLatin1String("%1<br><ul><li>%2</li><li>%3</li></ul>").arg(invalidTemplateNamesHeader, emptyTemplateNameMsg(), duplicateTemplateNamesMsg());
    } else {
        invalidTemplatesText = QLatin1String("%1 %2").arg(invalidTemplateNamesHeader, (emptyNames ? emptyTemplateNameMsg() : duplicateTemplateNamesMsg()));
    }
    m_ui->invalidTemplateNameWidget->setText(invalidTemplatesText);
    m_ui->invalidTemplateNameWidget->animatedShow();
}

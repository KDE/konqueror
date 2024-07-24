/*
    SPDX-FileCopyrightText: 2002 Leo Savernik <l.savernik@aon.at>
    Derived from jsopt.cpp, code copied from there is copyrighted to its
    respective owners.

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "jspolicies.h"

// Qt
#include <QButtonGroup>
#include <QLabel>
#include <QRadioButton>
#include <QGridLayout>

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>
#include <KLocalizedString>

// == class JSPolicies ==

JSPolicies::JSPolicies(KSharedConfig::Ptr config, const QString &group,
                       bool global, const QString &domain) :
    is_global(global), config(config), groupname(group),
    prefix(QStringLiteral("javascript.")), feature_key(QStringLiteral("EnableJavaScript"))
{
    if (is_global) {
        this->prefix.clear();   // global keys have no prefix
    }/*end if*/
    setDomain(domain);
}

JSPolicies::~JSPolicies() noexcept
{
}

void JSPolicies::setDomain(const QString &domain)
{
    if (is_global) {
        return;
    }
    this->domain = domain.toLower();
    groupname = this->domain; // group is domain in this case
}


void JSPolicies::load()
{

    KConfigGroup cg(config, groupname);

    QString key = prefix + feature_key;
    if (cg.hasKey(key)) {
        feature_enabled = cg.readEntry(key, false);
    } else {
        feature_enabled = is_global ? true : inherit_policy;
    }

    //Explicitly cast enum to uint to avoid a compiler warning
    key = prefix + "WindowOpenPolicy";
    window_open = cg.readEntry(key, (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowOpenSmart) : inherit_policy));

    key = prefix + "WindowResizePolicy";
    window_resize = cg.readEntry(key, (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowResizeAllow) : inherit_policy));

    key = prefix + "WindowMovePolicy";
    window_move = cg.readEntry(key, (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowMoveAllow) : inherit_policy));

    key = prefix + "WindowFocusPolicy";
    window_focus = cg.readEntry(key, (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowFocusAllow) : inherit_policy));

    key = prefix + "WindowStatusPolicy";
    window_status = cg.readEntry(key, (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowStatusAllow) : inherit_policy));
}

void JSPolicies::defaults()
{

    feature_enabled = is_global ? true : inherit_policy;

    //Explicitly cast enum to uint to avoid a compiler warning
    window_open = (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowOpenSmart) : inherit_policy);
    window_resize = (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowResizeAllow) : inherit_policy);
    window_move = (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowMoveAllow) : inherit_policy);
    window_focus = (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowFocusAllow) : inherit_policy);
    window_status = (is_global ? static_cast<uint>(HtmlSettingsInterface::JSWindowStatusAllow) : inherit_policy);
}

void JSPolicies::save()
{

    KConfigGroup cg(config, groupname);

    QString key = prefix + feature_key;
    if (feature_enabled != inherit_policy) {
        cg.writeEntry(key, (bool)feature_enabled);
    } else {
        cg.deleteEntry(key);
    }

    key = prefix + "WindowOpenPolicy";
    if (window_open != inherit_policy) {
        config->group(groupname).writeEntry(key, window_open);
    } else {
        config->group(groupname).deleteEntry(key);
    }

    key = prefix + "WindowResizePolicy";
    if (window_resize != inherit_policy) {
        config->group(groupname).writeEntry(key, window_resize);
    } else {
        config->group(groupname).deleteEntry(key);
    }

    key = prefix + "WindowMovePolicy";
    if (window_move != inherit_policy) {
        config->group(groupname).writeEntry(key, window_move);
    } else {
        config->group(groupname).deleteEntry(key);
    }

    key = prefix + "WindowFocusPolicy";
    if (window_focus != inherit_policy) {
        config->group(groupname).writeEntry(key, window_focus);
    } else {
        config->group(groupname).deleteEntry(key);
    }

    key = prefix + "WindowStatusPolicy";
    if (window_status != inherit_policy) {
        config->group(groupname).writeEntry(key, window_status);
    } else {
        config->group(groupname).deleteEntry(key);
    }

    // don't do a config->sync() here for sake of efficiency
}

// == class JSPoliciesFrame ==

JSPoliciesFrame::JSPoliciesFrame(JSPolicies *policies, const QString &title,
                                 QWidget *parent) :
    QGroupBox(title, parent),
    policies(policies)
{

    bool is_per_domain = !policies->isGlobal();

    QGridLayout *this_layout = new QGridLayout();
    setLayout(this_layout);
    this_layout->setAlignment(Qt::AlignTop);

    QString wtstr;    // what's this description
    int colIdx;       // column index

    // === window.open ================================
    colIdx = 0;
    QLabel *label = new QLabel(i18n("Open new windows:"), this);
    this_layout->addWidget(label, 0, colIdx++);

    js_popup = new QButtonGroup(this);
    js_popup->setExclusive(true);

    QRadioButton *policy_btn;
    if (is_per_domain) {
        policy_btn = new QRadioButton(i18n("Use global"), this);
        policy_btn->setToolTip(i18n("Use setting from global policy."));
        js_popup->addButton(policy_btn, JSPolicies::inherit_policy);
        this_layout->addWidget(policy_btn, 0, colIdx++);
        this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);
    }/*end if*/

    policy_btn = new QRadioButton(i18n("Allow"), this);
    policy_btn->setToolTip(i18n("Accept all popup window requests."));
    js_popup->addButton(policy_btn, HtmlSettingsInterface::JSWindowOpenAllow);
    this_layout->addWidget(policy_btn, 0, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Ask"), this);
    policy_btn->setToolTip(i18n("Prompt every time a popup window is requested."));
    js_popup->addButton(policy_btn, HtmlSettingsInterface::JSWindowOpenAsk);
    this_layout->addWidget(policy_btn, 0, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Deny"), this);
    policy_btn->setToolTip(i18n("Reject all popup window requests."));
    js_popup->addButton(policy_btn, HtmlSettingsInterface::JSWindowOpenDeny);
    this_layout->addWidget(policy_btn, 0, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Smart"), this);
    policy_btn->setToolTip(i18n("Accept popup window requests only when "
                                  "links are activated through an explicit "
                                  "mouse click or keyboard operation."));
    js_popup->addButton(policy_btn, HtmlSettingsInterface::JSWindowOpenSmart);
    this_layout->addWidget(policy_btn, 0, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    wtstr = i18n("If you disable this, Konqueror will stop "
                 "interpreting the <i>window.open()</i> "
                 "JavaScript command. This is useful if you "
                 "regularly visit sites that make extensive use "
                 "of this command to pop up ad banners.<br />"
                 "<br /><b>Note:</b> Disabling this option might "
                 "also break certain sites that require <i>"
                 "window.open()</i> for proper operation. Use "
                 "this feature carefully.");
    label->setToolTip(wtstr);
    connect(js_popup, &QButtonGroup::idClicked, this, &JSPoliciesFrame::setWindowOpenPolicy);

    // === window.resizeBy/resizeTo ================================
    colIdx = 0;
    label = new QLabel(i18n("Resize window:"), this);
    this_layout->addWidget(label, 1, colIdx++);

    js_resize = new QButtonGroup(this);
    js_resize->setExclusive(true);

    if (is_per_domain) {
        policy_btn = new QRadioButton(i18n("Use global"), this);
        policy_btn->setToolTip(i18n("Use setting from global policy."));
        js_resize->addButton(policy_btn, JSPolicies::inherit_policy);
        this_layout->addWidget(policy_btn, 1, colIdx++);
        this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);
    }/*end if*/

    policy_btn = new QRadioButton(i18n("Allow"), this);
    policy_btn->setToolTip(i18n("Allow scripts to change the window size."));
    js_resize->addButton(policy_btn, HtmlSettingsInterface::JSWindowResizeAllow);
    this_layout->addWidget(policy_btn, 1, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Ignore"), this);
    policy_btn->setToolTip(i18n("Ignore attempts of scripts to change the window size. "
                                  "The web page will <i>think</i> it changed the "
                                  "size but the actual window is not affected."));
    js_resize->addButton(policy_btn, HtmlSettingsInterface::JSWindowResizeIgnore);
    this_layout->addWidget(policy_btn, 1, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    wtstr = i18n("Some websites change the window size on their own by using "
                 "<i>window.resizeBy()</i> or <i>window.resizeTo()</i>. "
                 "This option specifies the treatment of such "
                 "attempts.");
    label->setToolTip(wtstr);
    connect(js_resize, &QButtonGroup::idClicked, this, &JSPoliciesFrame::setWindowResizePolicy);

    // === window.moveBy/moveTo ================================
    colIdx = 0;
    label = new QLabel(i18n("Move window:"), this);
    this_layout->addWidget(label, 2, colIdx++);

    js_move = new QButtonGroup(this);
    js_move->setExclusive(true);

    if (is_per_domain) {
        policy_btn = new QRadioButton(i18n("Use global"), this);
        policy_btn->setToolTip(i18n("Use setting from global policy."));
        js_move->addButton(policy_btn, JSPolicies::inherit_policy);
        this_layout->addWidget(policy_btn, 2, colIdx++);
        this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);
    }/*end if*/

    policy_btn = new QRadioButton(i18n("Allow"), this);
    policy_btn->setToolTip(i18n("Allow scripts to change the window position."));
    js_move->addButton(policy_btn, HtmlSettingsInterface::JSWindowMoveAllow);
    this_layout->addWidget(policy_btn, 2, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Ignore"), this);
    policy_btn->setToolTip(i18n("Ignore attempts of scripts to change the window position. "
                                  "The web page will <i>think</i> it moved the "
                                  "window but the actual position is not affected."));
    js_move->addButton(policy_btn, HtmlSettingsInterface::JSWindowMoveIgnore);
    this_layout->addWidget(policy_btn, 2, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    wtstr = i18n("Some websites change the window position on their own by using "
                 "<i>window.moveBy()</i> or <i>window.moveTo()</i>. "
                 "This option specifies the treatment of such "
                 "attempts.");
    label->setToolTip(wtstr);
    connect(js_move, &QButtonGroup::idClicked, this, &JSPoliciesFrame::setWindowMovePolicy);

    // === window.focus ================================
    colIdx = 0;
    label = new QLabel(i18n("Focus window:"), this);
    this_layout->addWidget(label, 3, colIdx++);

    js_focus = new QButtonGroup(this);
    js_focus->setExclusive(true);

    if (is_per_domain) {
        policy_btn = new QRadioButton(i18n("Use global"), this);
        policy_btn->setToolTip(i18n("Use setting from global policy."));
        js_focus->addButton(policy_btn, JSPolicies::inherit_policy);
        this_layout->addWidget(policy_btn, 3, colIdx++);
        this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);
    }/*end if*/

    policy_btn = new QRadioButton(i18n("Allow"), this);
    policy_btn->setToolTip(i18n("Allow scripts to focus the window."));
    js_focus->addButton(policy_btn, HtmlSettingsInterface::JSWindowFocusAllow);
    this_layout->addWidget(policy_btn, 3, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Ignore"), this);
    policy_btn->setToolTip(i18n("Ignore attempts of scripts to focus the window. "
                                  "The web page will <i>think</i> it brought "
                                  "the focus to the window but the actual "
                                  "focus will remain unchanged."));
    js_focus->addButton(policy_btn, HtmlSettingsInterface::JSWindowFocusIgnore);
    this_layout->addWidget(policy_btn, 3, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    wtstr = i18n("Some websites set the focus to their browser window on their "
                 "own by using <i>window.focus()</i>. This usually leads to "
                 "the window being moved to the front interrupting whatever "
                 "action the user was dedicated to at that time. "
                 "This option specifies the treatment of such "
                 "attempts.");
    label->setToolTip(wtstr);
    connect(js_focus, &QButtonGroup::idClicked, this, &JSPoliciesFrame::setWindowFocusPolicy);

    // === window.status ================================
    colIdx = 0;
    label = new QLabel(i18n("Modify status bar text:"), this);
    this_layout->addWidget(label, 4, colIdx++);

    js_statusbar = new QButtonGroup(this);
    js_statusbar->setExclusive(true);

    if (is_per_domain) {
        policy_btn = new QRadioButton(i18n("Use global"), this);
        policy_btn->setToolTip(i18n("Use setting from global policy."));
        js_statusbar->addButton(policy_btn, JSPolicies::inherit_policy);
        this_layout->addWidget(policy_btn, 4, colIdx++);
        this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);
    }/*end if*/

    policy_btn = new QRadioButton(i18n("Allow"), this);
    policy_btn->setToolTip(i18n("Allow scripts to change the text of the status bar."));
    js_statusbar->addButton(policy_btn, HtmlSettingsInterface::JSWindowStatusAllow);
    this_layout->addWidget(policy_btn, 4, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    policy_btn = new QRadioButton(i18n("Ignore"), this);
    policy_btn->setToolTip(i18n("Ignore attempts of scripts to change the status bar text. "
                                  "The web page will <i>think</i> it changed "
                                  "the text but the actual text will remain "
                                  "unchanged."));
    js_statusbar->addButton(policy_btn, HtmlSettingsInterface::JSWindowStatusIgnore);
    this_layout->addWidget(policy_btn, 4, colIdx++);
    this_layout->addItem(new QSpacerItem(10, 0), 0, colIdx++);

    wtstr = i18n("Some websites change the status bar text by setting "
                 "<i>window.status</i> or <i>window.defaultStatus</i>, "
                 "thus sometimes preventing displaying the real URLs of hyperlinks. "
                 "This option specifies the treatment of such "
                 "attempts.");
    label->setToolTip(wtstr);
    connect(js_statusbar, &QButtonGroup::idClicked, this, &JSPoliciesFrame::setWindowStatusPolicy);
}

void JSPoliciesFrame::refresh()
{
    QRadioButton *button;
    button = static_cast<QRadioButton *>(js_popup->button(
            policies->window_open));
    if (button != nullptr) {
        button->setChecked(true);
    }
    button = static_cast<QRadioButton *>(js_resize->button(
            policies->window_resize));
    if (button != nullptr) {
        button->setChecked(true);
    }
    button = static_cast<QRadioButton *>(js_move->button(
            policies->window_move));
    if (button != nullptr) {
        button->setChecked(true);
    }
    button = static_cast<QRadioButton *>(js_focus->button(
            policies->window_focus));
    if (button != nullptr) {
        button->setChecked(true);
    }
    button = static_cast<QRadioButton *>(js_statusbar->button(
            policies->window_status));
    if (button != nullptr) {
        button->setChecked(true);
    }
}

void JSPoliciesFrame::setWindowOpenPolicy(int id)
{
    policies->window_open = id;
    emit changed();
}

void JSPoliciesFrame::setWindowResizePolicy(int id)
{
    policies->window_resize = id;
    emit changed();
}

void JSPoliciesFrame::setWindowMovePolicy(int id)
{
    policies->window_move = id;
    emit changed();
}

void JSPoliciesFrame::setWindowFocusPolicy(int id)
{
    policies->window_focus = id;
    emit changed();
}

void JSPoliciesFrame::setWindowStatusPolicy(int id)
{
    policies->window_status = id;
    emit changed();
}


/*
    SPDX-FileCopyrightText: 2002 to whoever created and edited this file before
    SPDX-FileCopyrightText: 2002 Leo Savernik <l.savernik@aon.at>
    Generalizing the policy dialog

*/

// Own
#include "policydlg.h"

// Qt
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>
// KDE
#include <KLocalizedString>
#include <kmessagebox.h>

// Local
#include "jspolicies.h"

PolicyDialog::PolicyDialog(JSPolicies *policies, QWidget *parent, const char *name)
    : QDialog(parent),
      policies(policies)
{
    setObjectName(name);
    setModal(true);
    setWindowTitle(i18nc("@title:window", "Domain-Specific Policies"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PolicyDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PolicyDialog::reject);
    okButton = buttonBox->button(QDialogButtonBox::Ok);

    QFrame *main = new QFrame(this);

    insertIdx = 1;    // index where to insert additional panels
    topl = new QVBoxLayout(main);
    topl->setContentsMargins(0, 0, 0, 0);

    QGridLayout *grid = new QGridLayout();
    topl->addLayout(grid);
    grid->setColumnStretch(1, 1);

    QLabel *l = new QLabel(i18n("&Host or domain name:"), main);
    grid->addWidget(l, 0, 0);

    le_domain = new QLineEdit(main);
    l->setBuddy(le_domain);
    grid->addWidget(le_domain, 0, 1);
    connect(le_domain, &QLineEdit::textChanged, this, &PolicyDialog::slotTextChanged);

    le_domain->setToolTip(i18n("Enter the name of a host (like www.kde.org) "
                                 "or a domain, starting with a dot (like .kde.org or .org)"));

    l_feature_policy = new QLabel(main);
    grid->addWidget(l_feature_policy, 1, 0);

    cb_feature_policy = new QComboBox(main);
    l_feature_policy->setBuddy(cb_feature_policy);
    policy_values << i18n("Use Global") << i18n("Accept") << i18n("Reject");
    cb_feature_policy->addItems(policy_values);
    grid->addWidget(cb_feature_policy, 1, 1);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(main);
    vLayout->addStretch(1);
    vLayout->addWidget(buttonBox);

    le_domain->setFocus();
    okButton->setEnabled(!le_domain->text().isEmpty());
}

PolicyDialog::FeatureEnabledPolicy PolicyDialog::featureEnabledPolicy() const
{
    return (FeatureEnabledPolicy)cb_feature_policy->currentIndex();
}

void PolicyDialog::slotTextChanged(const QString &text)
{
    okButton->setEnabled(!text.isEmpty());
}

void PolicyDialog::setDisableEdit(bool state, const QString &text)
{
    le_domain->setText(text);

    le_domain->setEnabled(state);

    if (state) {
        cb_feature_policy->setFocus();
    }
}

void PolicyDialog::refresh()
{
    FeatureEnabledPolicy pol;

    if (policies->isFeatureEnabledPolicyInherited()) {
        pol = InheritGlobal;
    } else if (policies->isFeatureEnabled()) {
        pol = Accept;
    } else {
        pol = Reject;
    }
    cb_feature_policy->setCurrentIndex(pol);
}

void PolicyDialog::setFeatureEnabledLabel(const QString &text)
{
    l_feature_policy->setText(text);
}

void PolicyDialog::setFeatureEnabledWhatsThis(const QString &text)
{
    cb_feature_policy->setToolTip(text);
}

void PolicyDialog::addPolicyPanel(QWidget *panel)
{
    topl->insertWidget(insertIdx++, panel);
}

QString PolicyDialog::featureEnabledPolicyText() const
{
    int pol = cb_feature_policy->currentIndex();
    if (pol >= 0 && pol < 3) { // Keep in sync with FeatureEnabledPolicy
        return policy_values[pol];
    } else {
        return QString();
    }
}

void PolicyDialog::accept()
{
    if (le_domain->text().isEmpty()) {
        KMessageBox::information(nullptr, i18n("You must first enter a domain name."));
        return;
    }

    FeatureEnabledPolicy pol = (FeatureEnabledPolicy)
                               cb_feature_policy->currentIndex();
    if (pol == InheritGlobal) {
        policies->inheritFeatureEnabledPolicy();
    } else if (pol == Reject) {
        policies->setFeatureEnabled(false);
    } else {
        policies->setFeatureEnabled(true);
    }
    QDialog::accept();
}


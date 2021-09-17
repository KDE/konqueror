/*
    SPDX-FileCopyrightText: 2002 Leo Savernik <l.savernik@aon.at>
    Derived from jsopt.cpp, code copied from there is copyrighted to its
    respective owners.

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "policies.h"

// KDE
#include <kconfiggroup.h>

// == class Policies ==

Policies::Policies(KSharedConfig::Ptr config, const QString &group,
                   bool global, const QString &domain, const QString &prefix,
                   const QString &feature_key) :
    is_global(global), config(config), groupname(group),
    prefix(prefix), feature_key(feature_key)
{

    if (is_global) {
        this->prefix.clear();   // global keys have no prefix
    }/*end if*/
    setDomain(domain);
}

Policies::~Policies()
{
}

void Policies::setDomain(const QString &domain)
{
    if (is_global) {
        return;
    }
    this->domain = domain.toLower();
    groupname = this->domain; // group is domain in this case
}

void Policies::load()
{
    KConfigGroup cg(config, groupname);

    QString key = prefix + feature_key;
    if (cg.hasKey(key)) {
        feature_enabled = cg.readEntry(key, false);
    } else {
        feature_enabled = is_global ? true : INHERIT_POLICY;
    }
}

void Policies::defaults()
{
    feature_enabled = is_global ? true : INHERIT_POLICY;
}

void Policies::save()
{
    KConfigGroup cg(config, groupname);

    QString key = prefix + feature_key;
    if (feature_enabled != INHERIT_POLICY) {
        cg.writeEntry(key, (bool)feature_enabled);
    } else {
        cg.deleteEntry(key);
    }

    // don't do a config->sync() here for sake of efficiency
}

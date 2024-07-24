/*
    SPDX-FileCopyrightText: 2002 Leo Savernik <l.savernik@aon.at>
    Derived from jsopt.h, code copied from there is copyrighted to its
    respective owners.

    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
    Merge JSPolicies with obsolete Policies class

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef JSPOLICIES_H
#define JSPOLICIES_H

#include <QGroupBox>

#include <htmlextension.h>
#include <htmlsettingsinterface.h>
#include <KSharedConfig>

class QButtonGroup;

/**
 * @short Contains all the JavaScript policies and methods for their manipulation.
 *
 * This class provides access to the JavaScript policies.
 *
 * @author Leo Savernik
 * @author Stefano Crocco
 */
class JSPolicies
{
public:
    /**
     * constructor
     * @param config configuration to initialize this instance from
     * @param group config group to use if this instance contains the global
     *    policies (global == true)
     * @param global true if this instance contains the global policy settings,
     *    false if this instance contains policies specific for a domain.
     * @param domain name of the domain this instance is used to configure the
     *    policies for (case insensitive, ignored if global == true)
     */
    JSPolicies(KSharedConfig::Ptr config, const QString &group, bool global,
               const QString &domain = QString());

    /**
     * dummy constructor to make QMap happy.
     *
     * Never construct an object by using this.
     * @internal
     */
    JSPolicies();

    ~JSPolicies();

    /**
     * Returns true if this is the global policies object
     */
    bool isGlobal() const
    {
        return is_global;
    }

    /** sets a new domain for this policy
     * @param domain domain name, will be converted to lowercase
     */
    void setDomain(const QString &domain);

    /**
     * Returns whether the "feature enabled" policy is inherited.
     */
    bool isFeatureEnabledPolicyInherited() const
    {
        return feature_enabled == inherit_policy;
    }

    /** inherits "feature enabled" policy */
    void inheritFeatureEnabledPolicy()
    {
        feature_enabled = inherit_policy;
    }

    /**
     * Returns whether this feature is enabled.
     *
     * This will return an illegal value if isFeatureEnabledPolicyInherited
     * is true.
     */
    bool isFeatureEnabled() const
    {
        return (bool)feature_enabled;
    }

    /**
     * Enables/disables this feature
     * @param on true will enable it, false disable it
     */
    void setFeatureEnabled(int on)
    {
        feature_enabled = on;
    }

    /**
     * Returns whether the WindowOpen policy is inherited.
     */
    bool isWindowOpenPolicyInherited() const
    {
        return window_open == inherit_policy;
    }

    /**
     * Returns the current value of the WindowOpen policy.
     *
     * This will return an illegal value if isWindowOpenPolicyInherited is
     * true.
     */
    HtmlSettingsInterface::JSWindowOpenPolicy windowOpenPolicy() const
    {
        return static_cast<HtmlSettingsInterface::JSWindowOpenPolicy>(window_open);
    }

    /**
     * Returns whether the WindowResize policy is inherited.
     */
    bool isWindowResizePolicyInherited() const
    {
        return window_resize == inherit_policy;
    }
    /**
     * Returns the current value of the WindowResize policy.
     *
     * This will return an illegal value if isWindowResizePolicyInherited is
     * true.
     */
    HtmlSettingsInterface::JSWindowResizePolicy windowResizePolicy() const
    {
        return static_cast<HtmlSettingsInterface::JSWindowResizePolicy>(window_resize);
    }

    /**
     * Returns whether the WindowMove policy is inherited.
     */
    bool isWindowMovePolicyInherited() const
    {
        return window_move == inherit_policy;
    }
    /**
     * Returns the current value of the WindowMove policy.
     *
     * This will return an illegal value if isWindowMovePolicyInherited is
     * true.
     */
    HtmlSettingsInterface::JSWindowMovePolicy windowMovePolicy() const
    {
        return static_cast<HtmlSettingsInterface::JSWindowMovePolicy>(window_move);
    }

    /**
     * Returns whether the WindowFocus policy is inherited.
     */
    bool isWindowFocusPolicyInherited() const
    {
        return window_focus == inherit_policy;
    }
    /**
     * Returns the current value of the WindowFocus policy.
     *
     * This will return an illegal value if isWindowFocusPolicyInherited is
     * true.
     */
    HtmlSettingsInterface::JSWindowFocusPolicy windowFocusPolicy() const
    {
        return static_cast<HtmlSettingsInterface::JSWindowFocusPolicy>(window_focus);
    }

    /**
     * Returns whether the WindowStatus policy is inherited.
     */
    bool isWindowStatusPolicyInherited() const
    {
        return window_status == inherit_policy;
    }
    /**
     * Returns the current value of the WindowStatus policy.
     *
     * This will return an illegal value if isWindowStatusPolicyInherited is
     * true.
     */
    HtmlSettingsInterface::JSWindowStatusPolicy windowStatusPolicy() const
    {
        return static_cast<HtmlSettingsInterface::JSWindowStatusPolicy>(window_status);
    }

    /**
     * (re)loads settings from configuration file given in the constructor.
     */
    void load();
    /**
     * saves current settings to the configuration file given in the constructor
     */
    void save();
    /**
     * restores the default settings
     */
    void defaults();

private:
    // one of HtmlSettingsInterface::JSWindowOpenPolicy or inherit_policy
    unsigned int window_open;
    // one of HtmlSettingsInterface::JSWindowResizePolicy or inherit_policy
    unsigned int window_resize;
    // one of HtmlSettingsInterface::JSWindowMovePolicy or inherit_policy
    unsigned int window_move;
    // one of HtmlSettingsInterface::JSWindowFocusPolicy or inherit_policy
    unsigned int window_focus;
    // one of HtmlSettingsInterface::JSWindowStatusPolicy or inherit_policy
    unsigned int window_status;

    static constexpr uint inherit_policy = 32767;
    // true or false or inherit_policy
    unsigned int feature_enabled;

    bool is_global;
    KSharedConfig::Ptr config;
    QString groupname;
    QString domain;
    QString prefix;
    QString feature_key;

    friend class JSPoliciesFrame; // for changing policies
};

/**
 * @short Provides a framed widget with controls for the JavaScript policy settings.
 *
 * This widget contains controls for changing all JavaScript policies
 * except the JavaScript enabled policy itself. The rationale behind this is
 * that the enabled policy be separate from the rest in a prominent
 * place.
 *
 * It is suitable for the global policy settings as well as for the
 * domain-specific settings.
 *
 * The difference between global and domain-specific is the existence of
 * a special inheritance option in the latter case. That way domain-specific
 * policies can inherit their value from the global policies.
 *
 * @author Leo Savernik
 */
class JSPoliciesFrame : public QGroupBox
{
    Q_OBJECT
public:
    /**
     * constructor
     * @param policies associated object containing the policy values. This
     *    object will be updated accordingly as the settings are changed.
     * @param title title for group box
     * @param parent parent widget
     */
    JSPoliciesFrame(JSPolicies *policies, const QString &title, QWidget *parent = nullptr);

    ~JSPoliciesFrame() override = default;

    /**
     * updates the controls to resemble the status of the underlying
     * JSPolicies object.
     */
    void refresh();

    /**
     * (re)loads settings from configuration file given in the constructor.
     */
    void load()
    {
        policies->load();
        refresh();
    }

    /**
     * saves current settings to the configuration file given in the constructor
     */
    void save()
    {
        policies->save();
    }

    /**
     * restores the default settings
     */
    void defaults()
    {
        policies->defaults();
        refresh();
    }

Q_SIGNALS:
    /**
     * emitted every time an option has been changed
     */
    void changed();

private Q_SLOTS:
    void setWindowOpenPolicy(int id);
    void setWindowResizePolicy(int id);
    void setWindowMovePolicy(int id);
    void setWindowFocusPolicy(int id);
    void setWindowStatusPolicy(int id);

private:

    JSPolicies *policies;
    QButtonGroup *js_popup;
    QButtonGroup *js_resize;
    QButtonGroup *js_move;
    QButtonGroup *js_focus;
    QButtonGroup *js_statusbar;
};

#endif      // __JSPOLICIES_H__

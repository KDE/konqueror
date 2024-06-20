//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef KONQ_SETTINGSBASE_H
#define KONQ_SETTINGSBASE_H

#include <libkonqsettings_export.h>

#include <KConfigSkeleton>

#include <QDebug>

namespace Konq {

/**
 * @todo write docs
 */
class LIBKONQSETTINGS_EXPORT  SettingsBase : public KConfigSkeleton
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param configname TODO
     * @param parent TODO
     */
    SettingsBase(const QString& configname={}, QObject* parent=nullptr);

    /**
     * Constructor
     *
     * @param config TODO
     * @param parent TODO
     */
    SettingsBase(KSharedConfig::Ptr config, QObject* parent=nullptr);

    /**
     * @brief The possible ways to deal with a cookie
     * @internal
     * It's important that Unknown has value 0.
     *
     * @warning If the last enum value is changed, intToAdvice() needs to be changed accordingly
     * @endinternal
     */
    enum class CookieAdvice {
        Unknown=0, ///< No action has been decided
        Accept, ///< Accept the cookie
        AcceptForSession, ///< Accept the cookie, but discard it when the application is closed
        Reject, ///<Reject the cookie
        Ask ///< Ask the user what to do
    };

    /**
     * @brief Returns the value of the `CookieGlobalAdvice` setting converted to a CookieAdvice
     *
     * This works as Konq::Settings::cookieGlobalAdviceRaw except that it converts the value to a CookieAdvice
     *
     * @return The CookieAdvice corresponding to the setting or CookieAdvice::Unknown if the setting is invalid
     * @note For implementation reasons, this isn't a static function
     */
    CookieAdvice cookieGlobalAdvice() const;

    /**
     * @brief Sets the value of the `CookieGlobalAdvice` setting
     *
     * This works as Konq::Settings::setCookieGlobalAdviceRaw except that it takes a CookieAdvice
     *
     * @param advice the new value of the setting
     * @note For implementation reasons, this isn't a static function
     */
    void setCookieGlobalAdvice(CookieAdvice advice);

    /**
     * @brief Returns the value of the `CookieDomainAdvice` setting converted to a QHash
     *
     * This works as Konq::Settings::cookieDomainAdviceRaw except that it converts the value to QHash
     *
     * @return The value of the setting in a QHash, where the keys are the domain and the values are the advice
     * @note For implementation reasons, this isn't a static function
     */
    QHash<QString, CookieAdvice> cookieDomainAdvice() const;

    /**
     * @brief Sets the value of the `CookieDomainAdvice` setting
     *
     * This works as Konq::Settings::setCookieDomainAdviceRaw except that it takes a QHash having the domains
     * as keys and the advice as values.
     *
     * @param advice the new value of the setting
     * @note For implementation reasons, this isn't a static function
     */
    void setCookieDomainAdvice(const QHash<QString, CookieAdvice> &advice);

    /**
     * @brief Helper function which converts an int to a CookieAdvice
     *
     * @param value the value to convert
     * @param defaultValue the value to return if @p value doesn't correspond to any advice value
     * @return the advice corresponding to the number @p value or @p defaultValue if it isn't a valid advice value
     */
    static CookieAdvice intToAdvice(int value, CookieAdvice defaultValue = CookieAdvice::Unknown);

    /**
     * @brief Returns a list of the certificate errors the user chose to always ignore for a given URL
     *
     * @param url the URL to retrieve the certificate errors for
     * @return a list of the certificate errors the user chose to always ignore for @p url
     * @note For implementation reasons, this isn't a static function
     * @internal This can't be managed by kconfigxt because each URL is a separate entry in the configuration file
     */
    QList<int> certificateExceptions(const QString &url) const;

    /**
     * @brief Adds a certificate error code to ignore for the given URL
     *
     * @param url the URL
     * @param exception the error code to add
     * @warning This doesn't save the configuration. You need to explicitly call save() to do so
     * @note For implementation reasons, this isn't a static function
     * @internal This can't be managed by kconfigxt because each URL is a separate entry in the configuration file
     */
    bool addCertificateException(const QString &url, int exception);

    /**
     * @brief Helper function which calls a lambda with `isDefaults` turned on then restores it to the orignal value
     *
     * This is usually used from a KCM defaults method:
     * <code>
     * void MyKCM::defaults() {
     *     settingsBase->withDefaults([this]{load();});
     * }
     * </code>
     * @param lambda the lambda to call
     */
    template <typename Functor>
    void withDefaults(Functor lambda);

    /**
     * @brief The argument last passed to useDefaults()
     *
     * @return `true` if useDefaults() was last called with `true` or `false` if it was last called with `false` or
     * if it hasn't been called as yet
     */
    bool isUsingDefaults() const {return m_isUsingDefaults;}

    /**
     * @brief Override of KConfigSkeleton::usrRead
     *
     * It handles reading the certificate exceptions settings
     */
    void usrRead() override;

    /**
     * @brief Override of KConfigSkeleton::usrSave
     *
     * It handles saving the certificate exceptions settings
     */
    bool usrSave() override;

    /**
     * @brief Override of KConfigSkeleton::usrSetDefaults
     *
     * It handles resetting the certificate exceptions settings to their default values
     */
    void usrSetDefaults() override;

    /**
     * @brief Override of KConfigSkeleton::usrUseDefaults
     *
     * It works as the base class version but it stores the argument passed to it
     * so that it can be retrieved using isUsingDefaults()
     */
    bool usrUseDefaults(bool b) override;

private:
    /**
     * @brief Helper function which returns the item managing the global cookie advice setting
     * @return the item managing the global cookie advice setting
     */
    ItemInt* cookieGlobalAdviceItem() const;

    /**
     * @brief Helper function which returns the item managing the domain cookie advice setting
     * @return the item managing the domain cookie advice setting
     */
    ItemString* cookieDomainAdviceItem() const;

    /**
     * @brief Helper function which returns the configuration group containing the certificate exceptions
     * @return the configuration group containing the certificate exceptions
     */
    KConfigGroup certificateExceptionsGroup() const;

private:

    bool m_isUsingDefaults = false; //!< Keeps track of whether or not we're inside a call to withDefaults()

    /**
     * @brief Struct containing information about certificate exceptions settings
     */
    struct CertificateExceptionsSettings {
        QHash<QString, QList<int>> data; //!< The exceptions for each URL
        bool dirty = false; //!< Whether the exceptions have been modified since they were read or were last saved
    };

    CertificateExceptionsSettings m_certificateExceptions; //!< Certificate error settings
};

template<typename Functor> void Konq::SettingsBase::withDefaults(Functor lambda)
{
    bool old = useDefaults(true);
    lambda();
    useDefaults(old);
}

LIBKONQSETTINGS_EXPORT QDebug operator<<(QDebug dbg, SettingsBase::CookieAdvice) ;

}

#endif // KONQ_SETTINGSBASE_H

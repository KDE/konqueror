/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQ_URLACTIONS_H
#define KONQ_URLACTIONS_H

#include <libkonq_export.h>

#include <QList>
#include <QDebug>

namespace Konq {
    /**
     * @brief Enum describing actions which can be performed with an URL
     */
    enum class UrlAction {
        UnknownAction, //!< A default value to use, for example, when no action has been determined
        DoNothing, //!< Take no action
        Save, //!< Save the URL to disk
        Embed, //!< Embed the URL in Konqueror
        Open, //!< Open the URL in an external application
        Execute //!< Execute the URL
    };

    QDebug LIBKONQ_EXPORT operator<<(QDebug dbg, Konq::UrlAction action);

    /**
     * @brief Class representing a list of allowed UrlActions
     */
    class LIBKONQ_EXPORT AllowedUrlActions : private QList<UrlAction> {
    public:
        /**
         * @brief Default constructor
         *
         * It constructs an object allowing all the actions
         */
        AllowedUrlActions();

        /**
         * @brief Constructor creating an object allowing only the given actions
         * @param actions the actions to allow
         */
        AllowedUrlActions(std::initializer_list<UrlAction> actions);

        /**
         * @brief Whether the given action is allowed
         * @param action the action to test. It shouldn't be UrlAction::UnknownAction
         * @return `true` if @p action is allowed and `false` otherwise. If @p action is
         * UrlAction::UnknownAction, this function will always return `false`, as
         * UrlAction::UnknownAction is a placeholder and not a real action
         */
        bool isAllowed(Konq::UrlAction action) const;

        /**
         * @brief Whether there's only one allowed action choice
         *
         * This function assumes that UrlAction::DoNothing is always an allowed choice,
         * so it doesn't take it into account when deciding whether there's only one
         * allowed choice
         *
         * @return `true` if there's only a possible action choice and `false` otherwise
         */
        bool isForced() const;

        /**
         * @brief Whether the given action is the only allowed action choice
         *
         * As in isForced(), this function assumes that UrlAction::DoNothing is always allowed
         * and doesn't take it into account.
         *
         * @param action the action to test
         * @return `true` if @p action is the only allowed action choice (besides
         * UrlAction::DoNothing) and `false` otherwise. If @p action is UrlAction::DoNothing,
         * it will return `true` only if no other action is allowed
         */
        bool isForced(Konq::UrlAction action) const;

        /**
         * @brief The only allowed action choice, if any
         * @return The only allowed action if there's only one (besides UrlAction::DoNothing)
         * and UrlAction::UnknownAction if there are more than one allowed actions
         */
        Konq::UrlAction forcedAction() const;

        /**
         * @brief Makes the given action the only allowed action
         *
         * @param action the action which should become the only allowed action. If it is UrlAction::UnknownAction,
         * nothing is done
         * @return `true` if @p action isn't UrlAction::UnknownAction and `false` otherwise
         */
        bool force(Konq::UrlAction action);

        /**
         * @brief Marks the given action as allowed or not allowed
         * @param action the action. It doesn't make sense to pass UrlAction::UnknownAction or
         * UrlAction::DoNothing
         * @param allowed whether the actions should be allowed or not
         * @note if @p action is UrlAction::UnknownAction or UrlAction::DoNothing, nothing is
         * done
         */
        void allow(Konq::UrlAction action, bool allowed);

        friend QDebug operator<<(QDebug dbg, const Konq::AllowedUrlActions &actions);
    };

    QDebug LIBKONQ_EXPORT operator<<(QDebug dbg, const AllowedUrlActions &actions);
}


#endif //KONQ_URLACTIONS_H

/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2009 Christoph Feck <christoph@maxiom.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQANIMATEDLOGO_P_H
#define KONQANIMATEDLOGO_P_H

#include <KAnimatedButton>

class QToolBar;

/**
 * @brief Class representing the animated button shown to the right of the location bar
 */
class KonqAnimatedLogo : public KAnimatedButton
{
    Q_OBJECT

public:
    /**
     * @brief Creates an animated logo button which follows the toolbar icon size
     *
     * @param parent the parent widget
     */
    KonqAnimatedLogo(QWidget *parent = nullptr);

protected:

    /**
     * @brief Override of `KonqAnimatedLogo::changeEvent()`
     *
     * It connects and disconnects from parent's signals when the parent changes.
     * It always calls the base class version of the function.
     *
     * @param event the event
     */
    void changeEvent(QEvent *event) override;

private Q_SLOTS:

    /**
     * @brief Updates the path of the file containing the animation so that it
     * has the given size
     *
     * @param size the new size of the button
     */
    void setAnimatedLogoSize(const QSize &size);

private:

    /**
     * @brief Connects to a toolbar's signals
     *
     * It also calls setAnimatedLogoSize() so that the animation size matches the
     * toolbar's icon size
     *
     * @param bar the toolbar to connect to. It's assumed that this will be the toolbar
     * where the button is
     */
    void connectToToolBar(QToolBar *bar);
};

#endif // KONQANIMATEDLOGO_P_H

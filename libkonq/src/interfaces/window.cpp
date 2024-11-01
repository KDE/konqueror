//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "window.h"
#include "interfaces/common.h"

using namespace KonqInterfaces;

KonqInterfaces::TabBarContextMenu::TabBarContextMenu(QWidget* parent) : QMenu(parent)
{
}


Window::~Window() = default;

Window::Window(QObject* parent) : QObject(parent)
{
}

Window* KonqInterfaces::Window::window(QObject* obj)
{
    return as<Window>(obj);
}


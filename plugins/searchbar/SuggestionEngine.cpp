/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Fredy Yanardi <fyanardi@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "SuggestionEngine.h"

#include <kservice.h>

#include "searchbar_debug.h"

SuggestionEngine::SuggestionEngine(const QString &engineName, QObject *parent)
    : QObject(parent),
      m_engineName(engineName)
{
    // First get the suggestion request URL for this engine
    KService::Ptr service = KService::serviceByDesktopPath(QStringLiteral("searchproviders/%1.desktop").arg(m_engineName));

    if (service) {
        const QString suggestionURL = service->property(QStringLiteral("Suggest")).toString();
        if (!suggestionURL.isNull() && !suggestionURL.isEmpty()) {
            m_requestURL = suggestionURL;
        } else {
            qCWarning(SEARCHBAR_LOG) << "Missing property [Suggest] for suggestion engine: " + m_engineName;
        }
    }
}

QString SuggestionEngine::requestURL() const
{
    return m_requestURL;
}

QString SuggestionEngine::engineName() const
{
    return m_engineName;
}


/*
    SPDX-FileCopyrightText: 2009 Jakub Wieczorek <faw217@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA
*/

#ifndef OPENSEARCHWRITER_H
#define OPENSEARCHWRITER_H

#include <QXmlStreamWriter>

class QIODevice;

class OpenSearchEngine;

class OpenSearchWriter : public QXmlStreamWriter
{
public:
    OpenSearchWriter();

    bool write(QIODevice *device, OpenSearchEngine *engine);

private:
    void write(OpenSearchEngine *engine);
};

#endif


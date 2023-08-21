/* This file is part of the KDE project

    SPDX-FileCopyrightText: 2005 Ivor Hewitt <ivor@kde.org>
    SPDX-FileCopyrightText: 2008 Maksim Orlovich <maksim@kde.org>
    SPDX-FileCopyrightText: 2008 Vyacheslav Tokarev <tsjoker@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "webengine_filter.h"

#include <QHash>
#include <QBitArray>

// rolling hash parameters
#define HASH_P (1997)
#define HASH_Q (17509)
// HASH_MOD = (HASH_P^7) % HASH_Q
#define HASH_MOD (523)


using namespace KDEPrivate;

// Updateable Multi-String Matcher based on Rabin-Karp's algorithm
class StringsMatcher {
public:
    // add filter to matching set
    void addString(const QString& pattern)
    {
        if (pattern.length() < 8) {
            // handle short string differently
            shortStringFilters.append(pattern);
        } else {
            // use modified Rabin-Karp's algorithm with 8-length string hash
            // i.e. store hash of first 8 chars in the HashMap for fast look-up
            stringFilters.append(pattern);
            int ind = stringFilters.size() - 1;
            int current = 0;

            // compute hash using rolling hash
            // hash for string: x0,x1,x2...xn-1 will be:
            // (p^(n-1)*x0 + p^(n-2)*x1 + ... + p * xn-2 + xn-1) % q
            // where p and q some wisely-chosen integers
            /*for (int k = 0; k < 8; ++k)*/
            int len = pattern.length();
            for (int k = len - 8; k < len; ++k)
                current = (current * HASH_P + pattern[k].unicode()) % HASH_Q;

            // insert computed hash value into HashMap
            QHash<int, QVector<int> >::iterator it = stringFiltersHash.find(current + 1);
            if (it == stringFiltersHash.end()) {
                QVector<int> list;
                list.append(ind);
                stringFiltersHash.insert(current + 1, list);
                fastLookUp.setBit(current);
            } else {
                it->append(ind);
            }
        }
    }


    // check if string match at least one string from matching set
    bool isMatched(const QString& str, QString *by = nullptr) const
    {
        // check short strings first
        for (int i = 0; i < shortStringFilters.size(); ++i) {
            if (str.contains(shortStringFilters[i]))
            {
                if (by != nullptr) *by = shortStringFilters[i];
                return true;
            }
        }

        int len = str.length();
        int k;

        int current = 0;
        int next = 0;
        // compute hash for first 8 characters
        for (k = 0; k < 8 && k < len; ++k)
            current = (current * HASH_P + str[k].unicode()) % HASH_Q;

        QHash<int, QVector<int> >::const_iterator hashEnd = stringFiltersHash.end();
        // main Rabin-Karp's algorithm loop
        for (k = 7; k < len; ++k, current = next) {
            // roll the hash if not at the end
            // (calculate hash for the next iteration)
            if (k + 1 < len)
                next = (HASH_P * ((current + HASH_Q - ((HASH_MOD * str[k - 7].unicode()) % HASH_Q)) % HASH_Q) + str[k + 1].unicode()) % HASH_Q;

            if (!fastLookUp.testBit(current))
                continue;

            // look-up the hash in the HashMap and check all strings
            QHash<int, QVector<int> >::const_iterator it = stringFiltersHash.find(current + 1);

            // check possible strings
            if (it != hashEnd) {
                for (int j = 0; j < it->size(); ++j) {
                    int index = it->value(j);
                    // check if we got simple string or REs prefix
                    if (index >= 0) {
                        int flen = stringFilters[index].length();
                        if (k - flen + 1 >= 0 && stringFilters[index] == str.midRef(k - flen + 1 , flen))
                        {
                            if (by != nullptr) *by = stringFilters[index];
                            return true;
                        }
                    } else {
                        index = -index - 1;
                        int flen = rePrefixes[index].length();
                        if (k - 8 + flen < len && rePrefixes[index] == str.midRef(k - 7, flen))
                        {
                            int remStart = k - 7 + flen;
                            QString remainder = QString::fromRawData(str.unicode() + remStart,
                                                                    str.length() - remStart);
                            QRegularExpression exactMatchRegexp = QRegularExpression(QRegularExpression::anchoredPattern(reFilters[index].pattern()));
                            if (exactMatchRegexp.match(remainder).hasMatch()) {
                                if (by != nullptr) *by = rePrefixes[index]+reFilters[index].pattern();
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    // add filter to matching set with wildcards (*,?) in it
    void addWildedString(const QString& prefix, const QRegularExpression& rx)
    {
        rePrefixes.append(prefix);
        reFilters.append(rx);
        int index = -rePrefixes.size();

        int current = 0;
        for (int k = 0; k < 8; ++k)
            current = (current * HASH_P + prefix[k].unicode()) % HASH_Q;

        // insert computed hash value into HashMap
        QHash<int, QVector<int> >::iterator it = stringFiltersHash.find(current + 1);
        if (it == stringFiltersHash.end()) {
            QVector<int> list;
            list.append(index);
            stringFiltersHash.insert(current + 1, list);
            fastLookUp.setBit(current);
        } else {
            it->append(index);
        }
    }

    void clear()
    {
        stringFilters.clear();
        shortStringFilters.clear();
        reFilters.clear();
        rePrefixes.clear();
        stringFiltersHash.clear();
        fastLookUp.resize(HASH_Q);
        fastLookUp.fill(0, 0, HASH_Q);
    }

private:
    QVector<QString> stringFilters;
    QVector<QString> shortStringFilters;
    QVector<QRegularExpression> reFilters;
    QVector<QString> rePrefixes;
    QBitArray fastLookUp;

    QHash<int, QVector<int> > stringFiltersHash;
};


// We only want a subset of features of wildcards -- just the 
// star, so we can't use QRegularExpression::wildcardToRegularExpression
static QRegularExpression fromAdBlockWildcard(const QString& wcStr) {
    QString pattern;
    for (const QChar &c : wcStr) {
        if (c == QLatin1Char('*')) {
            pattern.append(QLatin1String(".*"));
        } else if (c == QLatin1Char('?') || c == QLatin1Char('[') || c == QLatin1Char(']')) {
            pattern.append(QLatin1String("\\") + c);
        } else {
            pattern.append(c);
        }
    }
    return QRegularExpression(pattern);
}

FilterSet::FilterSet()
    :stringFiltersMatcher(new StringsMatcher)
{
}

FilterSet::~FilterSet()
{
    delete stringFiltersMatcher;
}

void FilterSet::addFilter(const QString& filterStr)
{
    QString filter = filterStr;

    /** ignore special lines starting with "[", "!", "&", or "#" or contain "#" (comments or features are not supported by KHTML's AdBlock */
    QChar firstChar = filter.at(0);
    if (firstChar == QLatin1Char('[') || firstChar == QLatin1Char('!') || firstChar == QLatin1Char('&') || firstChar == QLatin1Char('#') || filter.contains(QLatin1Char('#')))
        return;

    // Strip leading @@
    int first = 0;
    int last  = filter.length() - 1;
    if (filter.startsWith(QLatin1String("@@")))
        first = 2;

    // Strip options, we ignore them for now.
    // TODO: Add support for filters with options. See #310230.
    int dollar = filter.lastIndexOf(QLatin1Char('$'));
    if (dollar != -1) {
        return;
    }

    // Perhaps nothing left?
    if (first > last)
        return;

    filter = filter.mid(first, last - first + 1);

    // Is it a regexp filter?
    if (filter.length()>2 && filter.startsWith(QLatin1Char('/')) && filter.endsWith(QLatin1Char('/')))
    {
        QString inside = filter.mid(1, filter.length()-2);
        QRegularExpression rx(inside);
        reFilters.append(rx);
//         qCDebug(WEBENGINEPART_LOG) << "R:" << inside;
    }
    else
    {
        // Nope, a wildcard one.
        // Note: For these, we also need to handle |.

        // Strip wildcards at the ends
        first = 0;
        last  = filter.length() - 1;

        while (first < filter.length() && filter[first] == QLatin1Char('*'))
            ++first;

        while (last >= 0 && filter[last] == QLatin1Char('*'))
            --last;

        if (first > last)
            filter = QStringLiteral("*"); // erm... Well, they asked for it.
        else
            filter = filter.mid(first, last - first + 1);

        // Now, do we still have any wildcard stuff left?
        if (filter.contains(QLatin1String("*")))
        {
            // check if we can use RK first (and then check full RE for the rest) for better performance
            int aPos = filter.indexOf('*');
            if (aPos < 0)
                aPos = filter.length();
            if (aPos > 7) {
                QRegularExpression rx = fromAdBlockWildcard(filter.mid(aPos) + QLatin1Char('*'));
                // We pad the final r.e. with * so we can check for an exact match
                stringFiltersMatcher->addWildedString(filter.mid(0, aPos), rx);
            } else {
                QRegularExpression rx = fromAdBlockWildcard(filter);
                reFilters.append(rx);
            }
        }
        else
        {
            // Fast path
            stringFiltersMatcher->addString(filter);
        }
    }
}

bool FilterSet::isUrlMatched(const QString& url)
{
    if (stringFiltersMatcher->isMatched(url))
        return true;

    for (int c = 0; c < reFilters.size(); ++c)
    {
        if (url.contains(reFilters[c]))
            return true;
    }

    return false;
}

QString FilterSet::urlMatchedBy(const QString& url)
{
    QString by;

    if (stringFiltersMatcher->isMatched(url, &by))
        return by;

    for (int c = 0; c < reFilters.size(); ++c)
    {
        if (url.contains(reFilters[c]))
        {
            by = reFilters[c].pattern();
            break;
        }
    }

    return by;
}

void FilterSet::clear()
{
    reFilters.clear();
    stringFiltersMatcher->clear();
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;

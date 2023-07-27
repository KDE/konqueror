/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TEXTEXTENSION_H
#define TEXTEXTENSION_H

#include "libkonq_export.h"

#include <KFind>
#include <QObject>
#include <memory>

namespace KParts {
class ReadOnlyPart;
}
class TextExtensionPrivate;

/**
 * @class TextExtension textextension.h
 *
 * @short An extension for KParts that allows to retrieve text from the part.
 *
 * For instance, the text-to-speech plugin uses this to speak the whole text
 * from the part or the selected text. The translation plugin uses it for
 * translating the selected text, and so on.
 *
 * @since 4.6
 */
class LIBKONQ_EXPORT TextExtension : public QObject
{
    Q_OBJECT
public:
    explicit TextExtension(KParts::ReadOnlyPart *parent);
    ~TextExtension() override;

    /**
     * Queries @p obj for a child object which inherits from this
     * TextExtension class.
     */
    static TextExtension *childObject(QObject *obj);

    enum Format { PlainText, HTML };

    /**
     * Returns true if the user selected text in the part.
     */
    virtual bool hasSelection() const;
    /**
     * Returns the selected text, in the requested format.
     * If the format is not supported, the part must return an empty string.
     */
    virtual QString selectedText(Format format) const;
    /**
     * Returns the complete text shown in the part, in the requested format.
     * If the format is not supported, the part must return an empty string.
     */
    virtual QString completeText(Format format) const;

    /**
     * Returns the number of pages, for parts who support the concept of pages.
     * Otherwise returns 0.
     */
    virtual int pageCount() const;
    /**
     * Returns the current page (between 0 and pageCount()-1),
     * for parts who support the concept of pages.
     * Otherwise returns 0.
     */
    virtual int currentPage() const;
    /**
     * Returns the text in a given page, in the requested format.
     */
    virtual QString pageText(Format format) const;

    /**
     * Returns true if @p string is found using the given @p options.
     *
     * If any text matches @p string, then it will be selected/highlighted.
     * To find the next matching text, simply call this function again with the
     * same search text until it returns false.
     *
     * To clear a selection, just pass an empty string.
     *
     * Note that parts that implement this extension might not support all the
     * options available in @ref KFind::SearchOptions.
     */
    virtual bool findText(const QString &string, KFind::SearchOptions options) const;

    // for future extensions can be made via slots

Q_SIGNALS:
    /**
     * This signal is emitted when the selection changes.
     */
    void selectionChanged();

private:
    // for future extensions
    std::unique_ptr<TextExtensionPrivate> const d;
};

#endif /* TEXTEXTENSION_H */

/*
    SPDX-FileCopyrightText: 2002 Leo Savernik <l.savernik@aon.at>
    Derived from jsopts.h and javaopts.h, code copied from there is
    copyrighted to its respective owners.

    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
    Merged JSDomainListView (previously in jsopts.h) and DomainListView

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DOMAINLISTVIEW_H
#define DOMAINLISTVIEW_H

#include <QGroupBox>
#include <QMap>

#include <kconfig.h>
#include <ksharedconfig.h>

class QTreeWidgetItem;
class QPushButton;

class QTreeWidget;

class JSPolicies;
class PolicyDialog;
class KJavaScriptOptions;

/**
 * @short Provides a list view of domains which policies are attached to.
 *
 * This class resembles a list view of domain names and some buttons to
 * manipulate it. You should use this widget if you need to manage domains
 * whose policies are described by (derivatives of) Policies objects.
 *
 * The contained widgets can be accessed by respective getters for
 * fine-tuning/customizing them afterwards.
 *
 * To use this class you have to derive your own and implement most
 * (all) of the protected methods. You need these to customize this widget
 * for its special purpose.
 *
 * @author Leo Savernik
 */
class DomainListView : public QGroupBox
{
    Q_OBJECT
public:
    /** Enumerates the available buttons.
      */
    enum PushButton {
        AddButton, ChangeButton, DeleteButton, ImportButton, ExportButton
    };

    /**
     * constructor
     * @param config configuration to read from and to write to
     * @param title title to be used for enclosing group box
     * @param parent parent widget
     * @param name internal name for debugging
     */
    DomainListView(KSharedConfig::Ptr config, const QString &group, KJavaScriptOptions *options, QWidget *parent);

    ~DomainListView() override;

    /**
     * clears the list view.
     */
//  void clear();

    /**
     * returns the list view displaying the domains
     */
    QTreeWidget *listView() const
    {
        return domainSpecificLV;
    }

    /**
     * returns the add push-button.
     *
     * Note: The add button already contains a default "what's this" text.
     */
    QPushButton *addButton() const
    {
        return addDomainPB;
    }

    /**
     * returns the change push-button.
     *
     * Note: The change button already contains a default "what's this" text.
     */
    QPushButton *changeButton() const
    {
        return changeDomainPB;
    }

    /**
     * returns the delete push-button.
     *
     * Note: The delete button already contains a default "what's this" text.
     */
    QPushButton *deleteButton() const
    {
        return deleteDomainPB;
    }

    /**
     * returns the import push-button.
     */
    QPushButton *importButton() const
    {
        return importDomainPB;
    }

    /**
     * returns the export push-button.
     */
    QPushButton *exportButton() const
    {
        return exportDomainPB;
    }

    /**
     * Initializes the list view with the given list of domains as well
     * as the domain policy map.
     *
     * This method may be called multiple times on a DomainListView instance.
     *
     * @param domainList given list of domains
     */
    void initialize(const QStringList &domainList);

    /**
     * saves the current state of all domains to the configuration object.
     * @param group the group the information is to be saved under
     * @param domainListKey the name of the key which the list of domains
     *    is stored under.
     */
    void save(const QString &group, const QString &domainListKey);

Q_SIGNALS:
    /**
     * indicates that a configuration has been changed within this list view.
     * @param state true if changed, false if not
     */
    void changed(bool state);

protected:
    /**
     * factory method for creating a new domain-specific policies object.
     *
     * Example:
     * <pre>
     * JavaPolicies *JavaDomainListView::createPolicies() {
     *   return new JavaPolicies(m_pConfig,m_groupname,false);
     * }
     * </pre>
     */
    JSPolicies *createPolicies();

    /**
     * factory method for copying a policies object.
     *
     * Derived classes must interpret the given object as the same type
     * as those created by createPolicies and return a copy of this very type.
     *
     * Example:
     * <pre>
     * JavaPolicies *JavaDomainListView::copyPolicies(Policies *pol) {
     *   return new JavaPolicies(*static_cast<JavaPolicies *>(pol));
     * }
     * </pre>
     * @param pol policies object to be copied
     */
    JSPolicies *copyPolicies(JSPolicies *pol);

    /**
     * allows derived classes to customize the policy dialog.
     *
     * The default implementation does nothing.
     * @param trigger triggered by which button
     * @param pDlg reference to policy dialog
     * @param copy policies object this dialog is used for changing. Derived
     *    classes can safely cast the @p copy object to the same type they
     *    returned in their createPolicies implementation.
     */
    void setupPolicyDlg(PushButton trigger, PolicyDialog& pDlg, JSPolicies* pol);

private Q_SLOTS:
    void addPressed();
    void changePressed();
    void deletePressed();
    void importPressed();
    void exportPressed();
    void updateButton();

protected:

    KSharedConfig::Ptr config;
    QString group;

    QTreeWidget *domainSpecificLV;

    QPushButton *addDomainPB;
    QPushButton *changeDomainPB;
    QPushButton *deleteDomainPB;
    QPushButton *importDomainPB;
    QPushButton *exportDomainPB;

    KJavaScriptOptions *options;

    typedef QMap<QTreeWidgetItem *, JSPolicies *> DomainPolicyMap;
    DomainPolicyMap domainPolicies;
};

#endif      // DOMAINLISTVIEW_H


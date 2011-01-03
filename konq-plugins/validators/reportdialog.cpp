/* This file is part of Validators
 *
 *  Copyright (C) 2008 by  Pino Toscano <pino@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include "reportdialog.h"

#include "tidy_validator.h"

#include <qheaderview.h>

#include <klocale.h>

static const int FrameNumberRole = Qt::UserRole + 1;

static bool compare_report_items(QTreeWidgetItem* a, QTreeWidgetItem* b)
{
  int val1 = a->data(0, FrameNumberRole).toInt();
  int val2 = b->data(0, FrameNumberRole).toInt();
  if (val1 != val2)
    return val1 < val2;
  val1 = a->text(2).toInt();
  val2 = b->text(2).toInt();
  if (val1 != val2)
    return val1 < val2;
  val1 = a->text(3).toInt();
  val2 = b->text(3).toInt();
  return val1 < val2;
}

QTreeWidgetItem* createItemFromReport(const TidyReport &report, const QIcon &icon, const QString &iconToolTip,
                                      const QString &frameName, int frameNumber)
{
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setIcon(0, icon);
  item->setText(1, frameName);
  item->setText(2, QString::number(report.line));
  item->setText(3, QString::number(report.col));
  item->setText(4, report.msg);
  item->setToolTip(0, iconToolTip);
  item->setData(0, FrameNumberRole, frameNumber);
  return item;
}

ReportDialog::ReportDialog(const QList<ValidationResult *> &results, QWidget* parent)
  : KDialog(parent)
{
  setButtons(KDialog::Close);
  setCaption(i18n("Validation Report"));

  m_ui.setupUi(mainWidget());
  mainWidget()->layout()->setMargin(0);
  QHeaderView *header = m_ui.reportsView->header();
  header->setResizeMode(0, QHeaderView::ResizeToContents);
  header->setResizeMode(1, QHeaderView::ResizeToContents);
  header->setResizeMode(2, QHeaderView::ResizeToContents);
  header->setResizeMode(3, QHeaderView::ResizeToContents);
  QList<QTreeWidgetItem *> items;
  int i = 0;
  Q_FOREACH (ValidationResult* res, results)
  {
    const KIcon errorIcon("dialog-error");
    const QString errorStatus = i18nc("Validation status", "Error");
    Q_FOREACH (const TidyReport &r, res->errors)
    {
      QTreeWidgetItem *item = createItemFromReport(
              r, errorIcon, errorStatus, res->frameName, i);
      items.append(item);
    }
    const KIcon warningIcon("dialog-warning");
    const QString warningStatus = i18nc("Validation status", "Warning");
    Q_FOREACH (const TidyReport &r, res->warnings)
    {
      QTreeWidgetItem *item = createItemFromReport(
              r, warningIcon, warningStatus, res->frameName, i);
      items.append(item);
    }
    const KIcon a11yWarningIcon("preferences-desktop-accessibility");
    const QString a11yWarningStatus = i18nc("Validation status", "Accessibility warning");
    Q_FOREACH (const TidyReport &r, res->accesswarns)
    {
      QTreeWidgetItem *item = createItemFromReport(
              r, a11yWarningIcon, a11yWarningStatus, res->frameName, i);
      items.append(item);
    }
    ++i;
  }
  qStableSort(items.begin(), items.end(), compare_report_items);
  m_ui.reportsView->addTopLevelItems(items);
  if (results.count() == 1)
    header->setSectionHidden(1, true);
}

QSize ReportDialog::sizeHint() const
{
  return QSize(500, 400);
}

#include "reportdialog.moc"

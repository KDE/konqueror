/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

/* Test Directory Scanning. Usually not build. */

#include <stdio.h>

#include <QApplication>

#include "scan.h"

class MyListener: public ScanListener
{
public:
    void scanStarted(ScanDir *d) override
    {
        printf("Started Scan on %s\n", qPrintable(d->name()));
    };

    void sizeChanged(ScanDir *d) override
    {
        printf("Change in %s: Dirs %d, Files %d ",
               qPrintable(d->name()),
               d->dirCount(), d->fileCount());
        printf("Size %llu\n", (unsigned long long int)d->size());
    }

    void scanFinished(ScanDir *d) override
    {
        printf("Finished Scan on %s\n", qPrintable(d->name()));
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ScanManager m(QStringLiteral("/opt"));
    if (argc > 1) {
        m.setTop(argv[1]);
    }

    m.setListener(new MyListener());
    m.startScan();
    while (m.scan(1));
}

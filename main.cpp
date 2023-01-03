// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ai.h"

#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Ai recorder;
    recorder.show();

    return app.exec();
}

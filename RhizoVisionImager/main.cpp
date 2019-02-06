/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: main.cpp

Author: Anand Seethepalli (aseethepalli@noble.org)
Principal Investigator: Larry York (lmyork@noble.org)
Root Phenomics Lab
Noble Research Institute, LLC

This file is part of RhizoVision Imager.

RhizoVision Imager is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvutil is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RhizoVision Imager.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <cvutil.h>

#include "Imager.h"

void logger(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QString lastmsg;
    QString helper = "";

    if (Imager::logbox == 0)
    {
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type)
        {
        case QtDebugMsg:
            if (Imager::showDebugMessages)
                fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtWarningMsg:
            fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtCriticalMsg:
            fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            abort();
        }
    }
    else
    {
        switch (type)
        {
        case QtDebugMsg:
            if (Imager::showDebugMessages)
            {
                Imager::logbox->setTextColor(QColor(0x2C, 0xB5, 0xE9));
                Imager::logbox->append(msg);
                //lastmsg = msg;
                return;
            }
            else
                return;
            break;
        case QtInfoMsg:
            Imager::logbox->setTextColor(QColor(0x0, 0x0, 0x0));
            break;
        case QtWarningMsg:
            Imager::logbox->setTextColor(QColor(255, 127, 0));
            break;
        case QtCriticalMsg:
            Imager::logbox->setTextColor(QColor("red"));
            helper = "Error: ";
            break;
        case QtFatalMsg:
            abort();
        }

        if (Imager::logbox != 0 && lastmsg != msg)
        {
            Imager::logbox->append(helper + msg);
            lastmsg = msg;
        }
    }
}

int main(int argc, char* argv[])
{
    qInstallMessageHandler(logger);
    setUseOptimized(true);

    if (!checkHardwareSupport(CV_CPU_AVX2))
    {
        init(argc, argv, false);
        setUseOptimized(false);
    }
    else
    {
        init(argc, argv, true);
    }

    QCoreApplication *app = QApplication::instance();
    app->setOrganizationName("Noble Research Institute");
    app->setOrganizationDomain("www.noble.org");
    app->setApplicationName("RhizoVision Imager");
    
    Imager imgui;
    imgui.show();

    return QApplication::exec();
}

/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: Imager.cpp

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

#include "Imager.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>

#include <QtGui/QPainter>
#include <QtCore/QTimer>

QTextEdit *Imager::logbox = 0;

#ifdef _DEBUG
bool Imager::showDebugMessages = true;
#else
bool Imager::showDebugMessages = false;
#endif

void ScrollArea::wheelEvent(QWheelEvent *e)
{
    int f = e->angleDelta().y() / 8 / 15;
    scale = scale * pow(1.25, f);
    scale = max(0.1, min(4.0, scale));
    e->accept();
    emit scaleChanged(int(scale * 1000));
}

ScrollArea::ScrollArea()
{
}

ScrollArea::ScrollArea(QWidget * parent) : QScrollArea(parent)
{
}

double ScrollArea::GetScale()
{
    return scale;
}

void ScrollArea::SetScale(double s)
{
    scale = max(0.1, min(4.0, s));
    emit scaleChanged(int(scale * 1000.0));
}

void rid()
{
    int nRetCode = 0;

    UINT nDevices;
    PRAWINPUTDEVICELIST pRawInputDeviceList;
    if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
    {
        qCritical() << "Could not get raw input device list.";
        return;
    }

    if ((pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == NULL)
    {
        qCritical() << "Could not allocate raw input device list.";
        return;
    }

    int nNoOfDevices = 0;
    if ((nNoOfDevices = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST))) == ((UINT)-1))
    {
        // Error
        return;
    }

    RID_DEVICE_INFO rdi;
    rdi.cbSize = sizeof(RID_DEVICE_INFO);

    for (int i = 0; i < nNoOfDevices; i++)
    {
        UINT size = 256;
        TCHAR tBuffer[256] = { 0 };

        if (GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, tBuffer, &size) < 0)
        {
            // Error in reading device name
        }

        qDebug () << QString::fromWCharArray(L"Device Name: %s\n") << QString::fromWCharArray(tBuffer);

        UINT cbSize = rdi.cbSize;
        if (GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdi, &cbSize) < 0)
        {
            qWarning() << "Error in reading raw input device information.";
        }

        if (rdi.dwType == RIM_TYPEMOUSE)
        {
            qDebug() << "ID for Mouse:" << rdi.mouse.dwId;
            qDebug() << "Number of Buttons:" << rdi.mouse.dwNumberOfButtons;
            qDebug() << "Sample rate(Number of data points):" << rdi.mouse.dwSampleRate;
            qDebug() << "**************************";
        }

        if (rdi.dwType == RIM_TYPEKEYBOARD)
        {
            qDebug() << "Keyboard Mode:" << rdi.keyboard.dwKeyboardMode;
            qDebug() << "Number of function keys:" << rdi.keyboard.dwNumberOfFunctionKeys;
            qDebug() << "Number of indicators:" << rdi.keyboard.dwNumberOfIndicators;
            qDebug()  << "Number of keys total: " << rdi.keyboard.dwNumberOfKeysTotal;
            qDebug()  << "Type of the keyboard: " << rdi.keyboard.dwType;
            qDebug()  << "Subtype of the keyboard: " << rdi.keyboard.dwSubType;
            qDebug()  << "***********************";
        }

        if (rdi.dwType == RIM_TYPEHID)
        {
            qDebug()  << "Vendor Id:" << rdi.hid.dwVendorId;
            qDebug()  << "Product Id:" << rdi.hid.dwProductId;
            qDebug()  << "Version No:" << rdi.hid.dwVersionNumber;
            qDebug()  << "Usage for the device: " << rdi.hid.usUsage;
            qDebug()  << "Usage Page for the device: " << rdi.hid.usUsagePage;
            qDebug()  << "***********************";
        }
    }

    free(pRawInputDeviceList);
}

void Imager::zoomin()
{
    area->SetScale(area->GetScale() * 1.25);
}

void Imager::zoomout()
{
    area->SetScale(area->GetScale() / 1.25);
}

void Imager::zoomfit()
{
    double _scale = MIN(double(area->height()) / double(image->height()), double(area->width()) / double(image->width())) - 0.001;
    area->SetScale(area->GetScale() * _scale);
}

Imager::Imager()
{
    this->setWindowIcon(QIcon(":/icons/rvimagernew"));

    // To first set the logging sub-system.
    logdocker = new QDockWidget("Log", this);
    logdocker->setObjectName(logdocker->windowTitle());
    logbox = new QTextEdit(logdocker);
    logdocker->setWidget(logbox);
    logbox->setReadOnly(true);
    logdocker->setAllowedAreas(Qt::BottomDockWidgetArea);
    this->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, logdocker);
    
    rid();
    br = new BarcodeReader(this);
    br->initialize();
    
    cams = VisionCameraArray::GetInstance();
    cams->init();
    docker = new QDockWidget("Imaging settings", this);
    docker->setAllowedAreas(Qt::LeftDockWidgetArea);

    createmenus();

    QWidget *wid = new QWidget(this);
    QVBoxLayout *vl = new QVBoxLayout();
    QToolBar *tbox = new QToolBar();

    QAction *act = new QAction(QIcon(":/icons/zoomout"), tr("Zoom out."), this);
    act->setStatusTip(tr("Zoom out."));
    connect(act, &QAction::triggered, this, &Imager::zoomout);
    
    tbox->addAction(act);

    act = new QAction(QIcon(":/icons/zoomin"), tr("Zoom in."), this);
    act->setStatusTip(tr("Zoom in."));
    connect(act, &QAction::triggered, this, &Imager::zoomin);

    tbox->addAction(act);

    act = new QAction(QIcon(":/icons/fittowindow"), tr("Zoom to fit."), this);
    act->setStatusTip(tr("Zoom to fit."));
    connect(act, &QAction::triggered, this, &Imager::zoomfit);

    tbox->addAction(act);

    area = new ScrollArea(this);
    area->setBackgroundRole(QPalette::Dark);
    image = new ImageLabel(this);
    image->setBackgroundRole(QPalette::Base);
    image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    image->setScaledContents(true);

    area->setWidget(image);
    
    connect(area, &ScrollArea::scaleChanged, this, &Imager::updateimage);

    vl->addWidget(tbox);
    vl->addWidget(area);
    wid->setLayout(vl);

    setCentralWidget(wid);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    
    dockcontainer = new QVBoxLayout();
    
    QLabel *savlab = new QLabel("Image save location", docker);
    savepath = new QLineEdit(docker);
    validpath = new QLabel(docker);
    browse = new QPushButton("Browse", docker);

    connect(browse, &QPushButton::released, this, &Imager::browser);

    QHBoxLayout *hl = new QHBoxLayout();
    hl->addWidget(savepath, 1);
    hl->addWidget(validpath);

    connect(savepath, &QLineEdit::textChanged, this, &Imager::updateLabel);

    dockcontainer->addWidget(savlab);
    dockcontainer->addLayout(hl);
    dockcontainer->addWidget(browse, 0, Qt::AlignRight);

    QLabel *fileprefixlab = new QLabel("Save file prefix: ", docker);
    fileprefix = new QLineEdit(docker);
    fileprefix->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_-]*"), docker));
    
    //outputsegmented = new QCheckBox("Output segmented images.", docker);
    singleshotbtn = new QPushButton("Single shot");
    chkbarcode = new QCheckBox("Enable barcode reader for imaging.", docker);
    enablebeep = new QCheckBox("Enable sound for image capture.", docker);

    beeper = new QSound(QString::fromStdString("sounds/camera_shutter.wav"));

    //connect(outputsegmented, &QCheckBox::stateChanged, this, &Imager::outputsegmentedchanged);
    connect(singleshotbtn, &QPushButton::released, this, &Imager::singleshot);
    connect(chkbarcode, &QCheckBox::stateChanged, this, &Imager::chkbarcodechanged);
    
    //vl->addLayout(hl);
    dockcontainer->addWidget(fileprefixlab);
    dockcontainer->addWidget(fileprefix);
    //dockcontainer->addWidget(outputsegmented);
    dockcontainer->addWidget(singleshotbtn, 0, Qt::AlignRight);
    dockcontainer->addWidget(chkbarcode);
    dockcontainer->addWidget(enablebeep);

    labcameras = new QLabel("Cameras connected");

    dockcontainer->addWidget(labcameras);

    cameras = new QTreeWidget(docker);
    cameras->setColumnCount(2);
    cameras->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

    createCameraControls();
    cameras->collapseAll();
    
    dockcontainer->addWidget(cameras);

    QWidget *w = new QWidget(this);
    w->setLayout(dockcontainer);

    // Add all controls to docker
    docker->setWidget(w);
    this->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, docker);
    
    updateLabel();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
    
    LoadSettings();
    createprofilecontrols();
    //connect(this, &Imager::cl)
    cams->initLiveImaging();

    if (cams->GetLiveImage(liveimg))
    {
        if (!liveimg.empty())
        {
            if(liveimg.channels() == 1)
                image->setPixmap(QPixmap::fromImage(QImage(liveimg.data,
                    liveimg.cols, liveimg.rows, liveimg.step,
                    QImage::Format_Grayscale8)));
            else
                image->setPixmap(QPixmap::fromImage(QImage(liveimg.data,
                    liveimg.cols, liveimg.rows, liveimg.step,
                    QImage::Format::Format_RGB888)));
        }
    }

    inttimer = new QTimer(this);
    connect(inttimer, &QTimer::timeout, this, &Imager::intervaltimeout);

    image->adjustSize();
    cams->liveimagingstarted = false;
    timerid = startTimer(50);
    cams->threadcomplete = true;
}

void Imager::opencsv()
{
    QString fileloc;
    
    fileloc = QFileDialog::getOpenFileName(this,
        tr("Output Location"), QString(), tr("csv files (*.csv)"));

    QFile f(fileloc);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Cannot read csv file.";
        return;
    }
    
    QTextStream stream(&f);
    QString csvtext = stream.readAll();
    QStringList slist = csvtext.split(QRegExp("[\r\n\t,;]+"));
    plotlist.clear();
    plotidx = 0;

    for (int i = 0; i < slist.size(); i++)
        if (slist[i].length() > 0)
            plotlist.push_back(slist[i].toStdString());
    
    qInfo() << "Next imaging " << (plotidx + 1) << " of " << plotlist.size() << "(Sample plot: " << tr(plotlist[0].c_str()) << ").\n\tPress ENTER to capture image, \n\tESC to stop imaging plot sequence, \n\tSHIFT + ENTER to recapture previous image again.";
    plotseqmodestarted = true;
}

void Imager::closeEvent(QCloseEvent *event)
{
    SaveSettings();
    QMainWindow::closeEvent(event);
}

void Imager::LoadSettings()
{
    if (cams->camerror)
        return;

    QSettings settings(QCoreApplication::applicationDirPath() + "/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName() + ".ini", QSettings::IniFormat);

    LoadProfile(settings.value("Profile", "").toString());
}

void Imager::SaveSettings()
{
    if (cams->camerror)
        return;

    QSettings settings(QCoreApplication::applicationDirPath() + "/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName() + ".ini", QSettings::IniFormat);
    settings.setValue("Profile", currentprofile);
    //LoadProfile(settings.value("Profile", "").toString());
}

void Imager::LoadProfile(QString profilename)
{
    if (cams->size() == 0)
    {
        qCritical() << "Cameras not connected for loading profile.";
        currentprofile = profilename;
        return;
    }

    if (profilename == "")
    {
        for (int i = 0; i < cams->size(); i++)
        {
            editors[i * 3]->setText("0.0");
            editors[i * 3 + 1]->setText("1.0");
            editors[i * 3 + 2]->setText("35000.0");
            
            /*cams->setgain(i, 0.0);
            cams->setgamma(i, 1.0);
            cams->setexposure(i, 35000.0);*/
        }

        currentprofile = "";
        qInfo() << "Empty profile loaded.";
    }
    else
    {
        QSettings settings(QCoreApplication::applicationDirPath() + "/profiles/" + profilename + ".ini", QSettings::IniFormat);

        try
        {
            for (int i = 0; i < cams->size(); i++)
            {
                editors[i * 3]->setText(settings.value("cam" + QString::fromStdString(to_string(i + 1)) + "/Gain", 0.0).toString());
                editors[i * 3 + 1]->setText(settings.value("cam" + QString::fromStdString(to_string(i + 1)) + "/Gamma", 1.0).toString());
                editors[i * 3 + 2]->setText(settings.value("cam" + QString::fromStdString(to_string(i + 1)) + "/Exposure", 35000.0).toString());
            }
        }
        catch (GenericException ex)
        {
            refreshdevices();

            for (int i = 0; i < cams->size(); i++)
            {
                editors[i * 3]->setText(settings.value("cam" + QString::fromStdString(to_string(i + 1)) + "/Gain", 0.0).toString());
                editors[i * 3 + 1]->setText(settings.value("cam" + QString::fromStdString(to_string(i + 1)) + "/Gamma", 1.0).toString());
                editors[i * 3 + 2]->setText(settings.value("cam" + QString::fromStdString(to_string(i + 1)) + "/Exposure", 35000.0).toString());
            }
        }

        currentprofile = profilename;
        qInfo() << "Loaded profile: " << profilename;
    }
}

void Imager::SaveProfile(QString profilename)
{
    if (cams->size() == 0)
    {
        qCritical() << "Cameras not connected for saving profile.";
        return;
    }

    QSettings settings(QCoreApplication::applicationDirPath() + "/profiles/" + profilename + ".ini", QSettings::IniFormat);

    for (int i = 0; i < cams->size(); i++)
    {
        settings.setValue("cam" + QString::fromStdString(to_string(i + 1)) + "/Gain", cams->gain(i));
        settings.setValue("cam" + QString::fromStdString(to_string(i + 1)) + "/Gamma", cams->gamma(i));
        settings.setValue("cam" + QString::fromStdString(to_string(i + 1)) + "/Exposure", cams->exposure(i));
    }
}

bool isvalidcharacter(int keyval)
{
    char key = char(keyval);
    return (isprint(keyval) &&
        key != '<' &&
        key != '>' &&
        key != ':' &&
        key != '/' &&
        key != '\\' &&
        key != '|' &&
        key != '?' &&
        key != '*' &&
        key != '"');
}

void Imager::profilecreateorupdate()
{
    if (cams->size() == 0)
    {
        qCritical() << "Cameras not connected for saving profile.";
        return;
    }

    bool ok = false;
    bool validity = false;

    while (true)
    {
        string text = QInputDialog::getItem(this, tr("Save Profile"), tr("Profile name:"), profiles, -1, true, &ok).toStdString();
        
        if (ok)
        {
            for (auto& ch : text)
            {
                if (!isvalidcharacter(ch))
                {
                    validity = false;
                    break;
                }
                else
                    validity = true;
            }

            if (!validity)
            {
                qCritical() << "Invalid profile name.";
                continue;
            }

            if (!text.empty())
            {
                if (profiles.contains(QString::fromStdString(text)))
                {
                    if (QMessageBox::question(this, "Save Profile", "The profile already exists. Overwrite existing profile?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
                    {
                        SaveProfile(QString::fromStdString(text));
                        return;
                    }
                    else
                        continue;
                }

                SaveProfile(QString::fromStdString(text));
                profiles.push_back(QString::fromStdString(text));
                profiles.sort();
                currentprofile = QString::fromStdString(text);
                break;
            }
        }
        else
            break;
    }
}

void Imager::createprofilecontrols()
{
    profileactiongroup = new QActionGroup(this);

    QDir dir(QCoreApplication::applicationDirPath() + "/profiles/");
    QFileInfoList infolist = dir.entryInfoList({ "*.ini" }, QDir::Filter::Files, QDir::SortFlag::Name);

    for (QFileInfo fi : infolist)
    {
        profiles.push_back(fi.baseName());
        QAction *action = new QAction(fi.baseName(), this);
        action->setCheckable(true);
        if (fi.baseName() == currentprofile)
            action->setChecked(true);
        connect(action, &QAction::triggered, this, &Imager::profilechanged);
        profileactiongroup->addAction(action);
    }

    profiles.sort();
    profilemenu->addActions(profileactiongroup->actions());
}

void Imager::profilechanged()
{
    if (cams->camerror)
        return;

    QAction *action = qobject_cast<QAction *>(sender());
    QString profilename = action->text();
    LoadProfile(profilename);
}

void Imager::createSliderControl(int camid, QTreeWidgetItem *rootitem,
    string propname, string proptext, int dec)
{
    double minv = 0, maxv = 0, dval = 0;
    QSlider *slider = nullptr;
    QLineEdit *editor = nullptr;
    QHBoxLayout *hl = nullptr;
    QWidget *w = nullptr;
    QTreeWidgetItem *item = nullptr;

    minv = cams->GetValue(camid, propname, VisionCameraArray::ValType::MinValue);
    maxv = cams->GetValue(camid, propname, VisionCameraArray::ValType::MaxValue);
    dval = cams->GetValue(camid, propname, VisionCameraArray::ValType::CurrentValue);

    w = new QWidget();

    slider = new QSlider(Qt::Orientation::Horizontal);
    slider->setMinimum(int(minv * dec));
    slider->setMaximum(int(maxv * dec));
    slider->setValue(int(dval * dec));
    slider->setTickInterval(1);

    editor = new QLineEdit();
    editor->setValidator(new QDoubleValidator(minv, maxv, log10(dec)));
    editor->setText(QString::fromStdString(to_string(dval)));

    connect(slider, &QSlider::sliderMoved, this, &Imager::updateText);
    connect(slider, &QSlider::sliderReleased, this, &Imager::updateText);
    connect(editor, &QLineEdit::textChanged, this, &Imager::updateSlider);

    hl = new QHBoxLayout();
    hl->addWidget(slider);
    hl->addWidget(editor);

    hl->setContentsMargins(0, 0, 0, 0);
    w->setLayout(hl);

    sliders.push_back(slider);
    editors.push_back(editor);
    decimals.push_back(dec);

    item = new QTreeWidgetItem();
    rootitem->addChild(item);
    item->setText(0, proptext.c_str());
    //item->setText(1, proptext.c_str());
    cameras->setItemWidget(item, 1, w);
    
    camids.push_back(camid);
    props.push_back(propname.c_str());
}

void Imager::livecamerachanged()
{
    QAction *action = qobject_cast<QAction *>(sender());

    QString camname = action->text();

    for (int i = 0; i < cams->size(); i++)
    {
        if (!cams->IsOpen(i))
        {
            refreshdevices();
            continue;
        }

        QString cam = ("Camera #" + to_string(i + 1) + " - (" + cams->serial(int(i)) + ")").c_str();

        if (cam == camname)
        {
            cams->setLiveID(i);
            
            break;
        }
    }
}

void Imager::formatchanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    QString desc = action->text();

    for (int i = 0; i < formatdesc.size(); i++)
    {
        if (formatdesc[i] == desc)
        {
            cams->setsaveformat(formats[i].toStdString());
            break;
        }
    }
}

void Imager::createCameraControls()
{
    if (cams->camerror)
        return;

    QTreeWidgetItem *rootitem = nullptr;
    vector<string> propnames = {"Gain", "Gamma", "ExposureTime"};
    vector<string> proptexts = { "Gain", "Gamma", "Exposure time" };
    vector<int> dec= {100000, 100000, 1};
    bool hasopencamera = false;

    if (menuactiongroup != nullptr)
    {
        for (int p = 0; p < menuactiongroup->actions().size(); p++)
        {
            livecammenu->removeAction(menuactiongroup->actions()[p]);
        }

        menuactiongroup->deleteLater();
        menuactiongroup = nullptr;
    }

    menuactiongroup = new QActionGroup(this);

    for (int i = 0; i < cams->size(); i++)
    {
        if (!cams->IsOpen(i))
            continue;

        hasopencamera = true;
        rootitem = new QTreeWidgetItem(cameras);
        rootitem->setFirstColumnSpanned(true);
        QFont f = rootitem->font(0);
        f.setBold(true);
        rootitem->setFont(0, f);

        //rootitem->setChildIndicatorPolicy(QTreeWidgetItem::ChildIndicatorPolicy::ShowIndicator);
        rootitem->setText(0, ("Camera #" + to_string(i + 1) + " - (" + cams->serial(int(i)) + ")").c_str());
        QAction *action = new QAction(("Camera #" + to_string(i + 1) + " - (" + cams->serial(int(i)) + ")").c_str(), this);
        action->setCheckable(true);
        menuactiongroup->addAction(action);
        if (i == cams->getLiveID())
            action->setChecked(true);
        connect(action, &QAction::triggered, this, &Imager::livecamerachanged);
        cameras->addTopLevelItem(rootitem);

        for (int j = 0; j < propnames.size(); j++)
            createSliderControl(i, rootitem, propnames[j], proptexts[j], dec[j]);
    }

    if (!hasopencamera)
    {
        QAction *action = new QAction(string("(None)").c_str(), this);
        action->setDisabled(true);
        menuactiongroup->addAction(action);
    }

    livecammenu->addActions(menuactiongroup->actions());
    cameras->setHeaderLabels({"Feature", "Value"});
    //cameras->setIndentation(12);
}

void Imager::createmenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    //QToolBar *fileToolBar = addToolBar(tr("File"));
    //const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    QAction *refreshAct = new QAction(tr("Refresh devices"), this);
    //singleshotAct->setShortcuts(QKeySequence::New);
    refreshAct->setStatusTip(tr("Refresh devices."));
    connect(refreshAct, &QAction::triggered, this, &Imager::refreshdevices);
    fileMenu->addAction(refreshAct);
    //fileToolBar->addAction(newAct);

    //QToolBar *fileToolBar = addToolBar(tr("File"));
    //const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    QAction *singleshotAct = new QAction(tr("Single shot"), this);
    //singleshotAct->setShortcuts(QKeySequence::New);
    singleshotAct->setStatusTip(tr("Take a single photo from all the connected cameras."));
    connect(singleshotAct, &QAction::triggered, this, &Imager::singleshot);
    fileMenu->addAction(singleshotAct);
    //fileToolBar->addAction(newAct);

    fileMenu->addSeparator();
    
    QAction *loadsampleidorderAct = new QAction(tr("Load Sample ID Order"), this);
    loadsampleidorderAct->setStatusTip(tr("Load the sample ID order from a csv file and use each entry in the file as a prefix to the subsequent capture of images."));
    connect(loadsampleidorderAct, &QAction::triggered, this, &Imager::opencsv);
    fileMenu->addAction(loadsampleidorderAct);

    QAction *intervalAct = new QAction(tr("Interval Imaging"), this);
    intervalAct->setStatusTip(tr("Take pictures at uniform time intervals."));
    connect(intervalAct, &QAction::triggered, this, &Imager::startintervalimaging);
    fileMenu->addAction(intervalAct);

    fileMenu->addSeparator();

    QMenu *saveformat = fileMenu->addMenu("Save File Format");
    formatactiongroup = new QActionGroup(this);

    for (int i = 0; i < formats.size(); i++)
    {
        QAction *action = new QAction(formatdesc[i], this);
        action->setCheckable(true);
        formatactiongroup->addAction(action);
        if (formats[i] == "PNG")
            action->setChecked(true);
        connect(action, &QAction::triggered, this, &Imager::formatchanged);
    }

    saveformat->addActions(formatactiongroup->actions());

    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(docker->toggleViewAction());
    viewMenu->addAction(logdocker->toggleViewAction());

    livecammenu = viewMenu->addMenu("Live camera");

    QMenu *profileMenu = menuBar()->addMenu(tr("&Profile"));
    profilemenu = profileMenu->addMenu("Current profile");

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &Imager::about);

    QAction *createprofileAct = new QAction(tr("Save profile"), this);
    createprofileAct->setStatusTip(tr("Save current camera settings in a profile."));
    connect(createprofileAct, &QAction::triggered, this, &Imager::profilecreateorupdate);
    profileMenu->addAction(createprofileAct);
}

void Imager::intervaltimeout()
{
    qInfo().noquote() << "Taking picture #" << nthimage << " at time " << ":"
        << QString("%1").arg(ehours, 2, 10, QChar('0')) << ":"
        << QString("%1").arg(eminutes, 2, 10, QChar('0')) << ":"
        << QString("%1").arg(eseconds, 2, 'g', 3, QChar('0')); // QString::number(eseconds, 10, 3);

    singleshotfile(fileprefix->text().toStdString() + "_" + QString::number(ehours * 3600 + eminutes * 60 + eseconds,'g', 3).toStdString());

    ehours += hours;
    eminutes += minutes;
    eseconds += seconds;

    if (eseconds >= 60.0)
    {
        eseconds -= 60.0;
        eminutes++;
    }

    if (eminutes >= 60)
    {
        eminutes -= 60;
        ehours++;
    }

    nthimage++;
}

void Imager::startintervalimaging()
{
    if (inttimer->isActive())
    {
        inttimer->stop();
        intervalimaging = false;
        nthimage = 0;
        ehours = eminutes = eseconds = 0.0;
    }

    QDialog *setinterval = new QDialog(this, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
    setinterval->setWindowTitle(tr("Interval Imaging"));

    QVBoxLayout *vl = new QVBoxLayout();
    QLabel *lb = new QLabel("Set time interval: ");
    vl->addWidget(lb);
    
    QHBoxLayout *layout = new QHBoxLayout();

    QSpinBox *sph = new QSpinBox();
    sph->setMinimum(0);
    sph->setMaximum(9999);
    lb = new QLabel("Hours :");
    
    layout->addWidget(lb, 0);
    layout->addWidget(sph, 0);

    vl->addLayout(layout);

    layout = new QHBoxLayout();
    QSpinBox *spm = new QSpinBox();
    spm->setMinimum(0);
    spm->setMaximum(60);
    lb = new QLabel("Minutes :");
    
    layout->addWidget(lb, 0);
    layout->addWidget(spm, 0);

    vl->addLayout(layout);

    layout = new QHBoxLayout();
    QDoubleSpinBox *dsp = new QDoubleSpinBox();
    dsp->setDecimals(3);
    dsp->setMinimum(1.0);
    dsp->setMaximum(60.0);
    lb = new QLabel("Seconds :");

    layout->addWidget(lb, 0);
    layout->addWidget(dsp, 0);

    vl->addLayout(layout);

    bool valueset = false;

    layout = new QHBoxLayout();
    QPushButton *btn = new QPushButton("Ok");
    connect(btn, &QPushButton::clicked, [&]() 
    {
        valueset = true;
        hours = sph->value();
        minutes = spm->value();
        seconds = dsp->value();

        setinterval->close();
    });

    layout->addWidget(btn, 0, Qt::AlignRight);
    
    btn = new QPushButton("Cancel");
    connect(btn, &QPushButton::clicked, setinterval, &QDialog::close);

    layout->addWidget(btn, 0, Qt::AlignRight);

    vl->addLayout(layout);

    setinterval->setLayout(vl);

    setinterval->open();
    setinterval->exec();

    if (valueset)
    {
        nthimage = 0;
        ehours = eminutes = eseconds = 0.0;
        intervalimaging = true;
        qInfo() << "Interval imaging started. Press ESC key to stop.";
        inttimer->start((double(hours * 60 * 60) + double(minutes * 60) + seconds) * 1000);
        intervaltimeout();
    }

    setinterval->deleteLater();
}

void Imager::about()
{
    QDialog *about = new QDialog(this);
    about->setWindowFlags(about->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    about->setWindowIcon(QIcon(":/icons/rvimagernew"));
    about->setWindowTitle(tr("About RhizoVision Imager"));
    
    QVBoxLayout *vl = new QVBoxLayout();
    QHBoxLayout *layout = new QHBoxLayout();

    QLabel *lb = new QLabel();
    lb->setPixmap(QPixmap(":/icons/rvimagernew"));
    lb->setMaximumWidth(100);
    lb->setMaximumHeight(100);
    lb->setScaledContents(true);

    layout->addWidget(lb, 0);

    QTextBrowser  *tlb = new QTextBrowser();
    tlb->setHtml(tr(
        "<b>RhizoVision Imager</b> (version " RHIZOVISION_IMAGER_VERSION ")<br/>"
        "<b>Copyright (C) 2018-2019 Noble Research Institute, LLC</b><br/><br/>"

        "This program connects to multiple Basler vision cameras to "
        "take picture of an object from multiple views or take multiple "
        "parts of an object. The program supports taking pictures from "
        "cameras using barcode reader.<br/><br/>"
        
        "RhizoVision Imager is free software: you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation, either version 3 of the License, or "
        "(at your option) any later version.<br/><br/>"
        "RhizoVision Imager is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
        "GNU General Public License for more details.<br/><br/>"
        "You should have received a copy of the GNU General Public License "
        R"(along with RhizoVision Imager.  If not, see <a href = "https://www.gnu.org/licenses/">https://www.gnu.org/licenses/.</a><br/><br/>)"

        "Please send any comments/suggestions/improvements for the program to the authors:<br/>"
        "<pre>Anand Seethepalli,<br/>"
        "Computer Vision Specialist,<br/>"
        "Root Phenomics Lab,<br/>"
        "Noble Research Institute, LLC<br/><br/>"
        "email: <br/>aseethepalli@noble.org<br/>"
        "anand_seethepalli@yahoo.co.in<br/><br/>"
        "Larry York,<br/>"
        "Principal Investigator<br/>"
        "Root Phenomics Lab,<br/>"
        "Noble Research Institute, LLC<br/><br/>"
        "email: <br/>lmyork@noble.org<br/><br/></pre>"
    ));
    tlb->setReadOnly(true);
    tlb->setOpenExternalLinks(true);
    layout->addWidget(tlb, 0); 
    
    vl->addLayout(layout);

    QPushButton *btn = new QPushButton("Ok");
    connect(btn, &QPushButton::clicked, about, &QDialog::close);

    vl->addWidget(btn, 0, Qt::AlignRight);
    about->setLayout(vl);
    about->layout()->setSizeConstraint(QLayout::SetFixedSize);

    about->open();

    about->exec();

    about->deleteLater();
}

Imager::~Imager()
{
    if(initialized)
        paintthread.join();
}

void Imager::updateLabel()
{
    int iconheight = 20;
    string filepath = savepath->text().toStdString();
    experimental::filesystem::path p(filepath); 
    p = experimental::filesystem::path(filepath);

    if (experimental::filesystem::exists(p) &&
        experimental::filesystem::is_directory(p))
    {
        QIcon icn = this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogApplyButton);
        QPixmap map = icn.pixmap(iconheight, iconheight);
        validpath->setPixmap(map);
        IsValidSaveLoc = true;

        if (filepath.length() > 0 && (filepath[filepath.length() - 1] != '/' || filepath[filepath.length() - 1] != '\\'))
            filepath += "/";
        savefilepath = filepath;
    }
    else
    {
        QIcon icn = this->style()->standardIcon(QStyle::StandardPixmap::SP_DialogCancelButton);
        QPixmap map = icn.pixmap(iconheight, iconheight);
        validpath->setPixmap(map);
        IsValidSaveLoc = false;
    }
}

//void Imager::outputsegmentedchanged()
//{
//    outputsegmentedimage = (outputsegmented->checkState() == Qt::CheckState::Checked);
//}

void Imager::chkbarcodechanged()
{
    enablebarcode = (chkbarcode->checkState() == Qt::CheckState::Checked);

    RAWINPUTDEVICE dev;

    if (enablebarcode)
    {
        dev.usUsagePage = 1;
        dev.usUsage = 6;
        dev.dwFlags = RIDEV_DEVNOTIFY;
        dev.hwndTarget = (HWND)winId();
        RegisterRawInputDevices(&dev, 1, sizeof(dev));
        br->initialize();
    }
    else
    {
        dev.usUsagePage = 1;
        dev.usUsage = 6;
        dev.dwFlags = RIDEV_REMOVE;
        dev.hwndTarget = NULL;
        RegisterRawInputDevices(&dev, 1, sizeof(dev));
    }
}

void Imager::barcodechanged()
{

}

void Imager::updateText()
{
    // If the event handler is called from 
    // another event handler ...
    //if (updateslidermode)
    //    return;

    updateslidermode = 1;

    int i = 0;
    QSlider *s = qobject_cast<QSlider *>(sender());

    for (; i < sliders.size(); i++)
        if (s == sliders[i])
            break;

    double val = double(s->value()) / double(decimals[i]);
    editors[i]->setText(to_string(val).c_str());
    cams->SetValue(camids[i], props[i].toStdString(), val);

    updateslidermode = 0;
}

void Imager::updateSlider()
{
    // If the event handler is called from 
    // another event handler ...
    //if (updateslidermode)
    //    return;

    updateslidermode = 2;

    int i = 0;
    QLineEdit *s = qobject_cast<QLineEdit *>(sender());

    for (; i < editors.size(); i++)
        if (s == editors[i])
            break;

    double val = double(s->text().toDouble()) * double(decimals[i]);
    sliders[i]->setValue(int(val));
    val = double(sliders[i]->value()) / double(decimals[i]);
    cams->SetValue(camids[i], props[i].toStdString(), val);
    //s->setText(QString::fromStdString(to_string(val)));

    updateslidermode = 0;
}

void Imager::browser()
{
    string fileloc;
    string filepath = savepath->text().toStdString();

    if (filepath.length() > 0 && (filepath[filepath.length() - 1] != '/' || filepath[filepath.length() - 1] != '\\'))
        filepath += "/";
    experimental::filesystem::path p(filepath);

    if (experimental::filesystem::exists(p) &&
        experimental::filesystem::is_directory(p))
        fileloc = QFileDialog::getExistingDirectory(this,
            tr("Output Location"), tr(filepath.c_str()),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdString();
    else
        fileloc = QFileDialog::getExistingDirectory(this,
            tr("Output Location"), QString(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdString();

    if (fileloc.length() > 0)
        savepath->setText(tr(fileloc.c_str()));
}

void Imager::updateimage(int scaleval)
{
    if (image->pixmap() == nullptr)
        return;

    static QSize sz = img.size();
    double scale = scaleval / 1000.0;

    //qInfo() << "updateimage out: " << " scale: " << scale << " img: " << img.size();
    //qInfo() << "updateimage before image->height() = " << image->height() << "image->width() = " << image->width();
    //image->resize(scale * image->pixmap()->size());
    if (img.size() != sz)
    {
        sz = img.size();
        scale *= double(img.size().height()) / double(sz.height());
        area->SetScale(scale);
        return;
    }

    image->resize(scale * img.size());
    //qInfo() << "updateimage after  image->height() = " << image->height() << "image->width() = " << image->width();

    area->horizontalScrollBar()->setValue(int(scale * area->horizontalScrollBar()->value()
        + ((scale - 1) * area->horizontalScrollBar()->pageStep() / 2)));
    area->verticalScrollBar()->setValue(int(scale * area->verticalScrollBar()->value()
        + ((scale - 1) * area->verticalScrollBar()->pageStep() / 2)));
}

//bool Imager::event(QEvent* ev)
//{
//    int keyval;
//    static string scanstr = "";
//    static clock_t start;
//    clock_t end;
//    long long interv = 0;
//    QKeyEvent *keyev = nullptr;
//
//    if (!enablebarcode)
//        return QWidget::event(ev);
//    
//    if (ev->type() == QEvent::KeyRelease)
//    {
//        keyev = static_cast<QKeyEvent*>(ev);
//        keyval = keyev->key();
//        
//        if (keyval >= -1 && keyval <= 255 && isvalidcharacter(keyval))
//        {
//            if (scanstr.length() == 0)
//            {
//                scanstr += char(keyval);
//                qDebug() << " [isalnum && scanstr.length() == 0] scanstr = " << QString::fromStdString(scanstr) << " --- timerinterval = " << timerinterval;
//
//                start = clock();
//                timerinterval = 0;
//            }
//            else
//            {
//                end = clock();
//                interv = (end - start - timerinterval);
//
//                qDebug() << " [isalnum && scanstr.length() > 0] Interval = " << interv << " --- timerinterval = " << timerinterval;
//
//                if (interv <= 15)
//                {
//                    start = clock();
//                    timerinterval = 0;
//                    scanstr += char(keyval);
//
//                    qDebug() << " [control 1] scanstr = " << QString::fromStdString(scanstr);
//                }
//                else
//                {
//                    start = clock();
//                    timerinterval = 0;
//                    scanstr = char(keyval);
//
//                    qDebug() << " [control 2] scanstr = " << QString::fromStdString(scanstr);
//                }
//            }
//        }
//        else
//        {
//            if (scanstr.length() != 0)
//            {
//                end = clock();
//                interv = (end - start - timerinterval);
//
//                qDebug() << " [!isalnum && scanstr.length() > 0] Interval = " << interv << " --- timerinterval = " << timerinterval;
//
//                if (interv <= 15)
//                {
//                    qDebug() << " [Bingo] : " << QString::fromStdString(scanstr);
//                    scanstr = "";
//                    return true;
//                    //singleshot(scanstr);
//                }
//                else
//                    scanstr = "";
//            }
//        }
//    }
//
//    return QWidget::event(ev);
//}

void Imager::keyReleaseEvent(QKeyEvent *event)
{
    if (!plotseqmodestarted && !intervalimaging)
    {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && fromnative == true)
    {
        fromnative = false;
        return;
    }

    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        if (event->modifiers() & Qt::ShiftModifier)
        {
            if (plotidx > 0)
            {
                if (!IsValidSaveLoc)
                {
                    qCritical() << "Unknown save location.";
                    return;
                }

                if (cams->size() == 0)
                {
                    qCritical() << "No camera(s) connected.";
                    return;
                }

                singleshotfile(plotlist[plotidx - 1]);
                if (plotidx < plotlist.size())
                    qInfo() << "Next imaging " << (plotidx + 1) << " of " << plotlist.size() << " (Sample plot: " << tr(plotlist[plotidx].c_str()) << ").";
            }
            else
                qCritical() << "Missing previous image to recapture.";
        }
        else
        {
            if (!IsValidSaveLoc)
            {
                qCritical() << "Unknown save location.";
                return;
            }

            if (cams->size() == 0)
            {
                qCritical() << "No camera(s) connected.";
                return;
            }

            singleshotfile(plotlist[plotidx]);
            plotidx++;

            if (plotidx < plotlist.size())
                qInfo() << "Next imaging " << (plotidx + 1) << " of " << plotlist.size() << " (Sample plot: " << tr(plotlist[plotidx].c_str()) << ").";
            else
            {
                qInfo() << "Sample imaging sequence completed.";
                plotidx = 0;
                plotseqmodestarted = false;
            }
        }
    }
    else if(event->key() == Qt::Key_Escape && plotseqmodestarted)
    {
        qInfo() << "Sample imaging sequence interrupted.";
        plotidx = 0;
        plotseqmodestarted = false;
    }
    else if (event->key() == Qt::Key_Escape && intervalimaging)
    {
        inttimer->stop();
        intervalimaging = false;
        nthimage = 0;
        ehours = eminutes = eseconds = 0.0;
        //cams->stopsavethreads();
        qInfo() << "Interval imaging stopped.";
    }
    else
    {
        QMainWindow::keyReleaseEvent(event);
        return;
    }
}

bool Imager::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG")
    {
        MSG *msg = reinterpret_cast<MSG*>(message);

        if (msg->message == WM_INPUT)
        {
            UINT dwSize;

            GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[dwSize];

            if (lpb == NULL)
            {
                return false;
            }

            if (!GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)))
            {
                delete lpb;
                return false;
            }
            else
            {
                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEKEYBOARD && br->is_needed(raw->header.hDevice))
                {
                    static int idx = 0;
                    static bool shifted = false;
                    static char scanstr[256] = { '\0' };
                    static UINT currkey;
                    //UINT sch = MapVirtualKey(raw->data.keyboard.VKey, MAPVK_VK_TO_CHAR);
                    
                    //qDebug("flags : %x, VK code : %x, ", raw->data.keyboard.Flags, raw->data.keyboard.VKey);
                    //qDebug() << "VK Code : " << hex << raw->data.keyboard.VKey;

                    if (raw->data.keyboard.Flags == 1) // key released
                    {
                        if (raw->data.keyboard.VKey == VK_RETURN)
                        {
                            scanstr[idx] = '\0';
                            fromnative = true;
                            
                            if(fileprefix->text().length() > 0)
                                singleshotfile(fileprefix->text().toStdString() + "_" + scanstr);
                            else
                                singleshotfile(scanstr);
                            idx = 0;
                            //qDebug() << "Control : " << scanstr;
                        }
                        else if (raw->data.keyboard.VKey == VK_SHIFT)
                        {
                            shifted = false;
                            //qDebug() << "unshifted";
                        }
                        else
                        {
                            scanstr[idx] = currkey; //(shifted) ? toupper(currkey) : tolower(currkey);
                            idx++;
                        }
                    }
                    else // key pressed
                    {
                        if (raw->data.keyboard.VKey == VK_SHIFT)
                        {
                            shifted = true;
                            //qDebug() << "shifted";
                        }
                        else if (raw->data.keyboard.VKey != VK_RETURN)
                        {
                            currkey = MapVirtualKey(raw->data.keyboard.VKey, MAPVK_VK_TO_CHAR);
                            //qDebug() << "currkey : " << (char)currkey;
                            currkey = (shifted) ? toupper(currkey) : tolower(currkey);
                            //qDebug() << "currkey shifted: " << (char)currkey;
                        }
                    }

                    delete lpb;
                    return true;
                }
                else
                {
                    raw = nullptr;
                    delete lpb;
                    return false;
                }
            }
        }
        else if (msg->message == WM_INPUT_DEVICE_CHANGE)
        {
            br->initialize();
            return true;
        }
    }

    return false;
}

void Imager::timerEvent(QTimerEvent * ev)
{
    static int pcamsize = 0;
    int camsize = cams->size();
    //tic();
    double sec = 0;
    if (ev->timerId() != timerid)
        return;

    //clock_t start = clock(), end;

    if (cams->size() == 0)
        return;

    if (!cams->threadcomplete)
        return;
    else if (initialized)
    {
        paintthread.join();
        image->setImage(img, area->GetScale());
    }

    initialized = true;

    paintthread = std::thread([&]()
    {
        cams->threadcomplete = false;

        if (cams->GetLiveImage(liveimg))
        {
            if (!liveimg.empty())
            {
                liveimg.copyTo(liveclone);
                if (liveimg.channels() == 1)
                    img = QImage(liveclone.data,
                        liveclone.cols, liveclone.rows, liveclone.step,
                        QImage::Format_Grayscale8).copy();
                else
                    img = QImage(liveclone.data,
                        liveclone.cols, liveclone.rows, liveclone.step,
                        QImage::Format_RGB888).copy();
                //QPixmap pmap = QPixmap::fromImage(img);
                
            }
        }

        cams->threadcomplete = true;
    });

    if (cams->liveimagingstarted && img.height() > 0) // && image->GetImageSetFlag())
    {
        //qInfo() << "image->pixmap()->size() = " << image->pixmap()->size();
        //qInfo() << "area->GetScale() = " << area->GetScale();
        //qInfo() << img.height(); // image->pixmap()->size();
        if (pcamsize == 0)
        {
            pcamsize = camsize;
            //qInfo() << "paint: " << img.size() << " scale: " << scale;
            image->setPixmap(QPixmap::fromImage(img).scaled(img.width() * area->GetScale(), img.height() * area->GetScale(), Qt::AspectRatioMode::KeepAspectRatio));
            image->adjustSize();
        }
        image->setPixmap(QPixmap::fromImage(img)); // .scaled(img.width() * area->GetScale(), img.height() * area->GetScale(), Qt::AspectRatioMode::KeepAspectRatio));
        ////image->adjustSize();
        ////image->ResetImagesetFlag();
        cams->liveimagingstarted = false;
        ////area->SetScale(area->GetScale());
        //
        //double scale = area->GetScale();
        //image->resize(area->GetScale() * image->pixmap()->size());
        //area->horizontalScrollBar()->setValue(int(area->GetScale() * area->horizontalScrollBar()->value()
        //    + ((area->GetScale() - 1) * area->horizontalScrollBar()->pageStep() / 2)));
        //area->verticalScrollBar()->setValue(int(area->GetScale() * area->verticalScrollBar()->value()
        //    + ((area->GetScale() - 1) * area->verticalScrollBar()->pageStep() / 2)));

        ////updateimage(area->GetScale() * 1000.0);
        ////area->update();

        //qInfo() << "img.height() = " << img.height() << "img.width() = " << img.width();
        //qInfo() << "image.height() = " << image->height() << "image.width() = " << image->width();
        //qInfo() << "image->pixmap()->height() = " << image->pixmap()->height() << "image->pixmap()->width() = " << image->pixmap()->width();
    }
}

void Imager::singleshot()
{
    singleshotfile(fileprefix->text().toStdString());
}

void Imager::singleshotfile(string filename)
{
    if (!IsValidSaveLoc)
    {
        qCritical() << "Unknown save location.";
        return;
    }

    if (cams->size() == 0)
    {
        qCritical() << "No camera(s) connected.";
        return;
    }

    if (filename.length() > 0)
        cams->shoot(savefilepath + filename + "_");
    else
        cams->shoot(savefilepath);

    if (enablebeep->checkState() == Qt::CheckState::Checked)
    {
        beeper->stop();
        beeper->play(); //QApplication::beep();
    }
}

void Imager::refreshdevices()
{
    cams->reinit();

    if (cams->camerror)
        return;

    // Refresh UI
    int camidx = dockcontainer->indexOf(cameras);
    dockcontainer->removeWidget(cameras);
    cameras->deleteLater();

    cameras = new QTreeWidget(docker);
    cameras->setColumnCount(2);
    cameras->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

    sliders.clear();
    editors.clear();
    decimals.clear();
    camids.clear();
    props.clear();

    createCameraControls();
    cameras->collapseAll();

    dockcontainer->insertWidget(camidx, cameras);

    if (cams->size() < 1)
    {
        update();
        return;
    }

    LoadProfile(currentprofile);

    update();
}

ImageLabel::ImageLabel()
{
    cams = VisionCameraArray::GetInstance();
}

ImageLabel::ImageLabel(QWidget * parent) : QLabel(parent)
{
    cams = VisionCameraArray::GetInstance();
}

void ImageLabel::setImage(const QImage & qimg, double scalev)
{
    img = qimg;
    scale = scalev;
    update();
}

void ImageLabel::paintEvent(QPaintEvent *e)
{
    static int pcamsize = 0;
    int camsize = cams->size();
    QPainter painter;
    painter.begin(this);

    painter.setClipRect(e->rect());
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.scale(scale, scale);
    //QPixmap pmap = QPixmap::fromImage(img);
    //qInfo() << "paint out: " << " scale: " << scale;

    if (camsize > 0 && cams->IsOpen(cams->getLiveID()) && img.height() > 0)
    {
        painter.drawImage(QPointF(0, 0), img);

        /*if (pcamsize == 0)
        {
            pcamsize = camsize;
            qInfo() << "paint: " << img.size() << " scale: " << scale;
            this->resize(scale * img.size());
        }*/
    }
    //imageset = true;
}

/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: Imager.h

Author: Anand Seethepalli (aseethepalli@noble.org)
Principal Investigator: Larry York (lmyork@noble.org)
Root Phenomics Lab
Noble Research Institute, LLC

This file is part of RhizoVision Imager.

RhizoVision Imager is free software: you can redistribute it and/or modify
it under the terms of the NOBLE RESEARCH INSTITUTE, GENERAL PUBLIC LICENSE.

RhizoVision Imager is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
NOBLE RESEARCH INSTITUTE GENERAL PUBLIC LICENSE for more details.

You should have received a copy of the Noble Research Institute General Public License
along with RhizoVision Imager.  If not, see <https://github.com/noble-research-institute/RhizoVisionImager/blob/master/LICENSE>.
*/

#pragma once

#ifndef IMAGER_H
#define IMAGER_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>
#include <QtMultimedia/QSound>

#include "VisionCameraArray.h"
#include "BarcodeReader.h"
#include "resource.h"

using namespace std;

#define RHIZOVISION_IMAGER_VERSION "1.0.1"

class ImageLabel : public QLabel
{
    Q_OBJECT;

    QImage img;
    double scale = 1;
    VisionCameraArray *cams;
public:
    ImageLabel();
    ImageLabel(QWidget *parent);

    void setImage(const QImage &qimg, double scalev);

protected:
    virtual void paintEvent(QPaintEvent *e);
};

class ScrollArea : public QScrollArea
{
    Q_OBJECT;

private:
    double scale = 1.0;

protected:
    virtual void wheelEvent(QWheelEvent *);

public:
    ScrollArea();
    ScrollArea(QWidget *parent);

    double GetScale();
    void SetScale(double);

signals:
    void scaleChanged(int);
};


class Imager : public QMainWindow
{
    Q_OBJECT;

    int liveviewcamid = -1;
    ScrollArea *area;
    ImageLabel *image;

    // General imaging settings
    QLineEdit *fileprefix;
    QLineEdit *savepath;
    QLabel *validpath;
    QPushButton *browse;
    //QCheckBox *outputsegmented;
    QPushButton *singleshotbtn;
    QCheckBox *chkbarcode;
    QCheckBox *enablebeep;
    QVBoxLayout *dockcontainer;

    QSound *beeper;

    // TODO later : timed imaging
    QCheckBox *timedimaging;
    QToolBox *timedoptions;

    QLabel *labcameras;
    QTreeWidget *cameras;
    QList<QSlider *> sliders;
    QList<QLineEdit *> editors;
    QList<int> decimals;
    QList<int> camids;
    QList<QString> props;

    // For live view
    int livecamid = -1, prevlivecamid = -1;
    //QComboBox *liveviewcamera;
    int timerid;
    Mat liveimg, liveclone;
    QPixmap *pmap = nullptr;
    QActionGroup *menuactiongroup = nullptr;
    QMenu *livecammenu;

    // For loading and saving profiles.
    QActionGroup *profileactiongroup = nullptr;
    QMenu *profilemenu;
    QList<QString> profiles;
    QString currentprofile = "";

    QActionGroup *formatactiongroup = nullptr;
    QMenu *formatmenu;
    QList<QString> formatdesc = { "Bitmap image (*.bmp)", "JPEG image (*.jpg)", "PNG image (*.png)", "TIFF image (*.tiff)" };
    QList<QString> formats = { "BMP", "JPG", "PNG", "TIFF" };

    // Docking cpabilities for options
    QDockWidget *docker, *logdocker;
    
    // File save path
    string savefilepath;
    bool IsValidSaveLoc = false;
    //bool outputsegmentedimage = false;
    bool enablebarcode = false;
    long long timerinterval = 0;

    // Camera specific
    VisionCameraArray *cams;

    // A tristate variable for slider and line editor
    // event handling. Used to remove race condition
    // against recursive event handling.
    int updateslidermode = 0;

    BarcodeReader *br;

    RAWINPUTDEVICE Rid[1];

    vector<string> plotlist;
    int plotidx = 0;
    bool plotseqmodestarted = false;
    bool fromnative = false;

    bool intervalimaging = false;
    QTimer *inttimer;
    int hours, minutes;
    double seconds;

    int ehours, eminutes;
    double eseconds;

    int nthimage;
private:
    void createCameraControls();
    void createSliderControl(int camid, QTreeWidgetItem *rootitem,
        string propname, string proptext, int dec);
    void createmenus();
    void createprofilecontrols();
    void updateLabel();
    void profilecreateorupdate();
    //void outputsegmentedchanged();
    void chkbarcodechanged();
    void barcodechanged();
    
protected:
    //bool event(QEvent* ev);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
    void timerEvent(QTimerEvent * ev);
    void closeEvent(QCloseEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private slots:
    void updateText();
    void updateSlider();
    void browser();
    void opencsv();
    void updateimage(int scale);
    void livecamerachanged();
    void profilechanged();
    void formatchanged();
    // Menu actions
    void singleshot();
    void singleshotfile(string filename);
    void refreshdevices();
    void intervaltimeout();
    void startintervalimaging();
    void about();

    void zoomout();
    void zoomin();
    void zoomfit();

    void LoadSettings();
    void SaveSettings();

    void LoadProfile(QString profilename);
    void SaveProfile(QString profilename);

public:
    Imager();
    ~Imager();

    static QTextEdit *logbox;
    static bool showDebugMessages;

    bool initialized = false;
    std::thread paintthread;
    QImage img;
};

#endif


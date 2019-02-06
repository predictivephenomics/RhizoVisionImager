/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: VisionCameraArray.h

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

#pragma once

#ifndef VISIONCAMERAARRAY_H
#define VISIONCAMERAARRAY_H

#include <cvutil.h>

#include <QtWidgets/QMessageBox>

#include <genapi\Types.h>
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCameraArray.h>
#ifdef PYLON_WIN_BUILD
#include <pylon/PylonGUI.h>
#endif

using namespace std;
using namespace cv;
using namespace Pylon;
using namespace GenICam;
using namespace Basler_UsbCameraParams;

class VisionCameraArray
{
private:
    VisionCameraArray();

public:
    static VisionCameraArray* GetInstance();

    // To prevent copies
    VisionCameraArray(VisionCameraArray const&) = delete;
    void operator=(VisionCameraArray const&) = delete;

    ~VisionCameraArray();

    // Initialize camera array.
    void init();

    // Reinitialize camera array.
    void reinit();

    void destroy();

    enum class ValType {CurrentValue, MinValue, MaxValue};

    double gain(int index, ValType type = ValType::CurrentValue);
    void setgain(int index, double val);
    double gamma(int index, ValType type = ValType::CurrentValue);
    void setgamma(int index, double val);
    double exposure(int index, ValType type = ValType::CurrentValue);
    void setexposure(int index, double val);

    string serial(int index);
    int size();
    bool IsOpen(int index);

    void shoot(string fileprefix);

    double GetValue(int index, string parameter, ValType type);
    void SetValue(int index, string parameter, double val);

    void setLiveID(int index);
    int getLiveID();
    void initLiveImaging();
    bool GetLiveImage(Mat &live);
    
    void setsaveformat(string format);
    bool IsColorCam(int index);
    void stopsavethreads();
    //void savethreadfunc();

    bool liveimagingstarted = false;

    bool camerror = false;
    bool threadcomplete = false;
private:
    // No further initialization is needed once the object 
    // below is added to the class.
    PylonAutoInitTerm autoInitTerm;
    CImageFormatConverter converter;

    CBaslerUsbInstantCameraArray cameras;
    int liveid = 0;
    CGrabResultPtr *liveGrabResult;
    CPylonImage convertedimage;
    string saveformat = "PNG";
    bool SavingLiveCam = false;
    vector<int> params;
    vector<CPylonImage> images;

    const int nqueue = 20;
    int saveidx = 0;
    vector<Mat> matimages;
    vector<string> savepath;
    vector<bool> indices;

    thread savthread;
    bool stopthread = false;
    bool startjoin = false;

    //vector<vector<stringstream>> filepath;
    
    //bool threadsjoined = true;
};

#endif // !VISIONCAMERAARRAY_H



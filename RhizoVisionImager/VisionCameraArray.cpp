/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: VisionCameraArray.cpp

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

#include "VisionCameraArray.h"

#ifdef WIN32
#define TARGET_WINDOWS
#endif

#ifdef TARGET_WINDOWS
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#include <io.h>
#define access _access
#define F_OK 0
#endif

#include <QtCore/QDebug>

using namespace GenApi_3_0_Basler_pylon_v5_0;

VisionCameraArray* VisionCameraArray::GetInstance()
{
    static VisionCameraArray instance;
    return &instance;
}

VisionCameraArray::VisionCameraArray()
{
    liveGrabResult = nullptr;
    converter.OutputBitAlignment = OutputBitAlignment_LsbAligned;
}

VisionCameraArray::~VisionCameraArray()
{
    destroy();
}

//void VisionCameraArray::savethreadfunc()
//{
//    int idx = 0;
//    int endidx = 0;
//    int aidx = 0;
//
//    while (true)
//    {
//        if (stopthread)
//            break;
//
//        Sleep(50);
//
//        if (saveidx < idx)
//            endidx = saveidx + nqueue;
//        else
//            endidx = saveidx;
//
//        for (; idx < endidx; idx++)
//        {
//            aidx = (idx % nqueue);
//            imwrite(savepath[aidx], matimages[aidx]);
//        }
//    }
//}

void VisionCameraArray::init()
{
    try
    {
        long hmax = 0, wmax = 0;
        CTlFactory& tlFactory = CTlFactory::GetInstance();
        DeviceInfoList_t devices;
        if (tlFactory.EnumerateDevices(devices) == 0)
        {
            qCritical() << "No camera(s) detected.";
            cameras.Initialize(0);
            return;
        }

        cameras.Initialize(devices.size());

        matimages.resize(nqueue);
        //threads.resize(nqueue);
        savepath.resize(nqueue);
        indices.resize(nqueue);
        //thstarted.resize(nqueue);
        images.resize((int)cameras.GetSize());

        for (size_t i = 0; i < cameras.GetSize(); ++i)
        {
            cameras[i].Attach(tlFactory.CreateDevice(devices[i]));
            cameras[i].Open();

            if(IsColorCam(i))
            {
                if (IsWritable(cameras[i].PixelFormat))
                {
                    if (cameras[i].PixelFormat.GetValue() == PixelFormat_BayerBG8)
                        cameras[i].PixelFormat.SetValue(PixelFormat_BayerBG8);
                    else if (cameras[i].PixelFormat.GetValue() == PixelFormat_BayerGB8)
                        cameras[i].PixelFormat.SetValue(PixelFormat_BayerGB8);
                    else if (cameras[i].PixelFormat.GetValue() == PixelFormat_BayerGR8)
                        cameras[i].PixelFormat.SetValue(PixelFormat_BayerGR8);
                    else
                        qWarning() << "Camera #" << (i + 1) << " - (" << (serial(int(i)) + ")").c_str() << " - Cannot set the pixel format.";
                }

                //matimages[i].create(hmax, wmax, CV_8UC3);
            }
            else
            {
                if (IsWritable(cameras[i].PixelFormat))
                    cameras[i].PixelFormat.SetValue(PixelFormat_Mono8);
                //matimages[i].create(hmax, wmax, CV_8UC1);
            }
            cameras[i].OutputQueueSize = 20;
            qInfo().nospace() << "Camera #" << (i + 1) << " - (" << (serial(int(i)) + ")").c_str() << " - Attached.";
        }

        for (int i = 0; i < cameras.GetSize(); i++)
        {
            hmax = (hmax < cameras[i].Height.GetValue()) ? cameras[i].Height.GetValue() : hmax;
            wmax = (wmax < cameras[i].Width.GetValue()) ? cameras[i].Width.GetValue() : wmax;
            //filepath[i].resize(cameras.GetSize());
        }

        for (int i = 0; i < nqueue; i++)
            matimages[i].create(hmax, wmax, CV_8UC3);

        savthread = thread([&]()
        {
            static int idx = 0;
            int endidx = 0;
            int aidx = 0;

            while (true)
            {
                if (stopthread)
                    break;

                Sleep(50);
                
                //cout << "Pre  - idx = " << idx << " - endidx = " << endidx << " - saveidx = " << saveidx << endl;
                if (idx >= nqueue)
                    idx -= nqueue;
                
                if (saveidx < idx)
                    endidx = saveidx + nqueue;
                else
                    endidx = saveidx;

                //cout << "Post - idx = " << idx << " - endidx = " << endidx << " - saveidx = " << saveidx << endl;
                for (; idx < endidx; idx++)
                {
                    aidx = (idx % nqueue);

                    //cout << "Now writing : " << savepath[aidx] << endl;
                    imwrite(savepath[aidx], matimages[aidx]);
                }
            }
        });

        startjoin = true;
        camerror = false;
        //threadsjoined = false;
    }
    catch (const GenericException & e)
    {
        qCritical() << "Cannot open devices:" + QString::fromStdString(e.GetDescription());
        camerror = true;
        //exit(0);
    }
}

void VisionCameraArray::reinit()
{
    unordered_map<string, double> gains, gammas, exposures;
    string liveserial;
    int id = 0;

    // Store the camera properties
    for (size_t i = 0; i < cameras.GetSize(); ++i)
    {
        if (!cameras[i].IsCameraDeviceRemoved() && IsOpen(i))
        {
            gains[serial(int(i))] = gain(i);
            gammas[serial(int(i))] = gamma(i);
            exposures[serial(int(i))] = exposure(i);
        }
    }

    // Store the serial id of live camera.
    if (size() > 0)
    {
        cameras[liveid].StopGrabbing();
        liveserial = serial(liveid);
    }

    destroy();
    init();

    // Set the properties back
    for (size_t i = 0; i < cameras.GetSize(); ++i)
    {
        for (auto kvpair : gains)
        {
            if (kvpair.first == serial(i))
            {
                setgain(i, gains[serial(int(i))]);
                setgamma(i, gammas[serial(int(i))]);
                setexposure(i, exposures[serial(int(i))]);

                break;
            }
        }
    }

    // Set the liveid back
    for (id = 0; id < size(); id++)
        if (serial(id) == liveserial)
            break;

    if (id == size())
        liveid = 0;
    else
        liveid = id;

    initLiveImaging();
}

void VisionCameraArray::destroy()
{
    cameras.Close();
    cameras.DetachDevice();

    if (matimages.size() > 0)
        for (size_t i = 0; i < nqueue; ++i)
            matimages[i].release();

    try
    {
        stopsavethreads();
    }
    catch (int)
    {

    }
}

double VisionCameraArray::gain(int index, ValType type)
{
    return GetValue(index, "Gain", ValType::CurrentValue);
}

void VisionCameraArray::setgain(int index, double val)
{
    SetValue(index, "Gain", val);
}

double VisionCameraArray::gamma(int index, ValType type)
{
    return GetValue(index, "Gamma", ValType::CurrentValue);
}

void VisionCameraArray::setgamma(int index, double val)
{
    SetValue(index, "Gamma", val);
}

double VisionCameraArray::exposure(int index, ValType type)
{
    return GetValue(index, "ExposureTime", ValType::CurrentValue);
}

void VisionCameraArray::setexposure(int index, double val)
{
    SetValue(index, "ExposureTime", val);
}

string VisionCameraArray::serial(int index)
{
    return string(cameras[index].GetDeviceInfo().GetSerialNumber().c_str());
}

int VisionCameraArray::size()
{
    return int(cameras.GetSize());
}

bool VisionCameraArray::IsOpen(int index)
{
    return cameras[index].IsOpen() && (!cameras[index].IsCameraDeviceRemoved());
}

void VisionCameraArray::stopsavethreads()
{
    /*if (threadsjoined)
        return;*/

    //for (int i = 0; i < nqueue; i++)
    //if (threads.size() > 0 && startjoin)
    //    threads[(imgwriteswitch + nqueue - 1) % nqueue].join();
    if (startjoin)
    {
        stopthread = true;
        savthread.join();
        stopthread = false;
        //threadsjoined = true;
        startjoin = false;
    }
}

void VisionCameraArray::shoot(string filepathprefix)
{
    int j = 0;
    int camsize = int(cameras.GetSize());
    //vector<CPylonImage> images(camsize);
    string sformat = saveformat;
    vector<int> params;
    vector<stringstream> filepath(camsize);
    QList<QString> formats = { "bmp", "jpg", "png", "tiff" };
    //Mat m;

    if (camsize == 0)
        return;

    std::transform(sformat.begin(), sformat.end(), sformat.begin(), ::tolower);
    
    CGrabResultPtr *ptrGrabResult = new CGrabResultPtr[camsize];
    int fexists = 0;

    for (size_t i = 0; i < camsize; i++)
    {
        filepath[i].clear();
        filepath[i].str("");
        filepath[i] << filepathprefix;
        filepath[i] << "c" << (i + 1) << "_p";
        
        for (j = 1; j < INT_MAX; j++)
        {
            fexists = 0;

            for (int k = 0; k < formats.size(); k++)
            {
                if (access((filepath[i].str() + to_string(j) + "." + formats[k].toStdString()).c_str(), F_OK) == -1)
                    fexists++;
            }

            if (fexists == formats.size())
                break;
        }

        filepath[i] << j << "." << sformat;
    }

    for (size_t i = 0; i < camsize; i++)
    {
        try
        {
            if (i != liveid)
            {
                cameras[i].StartGrabbing(1);
                cameras[i].RetrieveResult(5000, ptrGrabResult[i], TimeoutHandling_ThrowException);
            }
            else
            {
                while (!threadcomplete)
                    this_thread::sleep_for(chrono::milliseconds(2));
                threadcomplete = false;
                cameras[i].RetrieveResult(5000, ptrGrabResult[i], TimeoutHandling_ThrowException);
                threadcomplete = true;
            }
            //}
        }
        catch (const GenericException & e)
        {
            qCritical() << "(Camera #" << (i + 1) << " - (" << (serial(int(i)) + ")").c_str() << " - " << e.GetDescription();
        }
    }

    for (size_t i = 0; i < camsize; i++)
    {
        try
        {
            //globals.cameras[i].RetrieveResult( 5000, ptrGrabResult[i], TimeoutHandling_ThrowException);
            if (ptrGrabResult[i]->GrabSucceeded())
            {
                if (IsColorCam(i))
                {
                    converter.OutputPixelFormat = PixelType_BGR8packed;

                    converter.Convert(images[i], ptrGrabResult[i]);
                    Mat m = Mat(images[i].GetHeight(), images[i].GetWidth(), CV_8UC3, (uchar *)images[i].GetBuffer());
                    m.copyTo(matimages[saveidx]);
                    //matimages[saveidx] = Mat(images[i].GetHeight(), images[i].GetWidth(), CV_8UC3, (uchar *)images[i].GetBuffer()).clone();
                }
                else
                {
                    converter.OutputPixelFormat = PixelType_Mono8;
                    converter.Convert(images[i], ptrGrabResult[i]);
                    Mat m = Mat(images[i].GetHeight(), images[i].GetWidth(), CV_8UC1, (uchar *)images[i].GetBuffer());
                    m.copyTo(matimages[saveidx]);
                    //matimages[saveidx] = Mat(images[i].GetHeight(), images[i].GetWidth(), CV_8UC1, (uchar *)images[i].GetBuffer()).clone();
                }

                savepath[saveidx] = filepath[i].str();
                qInfo() << "Captured image: " << filepath[i].str().c_str();
                saveidx++;
                saveidx = (saveidx % nqueue);
            }
            else
                qCritical() << "(Camera #" << (i + 1) << " - (" << (serial(int(i)) + ")").c_str() << " - " << "Failed to download image.";
        }
        catch (const GenericException & e)
        {
            qCritical() << "(Camera #" << (i + 1) << " - (" << (serial(int(i)) + ")").c_str() << " - " << e.GetDescription();
        }
    }

    //threads[imgwriteswitch] = thread([&](int csize, int iswitch)
    //{
    //    for (size_t i = 0; i < csize; i++)
    //    {
    //        if (saveformat == "JPEG")
    //            imwrite(filepath[iswitch][i].str(), matimages[iswitch][i], params);
    //        else
    //            imwrite(filepath[iswitch][i].str(), matimages[iswitch][i]);// , params);
    //    }
    //}, camsize, imgwriteswitch);

    delete[] ptrGrabResult;
}

double VisionCameraArray::GetValue(int index, string parameter, ValType type)
{
    INodeMap &map = cameras[index].GetNodeMap();
    
    switch (type)
    {
    case ValType::CurrentValue:
        return (dynamic_cast<IFloat*>(map.GetNode(parameter.c_str())))->GetValue();
    case ValType::MinValue:
        return (dynamic_cast<IFloat*>(map.GetNode(parameter.c_str())))->GetMin();
    case ValType::MaxValue:
        return (dynamic_cast<IFloat*>(map.GetNode(parameter.c_str())))->GetMax();
    default:
        return -1;
    }
}

void VisionCameraArray::SetValue(int index, string parameter, double val)
{
    INodeMap &map = cameras[index].GetNodeMap();
    CFloatPtr p = map.GetNode(parameter.c_str());

    if (IsWritable(p))
    {
        try
        {
            p->SetValue(val);
        }
        catch (const GenericException & e)
        {
            /*QMessageBox::critical(0, ("Error (Camera #" + to_string(index + 1) + " - (" + serial(index) + ")").c_str(), 
                e.GetDescription(), QMessageBox::StandardButton::Ok);*/
        }
    }
    else
        qCritical() << "(Camera #" << (index + 1) << " - (" << (serial(int(index)) + ")").c_str() << " - " << "Cannot set the value (read-only?).";
}

int VisionCameraArray::getLiveID()
{
    return liveid;
}

void VisionCameraArray::setLiveID(int index)
{
    int grabbing = 0;

    if (cameras.GetSize() == 0)
        return;

    if (cameras[liveid].IsGrabbing())
    {
        grabbing = 1;
        cameras[liveid].StopGrabbing();
    }

    liveid = index;

    if (grabbing)
        initLiveImaging();
}

bool VisionCameraArray::GetLiveImage(Mat &live)
{
    if (cameras.GetSize() == 0)
        return false;

    if (liveGrabResult == nullptr)
        liveGrabResult = new CGrabResultPtr[1];
    
    if (liveid >= size())
        return false;

    if (SavingLiveCam)
        return false;

    try
    {
        if (cameras[liveid].IsGrabbing())
        {
            cameras[liveid].RetrieveResult(5000, *liveGrabResult, TimeoutHandling_Return);
            
            if (liveGrabResult[0]->GrabSucceeded())
            {
                if (IsColorCam(liveid))
                {
                    converter.OutputPixelFormat = PixelType_RGB8packed;

                    converter.Convert(convertedimage, liveGrabResult[0]);
                    live = Mat(convertedimage.GetHeight(), convertedimage.GetWidth(), CV_8UC3, (uchar *)convertedimage.GetBuffer());
                }
                else
                {
                    converter.OutputPixelFormat = PixelType_Mono8;
                    converter.Convert(convertedimage, liveGrabResult[0]);
                    live = Mat(convertedimage.GetHeight(), convertedimage.GetWidth(), CV_8UC1, (uchar *)convertedimage.GetBuffer());
                }
            }

            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const GenericException &e)
    {
        if (cameras[liveid].IsCameraDeviceRemoved())
        {
            for (int id = 0; id < size(); id++)
            {
                if (!cameras[id].IsCameraDeviceRemoved())
                {
                    liveid = id;
                    initLiveImaging();
                    break;
                }
            }
        }

        return false;
    }
}

void VisionCameraArray::initLiveImaging()
{
    if (cameras.GetSize() == 0)
        return;

    if (cameras[liveid].IsOpen())
    {
        /*if (liveid >= cameras.GetSize())
            liveid = 0;*/

        cameras[liveid].StartGrabbing(Pylon::EGrabStrategy::GrabStrategy_LatestImageOnly);
        liveimagingstarted = true;
    }
}

bool VisionCameraArray::IsColorCam(int index)
{
    GenApi::NodeList_t Entries;
    cameras[index].PixelFormat.GetEntries(Entries);
    bool Result = false;

    for (size_t i = 0; i < Entries.size(); i++)
    {
        GenApi::INode *pNode = Entries[i];
        if (IsAvailable(pNode))
        {
            GenApi::IEnumEntry *pEnum = dynamic_cast<GenApi::IEnumEntry *>(pNode);
            const GenICam::gcstring sym(pEnum->GetSymbolic());
            if (sym.find(GenICam::gcstring("Bayer")) != GenICam::gcstring::_npos())
            {
                Result = true;
                break;
            }
        }
    }
    
    return Result;
}

void VisionCameraArray::setsaveformat(string format)
{
    saveformat = format;

    if (saveformat == "JPEG")
    {
        params.clear();
        params.push_back(IMWRITE_JPEG_QUALITY);
        params.push_back(98);
    }
    else
        params.clear();
}

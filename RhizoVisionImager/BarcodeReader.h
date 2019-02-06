/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: BarcodeReader.h

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

#ifndef BARCODEREADER_H
#define BARCODEREADER_H

#include <cvutil.h>

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <libusb-1.0/libusb.h>

//#include "hidapi.h"

// BarcodeReader - Interface to USB barcode scanner devices.
// Historically barcode scanners were connected as RS-232 serial
// connector.
class BarcodeReader : public QObject
{
    Q_OBJECT;

    typedef struct
    {
        uint8_t  bLength = 0;
        uint8_t  bDescriptorType = 0;
        char bcdHID[10];
        uint8_t  bCountryCode = 0;
        uint8_t  bNumDescriptors = 0;
        uint8_t  bClassDescriptorType = 0;
        uint16_t  wClassDescriptorLength = 0;

        std::vector<uint8_t> bClassDescriptorTypes;
        std::vector<uint16_t> wClassDescriptorLengths;
    } libusb_hid_descriptor;

    typedef struct
    {
        int epnum;
        int maxpacketsize;
        int pollinginterval;
        char direction[10];
        char transfer_type[20];
        char sync_type[20];
        char usage_type[40];
    } epinfo;

    typedef struct
    {
        libusb_hid_descriptor desc;
        int config_num;
        int interface_num;
        int alt_num;
        int usagepage;
        int usage;
        int rdlen;
        uint8_t rd[256];
        libusb_device *dev;
        DWORD idProduct;
        DWORD idVendor;
        HANDLE devhandle;
        int devhandle_unknown;

        std::vector<epinfo> ep;
    } keyboard_info;

    struct input_report {
        uint8_t *data;
        size_t len;
        struct input_report *next;
    };

    libusb_device **devices; //pointer to pointer of device, used to retrieve a list of devices
    libusb_context *context = nullptr; //a libusb session

    // List that maintains all USB HID keyboard devices that are not system reserved.
    std::vector<keyboard_info *> keyboards;

    int r; //for return values
    int ndevices = 0; //holding number of devices in list
    
    libusb_device_handle* hdev;
    struct input_report *reports = nullptr;
private:
    
    bool IsHIDKeyboard(libusb_device *dev, keyboard_info* info);
    void parse_rd(keyboard_info* info);

    libusb_hid_descriptor BarcodeReader::libusb_get_hid_descriptor(libusb_device* dev, int cnum, int inum, int anum);

    std::string GetDeviceInfo(libusb_device *dev);
    bool ReportError(std::string message);
    std::string GetDeviceClass(uint8_t devclass);

    static void LIBUSB_CALL read_callback(struct libusb_transfer *transfer);
public:
    BarcodeReader(QObject *parent);
    //static BarcodeReader* GetInstance(Imager *parent);

    BarcodeReader(BarcodeReader const&) = delete;
    void operator=(BarcodeReader const &) = delete;

    ~BarcodeReader();

    void initialize();
    bool is_needed(HANDLE hdl);
    //void init_listener();
    //void run();

//signals:
    //void ReportProgress(QString text);

};

#endif

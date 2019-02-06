/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: BarcodeReader.cpp

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


#include "BarcodeReader.h"

#include <QtCore/QDebug>
#include <QtWidgets/QMessageBox>

using namespace std;

typedef struct
{
    uint8_t *data;
    size_t len;
} report;

BarcodeReader::libusb_hid_descriptor BarcodeReader::libusb_get_hid_descriptor(libusb_device* dev, int cnum, int inum, int anum)
{
    libusb_hid_descriptor result;
    libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    
    const libusb_interface *interface;
    libusb_config_descriptor *config;
    const libusb_interface_descriptor *interdesc;
    
    const unsigned char* extra;
    int extra_length;

    for (int i = 0; i < (int)desc.bNumConfigurations; i++)
    {
        if (i != cnum)
            continue;

        r = libusb_get_config_descriptor(dev, i, &config);

        if (r < 0)
            return result;

        for (int j = 0; j < (int)config->bNumInterfaces; j++)
        {
            if (j != inum)
                return result;

            interface = &config->interface[i];

            for (int k = 0; k < interface->num_altsetting; k++)
            {
                if (k != anum)
                    return result;

                interdesc = &interface->altsetting[k];

                extra = interdesc->extra;
                extra_length = interdesc->extra_length;

                result.bLength = extra[0];
                result.bDescriptorType = extra[1];
                sprintf(result.bcdHID, "%2d.%02d", extra[3], extra[2]);
                result.bCountryCode = extra[4];
                result.bNumDescriptors = extra[5];

                result.bClassDescriptorType = extra[6];
                result.wClassDescriptorLength = extra[8] << 8 | extra[7];

                for (uint8_t p = 1; p < result.bNumDescriptors; p++)
                {
                    result.bClassDescriptorTypes.push_back(extra[3 * p + 6]);
                    result.wClassDescriptorLengths.push_back(extra[3 * p + 8] << 8 | extra[3 * p + 7]);
                }
            }
        }
    }

    return result;
}

#define RDDATA    ((ds == 3) ? ((info->rd[index + 4] << 24) + (info->rd[index + 3] << 16) + (info->rd[index + 2] << 8) + info->rd[index + 1]) : ((ds == 2) ? ((info->rd[index + 2] << 8) + info->rd[index + 1]) : info->rd[index + 1]))
#define IDXINC    index += (((ds == 3) ? 4 : ds) + 1)

#define MNEMONIC(x)     \
if (ds > 0) \
qDebug().noquote() << x << " (0x" << hex << RDDATA << ")"; \
else \
qDebug().noquote() << x << " ()"; \
break;

void BarcodeReader::parse_rd(keyboard_info* info)
{
    int index = 0;
    uint8_t d, ds;
    
    index = 0;
    while (index < info->rdlen)
    {
        d = info->rd[index] & 0xfc;
        ds = info->rd[index] & 0x3;

        switch (d)
        {
            //Global tags
        case 4:
            MNEMONIC("Usage page");
        case 20:
            MNEMONIC("Local Minimum");
        case 36:
            MNEMONIC("Local Maximum");
        case 52:
            MNEMONIC("Physical Minimum");
        case 68:
            MNEMONIC("Physical Maximum");
        case 84:
            MNEMONIC("Unit exponent");
        case 100:
            MNEMONIC("Unit");
        case 116:
            MNEMONIC("Report size");
        case 132:
            MNEMONIC("Report ID");
        case 148:
            MNEMONIC("Report count");
        case 164:
            MNEMONIC("Push");
        case 180:
            MNEMONIC("Pop");
        
            // Main tags
        case 128:
            MNEMONIC("Input");
        case 144:
            MNEMONIC("Output");
        case 176:
            MNEMONIC("Feature");
        case 160:
            MNEMONIC("Collection");
        case 192:
            MNEMONIC("End collection");

            // Local tags
        case 8:
            MNEMONIC("Usage");
        case 24:
            MNEMONIC("Usage minimum");
        case 40:
            MNEMONIC("Usage maximum");
        case 56:
            MNEMONIC("Designator index");
        case 72:
            MNEMONIC("Designator minimum");
        case 88:
            MNEMONIC("Designator maximum");
        case 120:
            MNEMONIC("String index");
        case 136:
            MNEMONIC("String minimum");
        case 152:
            MNEMONIC("String maximum");
        case 168:
            MNEMONIC("Delimiter");
        }

        IDXINC;
    }
    
    qDebug();
}

BarcodeReader::BarcodeReader(QObject *parent) : QObject(parent)
{
    //qDebug() << "USB Initializing...";
    if (context == nullptr && libusb_init(&context) < 0)
    {
        ReportError("USB Initialization Error.");
        //qDebug() << "USB Initialized successfully.";

#ifdef _DEBUG
        libusb_set_debug(context, LIBUSB_LOG_LEVEL_DEBUG);
#else
        libusb_set_debug(context, LIBUSB_LOG_LEVEL_NONE);
#endif
    }
}

BarcodeReader::~BarcodeReader()
{
    libusb_free_device_list(devices, 1);
    libusb_exit(context);
}

void BarcodeReader::initialize()
{
    keyboard_info *ki;
    ndevices = libusb_get_device_list(context, &devices); //get the list of devices

    if (ndevices < 0)
        ReportError("Get Device Error"); //there was an error

    keyboards.clear();
    ki = new keyboard_info();
    for (int i = 0; i < ndevices; i++)
    {
        if (IsHIDKeyboard(devices[i], ki))
        {
            qDebug().noquote() << "[HID Keyboard]";
            qDebug() << "[HID Keyboard]: Num descriptors: " << (int)ki->desc.bNumDescriptors;
            qDebug() << "[HID Keyboard]: bcdHID: " << ki->desc.bcdHID;
            qDebug() << "[HID Keyboard]: Usage page: 0x" << hex << ki->usagepage;
            qDebug() << "[HID Keyboard]: Usage: 0x" << hex << ki->usage;
            for (int t = 0; t < ki->ep.size(); t++)
            {
                qDebug() << "[HID Keyboard]: \tEndpoint : " << t;

                qDebug() << "[HID Keyboard]: \t\tEndpoint address: " << ki->ep[t].epnum;
                qDebug() << "[HID Keyboard]: \t\tDirection: " << ki->ep[t].direction;
                qDebug() << "[HID Keyboard]: \t\tMax packet size: " << ki->ep[t].maxpacketsize;
                qDebug() << "[HID Keyboard]: \t\tPolling interval: " << ki->ep[t].pollinginterval;
                qDebug() << "[HID Keyboard]: \t\tTransfer type: " << ki->ep[t].transfer_type;
                qDebug() << "[HID Keyboard]: \t\tSynchronization type: " << ki->ep[t].sync_type;
                qDebug() << "[HID Keyboard]: \t\tUsage type: " << ki->ep[t].usage_type;
            }

            /*qDebug() << "[HID Keyboard]: Report descriptor";
            parse_rd(ki);*/
            qDebug().noquote() << QString::fromStdString(GetDeviceInfo(devices[i]));
            keyboards.push_back(ki);

            ki = new keyboard_info();
        }
    }

    // Sort keyboards based on polling interval
    sort(keyboards.begin(), keyboards.end(),
        [](const keyboard_info* a, const keyboard_info* b) -> bool
    {
        int pi1 = 100000, pi2 = 100000;

        for (auto &ep : a->ep)
            if (strcmp(ep.direction, "Input") == 0 && ep.pollinginterval < pi1)
                pi1 = ep.pollinginterval;
        for (auto &ep : b->ep)
            if (strcmp(ep.direction, "Input") == 0 && ep.pollinginterval < pi2)
                pi2 = ep.pollinginterval;

        return pi1 < pi2;
    });

    if (keyboards.size() < 1 || (keyboards.size() >= 1 && keyboards[0]->ep[0].pollinginterval > 10))
        qWarning() << "Barcode reader not found.";
    else
        qInfo() << "Barcode reader found.";
}

bool BarcodeReader::ReportError(string message)
{
    qCritical() << QString::fromStdString(message);
    return false;
}

string BarcodeReader::GetDeviceClass(uint8_t _devclass)
{
    string devclass = "";

    switch (_devclass)
    {
    case LIBUSB_CLASS_PER_INTERFACE:
        devclass = "Defined per interface";
        break;
    case LIBUSB_CLASS_AUDIO:
        devclass = "Audio";
        break;
    case LIBUSB_CLASS_COMM:
        devclass = "Communications";
        break;
    case LIBUSB_CLASS_HID:
        devclass = "Human Interface Device";
        break;
    case LIBUSB_CLASS_PHYSICAL:
        devclass = "Physical";
        break;
    case LIBUSB_CLASS_PRINTER:
        devclass = "Printer";
        break;
    case LIBUSB_CLASS_IMAGE: // Same as the legacy LIBUSB_CLASS_PTP
        devclass = "Image";
        break;
    case LIBUSB_CLASS_MASS_STORAGE:
        devclass = "Mass storage";
        break;
    case LIBUSB_CLASS_HUB:
        devclass = "Hub";
        break;
    case LIBUSB_CLASS_DATA:
        devclass = "Data";
        break;
    case LIBUSB_CLASS_SMART_CARD:
        devclass = "Smart Card";
        break;
    case LIBUSB_CLASS_CONTENT_SECURITY:
        devclass = "Content Security";
        break;
    case LIBUSB_CLASS_VIDEO:
        devclass = "Video";
        break;
    case LIBUSB_CLASS_PERSONAL_HEALTHCARE:
        devclass = "Personal Healthcare";
        break;
    case 0x10:
        devclass = "Audio/Video Device";
        break;
    case 0x11:
        devclass = "Billboard Device Class";
        break;
    case 0x12:
        devclass = "USB Type-C Bridge Class";
        break;
    case LIBUSB_CLASS_DIAGNOSTIC_DEVICE:
        devclass = "Diagnostic Device";
        break;
    case LIBUSB_CLASS_WIRELESS:
        devclass = "Wireless Controller";
        break;
    case 0xef:
        devclass = "Miscellaneous";
        break;
    case LIBUSB_CLASS_APPLICATION:
        devclass = "Application Specific";
        break;
    case LIBUSB_CLASS_VENDOR_SPEC:
        devclass = "Vendor Specific";
        break;
    }

    return devclass;
}

uint32_t get_bytes(uint8_t *rpt, size_t len, size_t num_bytes, size_t cur)
{
    /* Return if there aren't enough bytes. */
    if (cur + num_bytes >= len)
        return 0;

    if (num_bytes == 0)
        return 0;
    else if (num_bytes == 1) {
        return rpt[cur + 1];
    }
    else if (num_bytes == 2) {
        return (rpt[cur + 2] * 256 + rpt[cur + 1]);
    }
    else if (num_bytes == 4) {
        return (rpt[cur + 4] * 0x01000000 +
            rpt[cur + 3] * 0x00010000 +
            rpt[cur + 2] * 0x00000100 +
            rpt[cur + 1] * 0x00000001);
    }
    else
        return 0;
}

int get_usage(uint8_t *report_descriptor, size_t size,
    unsigned short *usage_page, unsigned short *usage)
{
    int i = 0;
    int size_code;
    int data_len, key_size;
    int usage_found = 0, usage_page_found = 0;

    while (i < size) {
        int key = report_descriptor[i];
        int key_cmd = key & 0xfc;

        //printf("key: %02hhx\n", key);

        if ((key & 0xf0) == 0xf0) {
            /* This is a Long Item. The next byte contains the
            length of the data section (value) for this key.
            See the HID specification, version 1.11, section
            6.2.2.3, titled "Long Items." */
            if (i + 1 < size)
                data_len = report_descriptor[i + 1];
            else
                data_len = 0; /* malformed report */
            key_size = 3;
        }
        else {
            /* This is a Short Item. The bottom two bits of the
            key contain the size code for the data section
            (value) for this key.  Refer to the HID
            specification, version 1.11, section 6.2.2.2,
            titled "Short Items." */
            size_code = key & 0x3;
            switch (size_code) {
            case 0:
            case 1:
            case 2:
                data_len = size_code;
                break;
            case 3:
                data_len = 4;
                break;
            default:
                /* Can't ever happen since size_code is & 0x3 */
                data_len = 0;
                break;
            };
            key_size = 1;
        }

        if (key_cmd == 0x4) {
            *usage_page = get_bytes(report_descriptor, size, data_len, i);
            usage_page_found = 1;
            //printf("Usage Page: %x\n", (uint32_t)*usage_page);
        }
        if (key_cmd == 0x8) {
            *usage = get_bytes(report_descriptor, size, data_len, i);
            usage_found = 1;
            //printf("Usage: %x\n", (uint32_t)*usage);
        }

        if (usage_page_found && usage_found)
            return 0; /* success */

                      /* Skip over this key and it's associated data */
        i += data_len + key_size;
    }

    return -1; /* failure */
}

int getdevicehandle(DWORD vid, DWORD pid, HANDLE* hdl)
{
    int nRetCode = 0;
    
    UINT nDevices;
    PRAWINPUTDEVICELIST pRawInputDeviceList;
    if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
    {
        return 1;
    }

    if ((pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == NULL)
    {
        return 1;
    }

    int nNoOfDevices = 0;
    if ((nNoOfDevices = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST))) == ((UINT)-1))
    {
        return 1;
    }

    RID_DEVICE_INFO rdi;
    rdi.cbSize = sizeof(RID_DEVICE_INFO);

    for (int i = 0; i < nNoOfDevices; i++)
    {
        UINT size = 256;
        TCHAR tBuffer[256] = { 0 };
        char dstname[256] = { 0 };
        string dname;

        if (GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, tBuffer, &size) < 0)
        {
            // Error in reading device name
            continue;
        }

        wcstombs(dstname, tBuffer, 256);
        dname = dstname;
        DWORD vid_ = stoul(dname.substr(dname.find("VID_") + 4, 4), nullptr, 16);
        DWORD pid_ = stoul(dname.substr(dname.find("PID_") + 4, 4), nullptr, 16);

        if (vid == vid_ && pid == pid_)
        {
            *hdl = pRawInputDeviceList[i].hDevice;
            return 0;
        }
    }
    
    free(pRawInputDeviceList);
    return 1;
}

bool BarcodeReader::IsHIDKeyboard(libusb_device *dev, BarcodeReader::keyboard_info* info)
{
    libusb_device_descriptor desc;
    libusb_hid_descriptor hid;
    int r = libusb_get_device_descriptor(dev, &desc);
    info->interface_num = -1;
    info->alt_num = -1;

    int nbytes = 0;
    unsigned char rd[256];
    
    bool driver_attached;
    const libusb_interface *interface;
    libusb_config_descriptor *config;
    const libusb_interface_descriptor *interdesc;
    libusb_device_handle *devhandle;
    const libusb_endpoint_descriptor *epdesc;
    int rcount = 0, icount = 0, acount = 0, iclass, isubclass, iprotocol;

    for (int i = 0; i < (int)desc.bNumConfigurations; i++)
    {
        r = libusb_get_config_descriptor(dev, i, &config);

        if (r < 0)
        {
            rcount++;
            continue;
        }

        icount = 0;
        for (int j = 0; j < (int)config->bNumInterfaces; j++)
        {
            interface = &config->interface[i];

            acount = 0;
            for (int k = 0; k < interface->num_altsetting; k++)
            {
                interdesc = &interface->altsetting[k];

                iclass = (int)interdesc->bInterfaceClass;

                if (iclass == 3)
                {
                    isubclass = (int)interdesc->bInterfaceSubClass;

                    if (isubclass == 1)
                    {
                        iprotocol = (int)interdesc->bInterfaceProtocol;

                        if (iprotocol == 1)
                        {
                            r = libusb_open(dev, &devhandle);

                            if (r < 0)
                            {
                                acount++;
                                continue;
                            }

                            //////////////
                            /// To query report descriptor.
                            if (libusb_kernel_driver_active(devhandle, interdesc->bInterfaceNumber) == 1)
                            {
                                driver_attached = true;
                                r = libusb_detach_kernel_driver(devhandle, interdesc->bInterfaceNumber);

                                if (r < 0)
                                {
                                    libusb_close(devhandle);
                                    driver_attached = false;
                                    acount++;
                                    continue;
                                }
                            }
                            else
                                driver_attached = false;

                            // To claim interface
                            r = libusb_claim_interface(devhandle, interdesc->bInterfaceNumber);

                            if (r < 0)
                            {
                                if (driver_attached)
                                    libusb_attach_kernel_driver(devhandle, interdesc->bInterfaceNumber);
                                libusb_close(devhandle);
                                driver_attached = false;
                                acount++;
                                continue;
                            }

                            r = libusb_control_transfer(devhandle, LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE, LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_REPORT << 8) | interdesc->bInterfaceNumber, 0, rd, sizeof(rd), 5000);
                            
                            if (r < 0)
                            {
                                libusb_release_interface(devhandle, interdesc->bInterfaceNumber);
                                if (driver_attached)
                                    libusb_attach_kernel_driver(devhandle, interdesc->bInterfaceNumber);
                                libusb_close(devhandle);
                                driver_attached = false;
                                acount++;
                                continue;
                            }

                            nbytes = r;
                            r = libusb_release_interface(devhandle, interdesc->bInterfaceNumber);

                            if (r < 0)
                            {
                                if (driver_attached)
                                    libusb_attach_kernel_driver(devhandle, interdesc->bInterfaceNumber);
                                libusb_close(devhandle);
                                driver_attached = false;
                                acount++;
                                continue;
                            }

                            if (driver_attached)
                            {
                                r = libusb_attach_kernel_driver(devhandle, interdesc->bInterfaceNumber);

                                if (r < 0)
                                {
                                    libusb_close(devhandle);
                                    driver_attached = false;
                                    acount++;
                                    continue;
                                }
                            }
                            
                            libusb_close(devhandle);
                            driver_attached = false;
                            //////////////

                            //------------------
                            // To get usage page and usage codes.
                            // We only identify USB HID Keyboard.
                            unsigned short up, u;
                            get_usage(rd, nbytes, &up, &u);
                            info->usagepage = (int)up;
                            info->usage = (int)u;
                            memcpy(info->rd, rd, 256);
                            info->rdlen = nbytes;
                            //------------------

                            hid = libusb_get_hid_descriptor(devices[i], i, j, k);

                            info->config_num = (int)config->bConfigurationValue;
                            info->interface_num = (int)interdesc->bInterfaceNumber;
                            info->alt_num = (int)interdesc->bAlternateSetting;

                            info->dev = dev;
                            info->desc = hid;

                            info->idProduct = (DWORD)desc.idProduct;
                            info->idVendor = (DWORD)desc.idVendor;

                            info->devhandle_unknown = getdevicehandle(info->idVendor, info->idProduct, &info->devhandle);

                            for (int t = 0; t < (int)interdesc->bNumEndpoints; t++)
                            {
                                epdesc = &interdesc->endpoint[t];
                                
                                epinfo ei;
                                
                                ei.epnum = (int)(epdesc->bEndpointAddress);
                                ei.maxpacketsize = (int)epdesc->wMaxPacketSize;
                                ei.pollinginterval = (int)epdesc->bInterval;

                                if (epdesc->bEndpointAddress & 0x80)
                                    sprintf(ei.direction, "Input");
                                else
                                    sprintf(ei.direction, "Output");

                                switch (epdesc->bmAttributes & 3)
                                {
                                case 0:
                                    sprintf(ei.transfer_type, "Control");
                                    sprintf(ei.sync_type, "Reserved");
                                    sprintf(ei.usage_type, "Reserved");
                                    break;
                                case 1:
                                    sprintf(ei.transfer_type, "Isochronous");
                                    switch ((epdesc->bmAttributes >> 2) & 3)
                                    {
                                    case 0:
                                        sprintf(ei.sync_type, "No Synchronization");
                                        break;
                                    case 1:
                                        sprintf(ei.sync_type, "Asynchronous");
                                        break;
                                    case 2:
                                        sprintf(ei.sync_type, "Adaptive");
                                        break;
                                    case 3:
                                        sprintf(ei.sync_type, "Synchronous");
                                        break;
                                    }
                                    switch ((epdesc->bmAttributes >> 4) & 3)
                                    {
                                    case 0:
                                        sprintf(ei.usage_type, "Data Endpoint");
                                        break;
                                    case 1:
                                        sprintf(ei.usage_type, "Feedback Endpoint");
                                        break;
                                    case 2:
                                        sprintf(ei.usage_type, "Implicit feedback Data endpoint");
                                        break;
                                    case 3:
                                        sprintf(ei.usage_type, "Reserved");
                                        break;
                                    }
                                    break;
                                case 2:
                                    sprintf(ei.transfer_type, "Bulk");
                                    sprintf(ei.sync_type, "Reserved");
                                    sprintf(ei.usage_type, "Reserved");
                                    break;
                                case 3:
                                    sprintf(ei.transfer_type, "Interrupt");
                                    sprintf(ei.sync_type, "Reserved");
                                    switch ((epdesc->bmAttributes >> 4) & 3)
                                    {
                                    case 0:
                                        sprintf(ei.usage_type, "Periodic");
                                        break;
                                    case 1:
                                        sprintf(ei.usage_type, "Notification");
                                        break;
                                    default:
                                        sprintf(ei.usage_type, "Reserved");
                                        break;
                                    }
                                    break;
                                }

                                info->ep.push_back(ei);
                            }

                            return true;
                        }
                        else
                        {
                            acount++;
                            continue;
                        }
                    }
                    else
                    {
                        acount++;
                        continue;
                    }
                }
                else
                {
                    acount++;
                    continue;
                }
            }

            if (acount == interface->num_altsetting)
            {
                icount++;
                continue;
            }
        }

        if (icount == (int)config->bNumInterfaces)
        {
            rcount++;
            libusb_free_config_descriptor(config);
            continue;
        }

        libusb_free_config_descriptor(config);
    }

    return false;
}

string BarcodeReader::GetDeviceInfo(libusb_device *dev)
{
    stringstream devinfo;
    devinfo.str("");
    libusb_device_descriptor desc;
    string devclass = "";
    int r = libusb_get_device_descriptor(dev, &desc);

    devinfo << "Number of possible configurations: " << (int)desc.bNumConfigurations << "\n";
    devinfo << "Device Class: " << (int)desc.bDeviceClass << "\n";
    devinfo << "VendorID: " << hex << desc.idVendor << "\n";
    devinfo << "ProductID: " << hex << desc.idProduct << "\n";
    devinfo << "Device Class: " << GetDeviceClass(desc.bDeviceClass) << "\n";
    devinfo << "Device Sub-Class: " << (int)desc.bDeviceSubClass << "\n";
    
    libusb_config_descriptor *config;
    r = libusb_get_config_descriptor(dev, 0, &config);

    if (r < 0)
    {
        devinfo << libusb_strerror(libusb_error(r));
        devinfo << endl;
        //libusb_free_config_descriptor(config);
        return devinfo.str();
    }

    devinfo << "Number of Interfaces: " << (int)config->bNumInterfaces << "\n";
    
    const libusb_interface *inter;
    const libusb_interface_descriptor *interdesc;
    const libusb_endpoint_descriptor *epdesc;

    for (int i = 0; i<(int)config->bNumInterfaces; i++)
    {
        inter = &config->interface[i];
        devinfo << "\tNumber of alternate settings: " << inter->num_altsetting << "\n";
        for (int j = 0; j<inter->num_altsetting; j++)
        {
            interdesc = &inter->altsetting[j];
            devinfo << "\t\tInterface Number: " << (int)interdesc->bInterfaceNumber << "\n";
            devinfo << "\t\tInterface Class Number: " << int(interdesc->bInterfaceClass) << "\n";
            devinfo << "\t\tInterface Class: " << GetDeviceClass(interdesc->bInterfaceClass) << "\n";
            devinfo << "\t\tInterface Sub-Class: " << int(interdesc->bInterfaceSubClass) << "\n";
            devinfo << "\t\tInterface Protocol: " << int(interdesc->bInterfaceProtocol) << "\n";
            devinfo << "\t\tNumber of endpoints: " << (int)interdesc->bNumEndpoints << "\n";
            for (int k = 0; k<(int)interdesc->bNumEndpoints; k++)
            {
                epdesc = &interdesc->endpoint[k];
                devinfo << "\t\t\tDescriptor Type: " << (int)epdesc->bDescriptorType << "\n";
                devinfo << "\t\t\tEP Address: " << (int)epdesc->bEndpointAddress << "\n";
            }
        }
    }
    
    devinfo << endl;
    libusb_free_config_descriptor(config);

    return devinfo.str();
}

bool BarcodeReader::is_needed(HANDLE hdl)
{
    if (keyboards.size() == 0)
    {
        qCritical() << "Barcode reader not found.";
        return false;
    }

    return (keyboards[0]->devhandle == hdl);
}

void BarcodeReader::read_callback(struct libusb_transfer *transfer)
{
    struct input_report *reports = (struct input_report *)transfer->user_data;
    int res;

    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        struct input_report *rpt = new struct input_report();
        rpt->data = new uint8_t[transfer->actual_length];
        memcpy(rpt->data, transfer->buffer, transfer->actual_length);
        rpt->len = transfer->actual_length;
        rpt->next = nullptr;

        /* Attach the new report object to the end of the list. */
        if (reports == NULL)
        {
            /* The list is empty. Put it at the root. */
            reports = rpt;
            //pthread_cond_signal(&dev->condition);
        }
        else
        {
            /* Find the end of the list and attach. */
            struct input_report *cur = reports;
            //int num_queued = 0;
            while (cur->next != NULL)
            {
                cur = cur->next;
                //num_queued++;
            }
            cur->next = rpt;

            /* Pop one off if we've reached 30 in the queue. This
            way we don't grow forever if the user never reads
            anything from the device. */
            /*if (num_queued > 30)
            {
                return_data(dev, NULL, 0);
            }*/
        }
        //pthread_mutex_unlock(&dev->mutex);
    }
    else if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
        /*dev->shutdown_thread = 1;
        dev->cancelled = 1;*/
        return;
    }
    else if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
        /*dev->shutdown_thread = 1;
        dev->cancelled = 1;*/
        return;
    }
    else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
        //LOG("Timeout (normal)\n");
    }
    else {
        //LOG("Unknown transfer code: %d\n", transfer->status);
    }

    /* Re-submit the transfer object. */
    res = libusb_submit_transfer(transfer);
    /*if (res != 0)
    {
        dev->shutdown_thread = 1;
        dev->cancelled = 1;
    }*/
}

//void BarcodeReader::run()
//{
//    bool driver_attached = false;
//    int r = libusb_open(keyboards[0]->dev, &hdev);
//    int cancelled = 0;
//    char rdata[1000];
//    struct input_report *rpt;
//    struct timeval tv = { 1, 0 };
//
//    if (r < 0)
//        return;
//    
//    if (libusb_kernel_driver_active(hdev, keyboards[0]->interface_num) == 1)
//    {
//        driver_attached = true;
//        r = libusb_detach_kernel_driver(hdev, keyboards[0]->interface_num);
//
//        if (r < 0)
//        {
//            libusb_close(hdev);
//            driver_attached = false;
//            return;
//        }
//    }
//    else
//        driver_attached = false;
//
//    r = libusb_claim_interface(hdev, keyboards[0]->interface_num);
//
//    if (r < 0)
//    {
//        if (driver_attached)
//            libusb_attach_kernel_driver(hdev, keyboards[0]->interface_num);
//        libusb_close(hdev);
//        driver_attached = false;
//        return;
//    }
//
//    int len = keyboards[0]->ep[0].maxpacketsize;
//    uint8_t *buf = new uint8_t[len];
//    libusb_transfer *transfer = libusb_alloc_transfer(0);
//
//    libusb_fill_interrupt_transfer(transfer, hdev, keyboards[0]->ep[0].epnum, buf, len,
//        read_callback, reports, 5000);
//
//    libusb_submit_transfer(transfer);
//
//    while (true)
//    {
//        r = libusb_handle_events_timeout_completed(context, &tv, NULL);
//        if (r < 0)
//        {
//            /* Break out of this loop only on fatal error.*/
//            if (r != LIBUSB_ERROR_BUSY &&
//                r != LIBUSB_ERROR_TIMEOUT &&
//                r != LIBUSB_ERROR_OVERFLOW &&
//                r != LIBUSB_ERROR_INTERRUPTED)
//                break;
//        }
//        else
//        {
//            if (isInterruptionRequested())
//                break;
//
//            if (reports != nullptr)
//            {
//                while (reports != nullptr)
//                {
//                    rpt = reports;
//
//                    qDebug() << "Report hex values :";
//                    for (size_t t = 0; t < rpt->len; t++)
//                        qDebug("%x, ", rpt->data[t]);
//                    qDebug() << "Report character values:";
//                    for (size_t t = 0; t < rpt->len; t++)
//                        qDebug("%c, ", (char)rpt->data[t]);
//
//                    reports = rpt->next;
//                    delete rpt->data;
//                    delete rpt;
//                }
//            }
//        }
//    }
//
//    libusb_cancel_transfer(transfer);
//
//    while (!cancelled)
//        libusb_handle_events_completed(context, &cancelled);
//
//    r = libusb_release_interface(hdev, keyboards[0]->interface_num);
//
//    if (r < 0)
//    {
//        if (driver_attached)
//            libusb_attach_kernel_driver(hdev, keyboards[0]->interface_num);
//        libusb_close(hdev);
//        driver_attached = false;
//        return;
//    }
//
//    if (driver_attached)
//    {
//        r = libusb_attach_kernel_driver(hdev, keyboards[0]->interface_num);
//
//        if (r < 0)
//        {
//            libusb_close(hdev);
//            driver_attached = false;
//            return;
//        }
//    }
//
//    libusb_close(hdev);
//}

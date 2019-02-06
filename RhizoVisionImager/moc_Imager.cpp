/*  Copyright (C) 2018-2019 Noble Research Institute, LLC

File: moc_Imager.cpp

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

/****************************************************************************
** Meta object code from reading C++ file 'Imager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Imager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Imager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ImageLabel_t {
    QByteArrayData data[1];
    char stringdata0[11];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ImageLabel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ImageLabel_t qt_meta_stringdata_ImageLabel = {
    {
QT_MOC_LITERAL(0, 0, 10) // "ImageLabel"

    },
    "ImageLabel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ImageLabel[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void ImageLabel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject ImageLabel::staticMetaObject = {
    { &QLabel::staticMetaObject, qt_meta_stringdata_ImageLabel.data,
      qt_meta_data_ImageLabel,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *ImageLabel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImageLabel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ImageLabel.stringdata0))
        return static_cast<void*>(const_cast< ImageLabel*>(this));
    return QLabel::qt_metacast(_clname);
}

int ImageLabel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLabel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_ScrollArea_t {
    QByteArrayData data[3];
    char stringdata0[25];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ScrollArea_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ScrollArea_t qt_meta_stringdata_ScrollArea = {
    {
QT_MOC_LITERAL(0, 0, 10), // "ScrollArea"
QT_MOC_LITERAL(1, 11, 12), // "scaleChanged"
QT_MOC_LITERAL(2, 24, 0) // ""

    },
    "ScrollArea\0scaleChanged\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScrollArea[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   19,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    2,

       0        // eod
};

void ScrollArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        ScrollArea *_t = static_cast<ScrollArea *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->scaleChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (ScrollArea::*_t)(int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&ScrollArea::scaleChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject ScrollArea::staticMetaObject = {
    { &QScrollArea::staticMetaObject, qt_meta_stringdata_ScrollArea.data,
      qt_meta_data_ScrollArea,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *ScrollArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScrollArea::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScrollArea.stringdata0))
        return static_cast<void*>(const_cast< ScrollArea*>(this));
    return QScrollArea::qt_metacast(_clname);
}

int ScrollArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QScrollArea::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void ScrollArea::scaleChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_Imager_t {
    QByteArrayData data[19];
    char stringdata0[210];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Imager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Imager_t qt_meta_stringdata_Imager = {
    {
QT_MOC_LITERAL(0, 0, 6), // "Imager"
QT_MOC_LITERAL(1, 7, 10), // "updateText"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 12), // "updateSlider"
QT_MOC_LITERAL(4, 32, 7), // "browser"
QT_MOC_LITERAL(5, 40, 11), // "updateimage"
QT_MOC_LITERAL(6, 52, 5), // "scale"
QT_MOC_LITERAL(7, 58, 17), // "livecamerachanged"
QT_MOC_LITERAL(8, 76, 14), // "profilechanged"
QT_MOC_LITERAL(9, 91, 10), // "singleshot"
QT_MOC_LITERAL(10, 102, 14), // "singleshotfile"
QT_MOC_LITERAL(11, 117, 6), // "string"
QT_MOC_LITERAL(12, 124, 8), // "filename"
QT_MOC_LITERAL(13, 133, 14), // "refreshdevices"
QT_MOC_LITERAL(14, 148, 12), // "LoadSettings"
QT_MOC_LITERAL(15, 161, 12), // "SaveSettings"
QT_MOC_LITERAL(16, 174, 11), // "LoadProfile"
QT_MOC_LITERAL(17, 186, 11), // "profilename"
QT_MOC_LITERAL(18, 198, 11) // "SaveProfile"

    },
    "Imager\0updateText\0\0updateSlider\0browser\0"
    "updateimage\0scale\0livecamerachanged\0"
    "profilechanged\0singleshot\0singleshotfile\0"
    "string\0filename\0refreshdevices\0"
    "LoadSettings\0SaveSettings\0LoadProfile\0"
    "profilename\0SaveProfile"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Imager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x08 /* Private */,
       3,    0,   80,    2, 0x08 /* Private */,
       4,    0,   81,    2, 0x08 /* Private */,
       5,    1,   82,    2, 0x08 /* Private */,
       7,    0,   85,    2, 0x08 /* Private */,
       8,    0,   86,    2, 0x08 /* Private */,
       9,    0,   87,    2, 0x08 /* Private */,
      10,    1,   88,    2, 0x08 /* Private */,
      13,    0,   91,    2, 0x08 /* Private */,
      14,    0,   92,    2, 0x08 /* Private */,
      15,    0,   93,    2, 0x08 /* Private */,
      16,    1,   94,    2, 0x08 /* Private */,
      18,    1,   97,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, QMetaType::QString,   17,

       0        // eod
};

void Imager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Imager *_t = static_cast<Imager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->updateText(); break;
        case 1: _t->updateSlider(); break;
        case 2: _t->browser(); break;
        case 3: _t->updateimage((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->livecamerachanged(); break;
        case 5: _t->profilechanged(); break;
        case 6: _t->singleshot(); break;
        case 7: _t->singleshotfile((*reinterpret_cast< string(*)>(_a[1]))); break;
        case 8: _t->refreshdevices(); break;
        case 9: _t->LoadSettings(); break;
        case 10: _t->SaveSettings(); break;
        case 11: _t->LoadProfile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 12: _t->SaveProfile((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject Imager::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_Imager.data,
      qt_meta_data_Imager,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *Imager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Imager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Imager.stringdata0))
        return static_cast<void*>(const_cast< Imager*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int Imager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

/****************************************************************************
** Meta object code from reading C++ file 'verify.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../SkyEye/verify.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'verify.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Verify[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,    8,    7,    7, 0x05,

 // slots: signature, parameters, type, tag, flags
      33,    7,    7,    7, 0x0a,
      44,    7,    7,    7, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Verify[] = {
    "Verify\0\0fdb\0autologinRoot(ForDB)\0"
    "VerifyIt()\0accept()\0"
};

void Verify::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Verify *_t = static_cast<Verify *>(_o);
        switch (_id) {
        case 0: _t->autologinRoot((*reinterpret_cast< ForDB(*)>(_a[1]))); break;
        case 1: _t->VerifyIt(); break;
        case 2: _t->accept(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Verify::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Verify::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_Verify,
      qt_meta_data_Verify, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Verify::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Verify::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Verify::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Verify))
        return static_cast<void*>(const_cast< Verify*>(this));
    return QDialog::qt_metacast(_clname);
}

int Verify::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void Verify::autologinRoot(ForDB _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE

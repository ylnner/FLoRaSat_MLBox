//
// Generated file, do not edit! Created by opp_msgtool 6.3 from Global/Statistics/TransmissionStatistics/CstTransmissionStatisticsTag.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "CstTransmissionStatisticsTag_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace statistics {

Register_Class(CstTransmissionStatisticsTag)

CstTransmissionStatisticsTag::CstTransmissionStatisticsTag() : ::inet::TagBase()
{
}

CstTransmissionStatisticsTag::CstTransmissionStatisticsTag(const CstTransmissionStatisticsTag& other) : ::inet::TagBase(other)
{
    copy(other);
}

CstTransmissionStatisticsTag::~CstTransmissionStatisticsTag()
{
}

CstTransmissionStatisticsTag& CstTransmissionStatisticsTag::operator=(const CstTransmissionStatisticsTag& other)
{
    if (this == &other) return *this;
    ::inet::TagBase::operator=(other);
    copy(other);
    return *this;
}

void CstTransmissionStatisticsTag::copy(const CstTransmissionStatisticsTag& other)
{
    this->latDev = other.latDev;
    this->longDev = other.longDev;
    this->satId = other.satId;
    this->elevSat = other.elevSat;
    this->doppler = other.doppler;
    this->loraTP = other.loraTP;
    this->loraCF = other.loraCF;
    this->loraSF = other.loraSF;
    this->loraBW = other.loraBW;
    this->loraCR = other.loraCR;
    this->rcvOk = other.rcvOk;
}

void CstTransmissionStatisticsTag::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::TagBase::parsimPack(b);
    doParsimPacking(b,this->latDev);
    doParsimPacking(b,this->longDev);
    doParsimPacking(b,this->satId);
    doParsimPacking(b,this->elevSat);
    doParsimPacking(b,this->doppler);
    doParsimPacking(b,this->loraTP);
    doParsimPacking(b,this->loraCF);
    doParsimPacking(b,this->loraSF);
    doParsimPacking(b,this->loraBW);
    doParsimPacking(b,this->loraCR);
    doParsimPacking(b,this->rcvOk);
}

void CstTransmissionStatisticsTag::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::TagBase::parsimUnpack(b);
    doParsimUnpacking(b,this->latDev);
    doParsimUnpacking(b,this->longDev);
    doParsimUnpacking(b,this->satId);
    doParsimUnpacking(b,this->elevSat);
    doParsimUnpacking(b,this->doppler);
    doParsimUnpacking(b,this->loraTP);
    doParsimUnpacking(b,this->loraCF);
    doParsimUnpacking(b,this->loraSF);
    doParsimUnpacking(b,this->loraBW);
    doParsimUnpacking(b,this->loraCR);
    doParsimUnpacking(b,this->rcvOk);
}

double CstTransmissionStatisticsTag::getLatDev() const
{
    return this->latDev;
}

void CstTransmissionStatisticsTag::setLatDev(double latDev)
{
    this->latDev = latDev;
}

double CstTransmissionStatisticsTag::getLongDev() const
{
    return this->longDev;
}

void CstTransmissionStatisticsTag::setLongDev(double longDev)
{
    this->longDev = longDev;
}

int CstTransmissionStatisticsTag::getSatId() const
{
    return this->satId;
}

void CstTransmissionStatisticsTag::setSatId(int satId)
{
    this->satId = satId;
}

double CstTransmissionStatisticsTag::getElevSat() const
{
    return this->elevSat;
}

void CstTransmissionStatisticsTag::setElevSat(double elevSat)
{
    this->elevSat = elevSat;
}

double CstTransmissionStatisticsTag::getDoppler() const
{
    return this->doppler;
}

void CstTransmissionStatisticsTag::setDoppler(double doppler)
{
    this->doppler = doppler;
}

double CstTransmissionStatisticsTag::getLoraTP() const
{
    return this->loraTP;
}

void CstTransmissionStatisticsTag::setLoraTP(double loraTP)
{
    this->loraTP = loraTP;
}

double CstTransmissionStatisticsTag::getLoraCF() const
{
    return this->loraCF;
}

void CstTransmissionStatisticsTag::setLoraCF(double loraCF)
{
    this->loraCF = loraCF;
}

int CstTransmissionStatisticsTag::getLoraSF() const
{
    return this->loraSF;
}

void CstTransmissionStatisticsTag::setLoraSF(int loraSF)
{
    this->loraSF = loraSF;
}

double CstTransmissionStatisticsTag::getLoraBW() const
{
    return this->loraBW;
}

void CstTransmissionStatisticsTag::setLoraBW(double loraBW)
{
    this->loraBW = loraBW;
}

int CstTransmissionStatisticsTag::getLoraCR() const
{
    return this->loraCR;
}

void CstTransmissionStatisticsTag::setLoraCR(int loraCR)
{
    this->loraCR = loraCR;
}

int CstTransmissionStatisticsTag::getRcvOk() const
{
    return this->rcvOk;
}

void CstTransmissionStatisticsTag::setRcvOk(int rcvOk)
{
    this->rcvOk = rcvOk;
}

class CstTransmissionStatisticsTagDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_latDev,
        FIELD_longDev,
        FIELD_satId,
        FIELD_elevSat,
        FIELD_doppler,
        FIELD_loraTP,
        FIELD_loraCF,
        FIELD_loraSF,
        FIELD_loraBW,
        FIELD_loraCR,
        FIELD_rcvOk,
    };
  public:
    CstTransmissionStatisticsTagDescriptor();
    virtual ~CstTransmissionStatisticsTagDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(CstTransmissionStatisticsTagDescriptor)

CstTransmissionStatisticsTagDescriptor::CstTransmissionStatisticsTagDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(statistics::CstTransmissionStatisticsTag)), "inet::TagBase")
{
    propertyNames = nullptr;
}

CstTransmissionStatisticsTagDescriptor::~CstTransmissionStatisticsTagDescriptor()
{
    delete[] propertyNames;
}

bool CstTransmissionStatisticsTagDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<CstTransmissionStatisticsTag *>(obj)!=nullptr;
}

const char **CstTransmissionStatisticsTagDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *CstTransmissionStatisticsTagDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int CstTransmissionStatisticsTagDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 11+base->getFieldCount() : 11;
}

unsigned int CstTransmissionStatisticsTagDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_latDev
        FD_ISEDITABLE,    // FIELD_longDev
        FD_ISEDITABLE,    // FIELD_satId
        FD_ISEDITABLE,    // FIELD_elevSat
        FD_ISEDITABLE,    // FIELD_doppler
        FD_ISEDITABLE,    // FIELD_loraTP
        FD_ISEDITABLE,    // FIELD_loraCF
        FD_ISEDITABLE,    // FIELD_loraSF
        FD_ISEDITABLE,    // FIELD_loraBW
        FD_ISEDITABLE,    // FIELD_loraCR
        FD_ISEDITABLE,    // FIELD_rcvOk
    };
    return (field >= 0 && field < 11) ? fieldTypeFlags[field] : 0;
}

const char *CstTransmissionStatisticsTagDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "latDev",
        "longDev",
        "satId",
        "elevSat",
        "doppler",
        "loraTP",
        "loraCF",
        "loraSF",
        "loraBW",
        "loraCR",
        "rcvOk",
    };
    return (field >= 0 && field < 11) ? fieldNames[field] : nullptr;
}

int CstTransmissionStatisticsTagDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "latDev") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "longDev") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "satId") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "elevSat") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "doppler") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "loraTP") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "loraCF") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "loraSF") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "loraBW") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "loraCR") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "rcvOk") == 0) return baseIndex + 10;
    return base ? base->findField(fieldName) : -1;
}

const char *CstTransmissionStatisticsTagDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "double",    // FIELD_latDev
        "double",    // FIELD_longDev
        "int",    // FIELD_satId
        "double",    // FIELD_elevSat
        "double",    // FIELD_doppler
        "double",    // FIELD_loraTP
        "double",    // FIELD_loraCF
        "int",    // FIELD_loraSF
        "double",    // FIELD_loraBW
        "int",    // FIELD_loraCR
        "int",    // FIELD_rcvOk
    };
    return (field >= 0 && field < 11) ? fieldTypeStrings[field] : nullptr;
}

const char **CstTransmissionStatisticsTagDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *CstTransmissionStatisticsTagDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int CstTransmissionStatisticsTagDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void CstTransmissionStatisticsTagDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'CstTransmissionStatisticsTag'", field);
    }
}

const char *CstTransmissionStatisticsTagDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string CstTransmissionStatisticsTagDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        case FIELD_latDev: return double2string(pp->getLatDev());
        case FIELD_longDev: return double2string(pp->getLongDev());
        case FIELD_satId: return long2string(pp->getSatId());
        case FIELD_elevSat: return double2string(pp->getElevSat());
        case FIELD_doppler: return double2string(pp->getDoppler());
        case FIELD_loraTP: return double2string(pp->getLoraTP());
        case FIELD_loraCF: return double2string(pp->getLoraCF());
        case FIELD_loraSF: return long2string(pp->getLoraSF());
        case FIELD_loraBW: return double2string(pp->getLoraBW());
        case FIELD_loraCR: return long2string(pp->getLoraCR());
        case FIELD_rcvOk: return long2string(pp->getRcvOk());
        default: return "";
    }
}

void CstTransmissionStatisticsTagDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        case FIELD_latDev: pp->setLatDev(string2double(value)); break;
        case FIELD_longDev: pp->setLongDev(string2double(value)); break;
        case FIELD_satId: pp->setSatId(string2long(value)); break;
        case FIELD_elevSat: pp->setElevSat(string2double(value)); break;
        case FIELD_doppler: pp->setDoppler(string2double(value)); break;
        case FIELD_loraTP: pp->setLoraTP(string2double(value)); break;
        case FIELD_loraCF: pp->setLoraCF(string2double(value)); break;
        case FIELD_loraSF: pp->setLoraSF(string2long(value)); break;
        case FIELD_loraBW: pp->setLoraBW(string2double(value)); break;
        case FIELD_loraCR: pp->setLoraCR(string2long(value)); break;
        case FIELD_rcvOk: pp->setRcvOk(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'CstTransmissionStatisticsTag'", field);
    }
}

omnetpp::cValue CstTransmissionStatisticsTagDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        case FIELD_latDev: return pp->getLatDev();
        case FIELD_longDev: return pp->getLongDev();
        case FIELD_satId: return pp->getSatId();
        case FIELD_elevSat: return pp->getElevSat();
        case FIELD_doppler: return pp->getDoppler();
        case FIELD_loraTP: return pp->getLoraTP();
        case FIELD_loraCF: return pp->getLoraCF();
        case FIELD_loraSF: return pp->getLoraSF();
        case FIELD_loraBW: return pp->getLoraBW();
        case FIELD_loraCR: return pp->getLoraCR();
        case FIELD_rcvOk: return pp->getRcvOk();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'CstTransmissionStatisticsTag' as cValue -- field index out of range?", field);
    }
}

void CstTransmissionStatisticsTagDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        case FIELD_latDev: pp->setLatDev(value.doubleValue()); break;
        case FIELD_longDev: pp->setLongDev(value.doubleValue()); break;
        case FIELD_satId: pp->setSatId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_elevSat: pp->setElevSat(value.doubleValue()); break;
        case FIELD_doppler: pp->setDoppler(value.doubleValue()); break;
        case FIELD_loraTP: pp->setLoraTP(value.doubleValue()); break;
        case FIELD_loraCF: pp->setLoraCF(value.doubleValue()); break;
        case FIELD_loraSF: pp->setLoraSF(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_loraBW: pp->setLoraBW(value.doubleValue()); break;
        case FIELD_loraCR: pp->setLoraCR(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_rcvOk: pp->setRcvOk(omnetpp::checked_int_cast<int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'CstTransmissionStatisticsTag'", field);
    }
}

const char *CstTransmissionStatisticsTagDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr CstTransmissionStatisticsTagDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void CstTransmissionStatisticsTagDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    CstTransmissionStatisticsTag *pp = omnetpp::fromAnyPtr<CstTransmissionStatisticsTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'CstTransmissionStatisticsTag'", field);
    }
}

}  // namespace statistics

namespace omnetpp {

}  // namespace omnetpp


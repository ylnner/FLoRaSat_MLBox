//
// Generated file, do not edit! Created by opp_msgtool 6.3 from Global/Messages/_30_Network/Net_CstRoutingTag.msg.
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
#include "Net_CstRoutingTag_m.h"

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

namespace routing {

Register_Enum(routing::CstPacketType, (routing::CstPacketType::NORMAL, routing::CstPacketType::CONTROL));

Register_Enum(routing::HopType, (routing::HopType::GS, routing::HopType::SAT, routing::HopType::DEV));

Hop::Hop()
{
}

void __doPacking(omnetpp::cCommBuffer *b, const Hop& a)
{
    doParsimPacking(b,a.type);
    doParsimPacking(b,a.id);
    doParsimPacking(b,a.lat);
    doParsimPacking(b,a.lon);
    doParsimPacking(b,a.alt);
}

void __doUnpacking(omnetpp::cCommBuffer *b, Hop& a)
{
    doParsimUnpacking(b,a.type);
    doParsimUnpacking(b,a.id);
    doParsimUnpacking(b,a.lat);
    doParsimUnpacking(b,a.lon);
    doParsimUnpacking(b,a.alt);
}

class HopDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_type,
        FIELD_id,
        FIELD_lat,
        FIELD_lon,
        FIELD_alt,
    };
  public:
    HopDescriptor();
    virtual ~HopDescriptor();

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

Register_ClassDescriptor(HopDescriptor)

HopDescriptor::HopDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(routing::Hop)), "")
{
    propertyNames = nullptr;
}

HopDescriptor::~HopDescriptor()
{
    delete[] propertyNames;
}

bool HopDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<Hop *>(obj)!=nullptr;
}

const char **HopDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *HopDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int HopDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 5+base->getFieldCount() : 5;
}

unsigned int HopDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_type
        FD_ISEDITABLE,    // FIELD_id
        FD_ISEDITABLE,    // FIELD_lat
        FD_ISEDITABLE,    // FIELD_lon
        FD_ISEDITABLE,    // FIELD_alt
    };
    return (field >= 0 && field < 5) ? fieldTypeFlags[field] : 0;
}

const char *HopDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "type",
        "id",
        "lat",
        "lon",
        "alt",
    };
    return (field >= 0 && field < 5) ? fieldNames[field] : nullptr;
}

int HopDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "type") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "id") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "lat") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "lon") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "alt") == 0) return baseIndex + 4;
    return base ? base->findField(fieldName) : -1;
}

const char *HopDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "routing::HopType",    // FIELD_type
        "int",    // FIELD_id
        "double",    // FIELD_lat
        "double",    // FIELD_lon
        "uint16_t",    // FIELD_alt
    };
    return (field >= 0 && field < 5) ? fieldTypeStrings[field] : nullptr;
}

const char **HopDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_type: {
            static const char *names[] = { "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *HopDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_type:
            if (!strcmp(propertyName, "enum")) return "routing::HopType";
            return nullptr;
        default: return nullptr;
    }
}

int HopDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void HopDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'Hop'", field);
    }
}

const char *HopDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string HopDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        case FIELD_type: return enum2string(static_cast<int>(pp->type), "routing::HopType");
        case FIELD_id: return long2string(pp->id);
        case FIELD_lat: return double2string(pp->lat);
        case FIELD_lon: return double2string(pp->lon);
        case FIELD_alt: return ulong2string(pp->alt);
        default: return "";
    }
}

void HopDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        case FIELD_id: pp->id = string2long(value); break;
        case FIELD_lat: pp->lat = string2double(value); break;
        case FIELD_lon: pp->lon = string2double(value); break;
        case FIELD_alt: pp->alt = string2ulong(value); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Hop'", field);
    }
}

omnetpp::cValue HopDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        case FIELD_type: return static_cast<int>(pp->type);
        case FIELD_id: return pp->id;
        case FIELD_lat: return pp->lat;
        case FIELD_lon: return pp->lon;
        case FIELD_alt: return (omnetpp::intval_t)(pp->alt);
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'Hop' as cValue -- field index out of range?", field);
    }
}

void HopDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        case FIELD_id: pp->id = omnetpp::checked_int_cast<int>(value.intValue()); break;
        case FIELD_lat: pp->lat = value.doubleValue(); break;
        case FIELD_lon: pp->lon = value.doubleValue(); break;
        case FIELD_alt: pp->alt = omnetpp::checked_int_cast<uint16_t>(value.intValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Hop'", field);
    }
}

const char *HopDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr HopDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void HopDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    Hop *pp = omnetpp::fromAnyPtr<Hop>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Hop'", field);
    }
}

Register_Class(CstRoutingTag)

CstRoutingTag::CstRoutingTag() : ::inet::TagBase()
{
}

CstRoutingTag::CstRoutingTag(const CstRoutingTag& other) : ::inet::TagBase(other)
{
    copy(other);
}

CstRoutingTag::~CstRoutingTag()
{
    delete [] this->route;
}

CstRoutingTag& CstRoutingTag::operator=(const CstRoutingTag& other)
{
    if (this == &other) return *this;
    ::inet::TagBase::operator=(other);
    copy(other);
    return *this;
}

void CstRoutingTag::copy(const CstRoutingTag& other)
{
    this->type = other.type;
    this->srcGs = other.srcGs;
    this->dstGs = other.dstGs;
    this->srcId = other.srcId;
    this->dstId = other.dstId;
    this->srcSat = other.srcSat;
    this->dstSat = other.dstSat;
    this->hops = other.hops;
    this->maxHops = other.maxHops;
    this->isLoraNode = other.isLoraNode;
    this->srcGsOrDev = other.srcGsOrDev;
    this->dstGsOrDev = other.dstGsOrDev;
    this->roundTrip = other.roundTrip;
    delete [] this->route;
    this->route = (other.route_arraysize==0) ? nullptr : new Hop[other.route_arraysize];
    route_arraysize = other.route_arraysize;
    for (size_t i = 0; i < route_arraysize; i++) {
        this->route[i] = other.route[i];
    }
}

void CstRoutingTag::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::TagBase::parsimPack(b);
    doParsimPacking(b,this->type);
    doParsimPacking(b,this->srcGs);
    doParsimPacking(b,this->dstGs);
    doParsimPacking(b,this->srcId);
    doParsimPacking(b,this->dstId);
    doParsimPacking(b,this->srcSat);
    doParsimPacking(b,this->dstSat);
    doParsimPacking(b,this->hops);
    doParsimPacking(b,this->maxHops);
    doParsimPacking(b,this->isLoraNode);
    doParsimPacking(b,this->srcGsOrDev);
    doParsimPacking(b,this->dstGsOrDev);
    doParsimPacking(b,this->roundTrip);
    b->pack(route_arraysize);
    doParsimArrayPacking(b,this->route,route_arraysize);
}

void CstRoutingTag::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::TagBase::parsimUnpack(b);
    doParsimUnpacking(b,this->type);
    doParsimUnpacking(b,this->srcGs);
    doParsimUnpacking(b,this->dstGs);
    doParsimUnpacking(b,this->srcId);
    doParsimUnpacking(b,this->dstId);
    doParsimUnpacking(b,this->srcSat);
    doParsimUnpacking(b,this->dstSat);
    doParsimUnpacking(b,this->hops);
    doParsimUnpacking(b,this->maxHops);
    doParsimUnpacking(b,this->isLoraNode);
    doParsimUnpacking(b,this->srcGsOrDev);
    doParsimUnpacking(b,this->dstGsOrDev);
    doParsimUnpacking(b,this->roundTrip);
    delete [] this->route;
    b->unpack(route_arraysize);
    if (route_arraysize == 0) {
        this->route = nullptr;
    } else {
        this->route = new Hop[route_arraysize];
        doParsimArrayUnpacking(b,this->route,route_arraysize);
    }
}

CstPacketType CstRoutingTag::getType() const
{
    return this->type;
}

void CstRoutingTag::setType(CstPacketType type)
{
    this->type = type;
}

int CstRoutingTag::getSrcGs() const
{
    return this->srcGs;
}

void CstRoutingTag::setSrcGs(int srcGs)
{
    this->srcGs = srcGs;
}

int CstRoutingTag::getDstGs() const
{
    return this->dstGs;
}

void CstRoutingTag::setDstGs(int dstGs)
{
    this->dstGs = dstGs;
}

int CstRoutingTag::getSrcId() const
{
    return this->srcId;
}

void CstRoutingTag::setSrcId(int srcId)
{
    this->srcId = srcId;
}

int CstRoutingTag::getDstId() const
{
    return this->dstId;
}

void CstRoutingTag::setDstId(int dstId)
{
    this->dstId = dstId;
}

int CstRoutingTag::getSrcSat() const
{
    return this->srcSat;
}

void CstRoutingTag::setSrcSat(int srcSat)
{
    this->srcSat = srcSat;
}

int CstRoutingTag::getDstSat() const
{
    return this->dstSat;
}

void CstRoutingTag::setDstSat(int dstSat)
{
    this->dstSat = dstSat;
}

int CstRoutingTag::getHops() const
{
    return this->hops;
}

void CstRoutingTag::setHops(int hops)
{
    this->hops = hops;
}

int CstRoutingTag::getMaxHops() const
{
    return this->maxHops;
}

void CstRoutingTag::setMaxHops(int maxHops)
{
    this->maxHops = maxHops;
}

int CstRoutingTag::getIsLoraNode() const
{
    return this->isLoraNode;
}

void CstRoutingTag::setIsLoraNode(int isLoraNode)
{
    this->isLoraNode = isLoraNode;
}

int CstRoutingTag::getSrcGsOrDev() const
{
    return this->srcGsOrDev;
}

void CstRoutingTag::setSrcGsOrDev(int srcGsOrDev)
{
    this->srcGsOrDev = srcGsOrDev;
}

int CstRoutingTag::getDstGsOrDev() const
{
    return this->dstGsOrDev;
}

void CstRoutingTag::setDstGsOrDev(int dstGsOrDev)
{
    this->dstGsOrDev = dstGsOrDev;
}

bool CstRoutingTag::getRoundTrip() const
{
    return this->roundTrip;
}

void CstRoutingTag::setRoundTrip(bool roundTrip)
{
    this->roundTrip = roundTrip;
}

size_t CstRoutingTag::getRouteArraySize() const
{
    return route_arraysize;
}

const Hop& CstRoutingTag::getRoute(size_t k) const
{
    if (k >= route_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)route_arraysize, (unsigned long)k);
    return this->route[k];
}

void CstRoutingTag::setRouteArraySize(size_t newSize)
{
    Hop *route2 = (newSize==0) ? nullptr : new Hop[newSize];
    size_t minSize = route_arraysize < newSize ? route_arraysize : newSize;
    for (size_t i = 0; i < minSize; i++)
        route2[i] = this->route[i];
    delete [] this->route;
    this->route = route2;
    route_arraysize = newSize;
}

void CstRoutingTag::setRoute(size_t k, const Hop& route)
{
    if (k >= route_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)route_arraysize, (unsigned long)k);
    this->route[k] = route;
}

void CstRoutingTag::insertRoute(size_t k, const Hop& route)
{
    if (k > route_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)route_arraysize, (unsigned long)k);
    size_t newSize = route_arraysize + 1;
    Hop *route2 = new Hop[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        route2[i] = this->route[i];
    route2[k] = route;
    for (i = k + 1; i < newSize; i++)
        route2[i] = this->route[i-1];
    delete [] this->route;
    this->route = route2;
    route_arraysize = newSize;
}

void CstRoutingTag::appendRoute(const Hop& route)
{
    insertRoute(route_arraysize, route);
}

void CstRoutingTag::eraseRoute(size_t k)
{
    if (k >= route_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)route_arraysize, (unsigned long)k);
    size_t newSize = route_arraysize - 1;
    Hop *route2 = (newSize == 0) ? nullptr : new Hop[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        route2[i] = this->route[i];
    for (i = k; i < newSize; i++)
        route2[i] = this->route[i+1];
    delete [] this->route;
    this->route = route2;
    route_arraysize = newSize;
}

class CstRoutingTagDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_type,
        FIELD_srcGs,
        FIELD_dstGs,
        FIELD_srcId,
        FIELD_dstId,
        FIELD_srcSat,
        FIELD_dstSat,
        FIELD_hops,
        FIELD_maxHops,
        FIELD_isLoraNode,
        FIELD_srcGsOrDev,
        FIELD_dstGsOrDev,
        FIELD_roundTrip,
        FIELD_route,
    };
  public:
    CstRoutingTagDescriptor();
    virtual ~CstRoutingTagDescriptor();

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

Register_ClassDescriptor(CstRoutingTagDescriptor)

CstRoutingTagDescriptor::CstRoutingTagDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(routing::CstRoutingTag)), "inet::TagBase")
{
    propertyNames = nullptr;
}

CstRoutingTagDescriptor::~CstRoutingTagDescriptor()
{
    delete[] propertyNames;
}

bool CstRoutingTagDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<CstRoutingTag *>(obj)!=nullptr;
}

const char **CstRoutingTagDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *CstRoutingTagDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int CstRoutingTagDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 14+base->getFieldCount() : 14;
}

unsigned int CstRoutingTagDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_type
        FD_ISEDITABLE,    // FIELD_srcGs
        FD_ISEDITABLE,    // FIELD_dstGs
        FD_ISEDITABLE,    // FIELD_srcId
        FD_ISEDITABLE,    // FIELD_dstId
        FD_ISEDITABLE,    // FIELD_srcSat
        FD_ISEDITABLE,    // FIELD_dstSat
        FD_ISEDITABLE,    // FIELD_hops
        FD_ISEDITABLE,    // FIELD_maxHops
        FD_ISEDITABLE,    // FIELD_isLoraNode
        FD_ISEDITABLE,    // FIELD_srcGsOrDev
        FD_ISEDITABLE,    // FIELD_dstGsOrDev
        FD_ISEDITABLE,    // FIELD_roundTrip
        FD_ISARRAY | FD_ISCOMPOUND | FD_ISRESIZABLE,    // FIELD_route
    };
    return (field >= 0 && field < 14) ? fieldTypeFlags[field] : 0;
}

const char *CstRoutingTagDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "type",
        "srcGs",
        "dstGs",
        "srcId",
        "dstId",
        "srcSat",
        "dstSat",
        "hops",
        "maxHops",
        "isLoraNode",
        "srcGsOrDev",
        "dstGsOrDev",
        "roundTrip",
        "route",
    };
    return (field >= 0 && field < 14) ? fieldNames[field] : nullptr;
}

int CstRoutingTagDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "type") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "srcGs") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "dstGs") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "srcId") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "dstId") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "srcSat") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "dstSat") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "hops") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "maxHops") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "isLoraNode") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "srcGsOrDev") == 0) return baseIndex + 10;
    if (strcmp(fieldName, "dstGsOrDev") == 0) return baseIndex + 11;
    if (strcmp(fieldName, "roundTrip") == 0) return baseIndex + 12;
    if (strcmp(fieldName, "route") == 0) return baseIndex + 13;
    return base ? base->findField(fieldName) : -1;
}

const char *CstRoutingTagDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "routing::CstPacketType",    // FIELD_type
        "int",    // FIELD_srcGs
        "int",    // FIELD_dstGs
        "int",    // FIELD_srcId
        "int",    // FIELD_dstId
        "int",    // FIELD_srcSat
        "int",    // FIELD_dstSat
        "int",    // FIELD_hops
        "int",    // FIELD_maxHops
        "int",    // FIELD_isLoraNode
        "int",    // FIELD_srcGsOrDev
        "int",    // FIELD_dstGsOrDev
        "bool",    // FIELD_roundTrip
        "routing::Hop",    // FIELD_route
    };
    return (field >= 0 && field < 14) ? fieldTypeStrings[field] : nullptr;
}

const char **CstRoutingTagDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_type: {
            static const char *names[] = { "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *CstRoutingTagDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_type:
            if (!strcmp(propertyName, "enum")) return "routing::CstPacketType";
            return nullptr;
        default: return nullptr;
    }
}

int CstRoutingTagDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_route: return pp->getRouteArraySize();
        default: return 0;
    }
}

void CstRoutingTagDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_route: pp->setRouteArraySize(size); break;
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'CstRoutingTag'", field);
    }
}

const char *CstRoutingTagDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string CstRoutingTagDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_type: return enum2string(static_cast<int>(pp->getType()), "routing::CstPacketType");
        case FIELD_srcGs: return long2string(pp->getSrcGs());
        case FIELD_dstGs: return long2string(pp->getDstGs());
        case FIELD_srcId: return long2string(pp->getSrcId());
        case FIELD_dstId: return long2string(pp->getDstId());
        case FIELD_srcSat: return long2string(pp->getSrcSat());
        case FIELD_dstSat: return long2string(pp->getDstSat());
        case FIELD_hops: return long2string(pp->getHops());
        case FIELD_maxHops: return long2string(pp->getMaxHops());
        case FIELD_isLoraNode: return long2string(pp->getIsLoraNode());
        case FIELD_srcGsOrDev: return long2string(pp->getSrcGsOrDev());
        case FIELD_dstGsOrDev: return long2string(pp->getDstGsOrDev());
        case FIELD_roundTrip: return bool2string(pp->getRoundTrip());
        case FIELD_route: return "";
        default: return "";
    }
}

void CstRoutingTagDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_srcGs: pp->setSrcGs(string2long(value)); break;
        case FIELD_dstGs: pp->setDstGs(string2long(value)); break;
        case FIELD_srcId: pp->setSrcId(string2long(value)); break;
        case FIELD_dstId: pp->setDstId(string2long(value)); break;
        case FIELD_srcSat: pp->setSrcSat(string2long(value)); break;
        case FIELD_dstSat: pp->setDstSat(string2long(value)); break;
        case FIELD_hops: pp->setHops(string2long(value)); break;
        case FIELD_maxHops: pp->setMaxHops(string2long(value)); break;
        case FIELD_isLoraNode: pp->setIsLoraNode(string2long(value)); break;
        case FIELD_srcGsOrDev: pp->setSrcGsOrDev(string2long(value)); break;
        case FIELD_dstGsOrDev: pp->setDstGsOrDev(string2long(value)); break;
        case FIELD_roundTrip: pp->setRoundTrip(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'CstRoutingTag'", field);
    }
}

omnetpp::cValue CstRoutingTagDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_type: return static_cast<int>(pp->getType());
        case FIELD_srcGs: return pp->getSrcGs();
        case FIELD_dstGs: return pp->getDstGs();
        case FIELD_srcId: return pp->getSrcId();
        case FIELD_dstId: return pp->getDstId();
        case FIELD_srcSat: return pp->getSrcSat();
        case FIELD_dstSat: return pp->getDstSat();
        case FIELD_hops: return pp->getHops();
        case FIELD_maxHops: return pp->getMaxHops();
        case FIELD_isLoraNode: return pp->getIsLoraNode();
        case FIELD_srcGsOrDev: return pp->getSrcGsOrDev();
        case FIELD_dstGsOrDev: return pp->getDstGsOrDev();
        case FIELD_roundTrip: return pp->getRoundTrip();
        case FIELD_route: return omnetpp::toAnyPtr(&pp->getRoute(i)); break;
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'CstRoutingTag' as cValue -- field index out of range?", field);
    }
}

void CstRoutingTagDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_srcGs: pp->setSrcGs(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstGs: pp->setDstGs(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcId: pp->setSrcId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstId: pp->setDstId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcSat: pp->setSrcSat(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstSat: pp->setDstSat(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_hops: pp->setHops(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_maxHops: pp->setMaxHops(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_isLoraNode: pp->setIsLoraNode(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcGsOrDev: pp->setSrcGsOrDev(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstGsOrDev: pp->setDstGsOrDev(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_roundTrip: pp->setRoundTrip(value.boolValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'CstRoutingTag'", field);
    }
}

const char *CstRoutingTagDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_route: return omnetpp::opp_typename(typeid(Hop));
        default: return nullptr;
    };
}

omnetpp::any_ptr CstRoutingTagDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        case FIELD_route: return omnetpp::toAnyPtr(&pp->getRoute(i)); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void CstRoutingTagDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    CstRoutingTag *pp = omnetpp::fromAnyPtr<CstRoutingTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'CstRoutingTag'", field);
    }
}

}  // namespace routing

namespace omnetpp {

}  // namespace omnetpp


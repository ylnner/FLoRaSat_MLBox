//
// Generated file, do not edit! Created by opp_msgtool 6.3 from Global/Messages/_30_Network/DDRARoutingHeader.msg.
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
#include "DDRARoutingHeader_m.h"

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
namespace ddra {

Register_Enum(routing::ddra::PacketType, (routing::ddra::PacketType::NORMAL, routing::ddra::PacketType::CONGESTED, routing::ddra::PacketType::UNCONGESTED, routing::ddra::PacketType::FAILURE));

Register_Class(DDRARoutingHeader)

DDRARoutingHeader::DDRARoutingHeader() : ::inet::FieldsChunk()
{
}

DDRARoutingHeader::DDRARoutingHeader(const DDRARoutingHeader& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

DDRARoutingHeader::~DDRARoutingHeader()
{
}

DDRARoutingHeader& DDRARoutingHeader::operator=(const DDRARoutingHeader& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void DDRARoutingHeader::copy(const DDRARoutingHeader& other)
{
    this->type = other.type;
    this->srcSat = other.srcSat;
    this->dstSat = other.dstSat;
    this->srcGs = other.srcGs;
    this->dstGs = other.dstGs;
    this->failureTarget = other.failureTarget;
}

void DDRARoutingHeader::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->type);
    doParsimPacking(b,this->srcSat);
    doParsimPacking(b,this->dstSat);
    doParsimPacking(b,this->srcGs);
    doParsimPacking(b,this->dstGs);
    doParsimPacking(b,this->failureTarget);
}

void DDRARoutingHeader::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->type);
    doParsimUnpacking(b,this->srcSat);
    doParsimUnpacking(b,this->dstSat);
    doParsimUnpacking(b,this->srcGs);
    doParsimUnpacking(b,this->dstGs);
    doParsimUnpacking(b,this->failureTarget);
}

PacketType DDRARoutingHeader::getType() const
{
    return this->type;
}

void DDRARoutingHeader::setType(PacketType type)
{
    handleChange();
    this->type = type;
}

int DDRARoutingHeader::getSrcSat() const
{
    return this->srcSat;
}

void DDRARoutingHeader::setSrcSat(int srcSat)
{
    handleChange();
    this->srcSat = srcSat;
}

int DDRARoutingHeader::getDstSat() const
{
    return this->dstSat;
}

void DDRARoutingHeader::setDstSat(int dstSat)
{
    handleChange();
    this->dstSat = dstSat;
}

int DDRARoutingHeader::getSrcGs() const
{
    return this->srcGs;
}

void DDRARoutingHeader::setSrcGs(int srcGs)
{
    handleChange();
    this->srcGs = srcGs;
}

int DDRARoutingHeader::getDstGs() const
{
    return this->dstGs;
}

void DDRARoutingHeader::setDstGs(int dstGs)
{
    handleChange();
    this->dstGs = dstGs;
}

int DDRARoutingHeader::getFailureTarget() const
{
    return this->failureTarget;
}

void DDRARoutingHeader::setFailureTarget(int failureTarget)
{
    handleChange();
    this->failureTarget = failureTarget;
}

class DDRARoutingHeaderDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_type,
        FIELD_srcSat,
        FIELD_dstSat,
        FIELD_srcGs,
        FIELD_dstGs,
        FIELD_failureTarget,
    };
  public:
    DDRARoutingHeaderDescriptor();
    virtual ~DDRARoutingHeaderDescriptor();

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

Register_ClassDescriptor(DDRARoutingHeaderDescriptor)

DDRARoutingHeaderDescriptor::DDRARoutingHeaderDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(routing::ddra::DDRARoutingHeader)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

DDRARoutingHeaderDescriptor::~DDRARoutingHeaderDescriptor()
{
    delete[] propertyNames;
}

bool DDRARoutingHeaderDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<DDRARoutingHeader *>(obj)!=nullptr;
}

const char **DDRARoutingHeaderDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *DDRARoutingHeaderDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int DDRARoutingHeaderDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 6+base->getFieldCount() : 6;
}

unsigned int DDRARoutingHeaderDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_type
        FD_ISEDITABLE,    // FIELD_srcSat
        FD_ISEDITABLE,    // FIELD_dstSat
        FD_ISEDITABLE,    // FIELD_srcGs
        FD_ISEDITABLE,    // FIELD_dstGs
        FD_ISEDITABLE,    // FIELD_failureTarget
    };
    return (field >= 0 && field < 6) ? fieldTypeFlags[field] : 0;
}

const char *DDRARoutingHeaderDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "type",
        "srcSat",
        "dstSat",
        "srcGs",
        "dstGs",
        "failureTarget",
    };
    return (field >= 0 && field < 6) ? fieldNames[field] : nullptr;
}

int DDRARoutingHeaderDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "type") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "srcSat") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "dstSat") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "srcGs") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "dstGs") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "failureTarget") == 0) return baseIndex + 5;
    return base ? base->findField(fieldName) : -1;
}

const char *DDRARoutingHeaderDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "routing::ddra::PacketType",    // FIELD_type
        "int",    // FIELD_srcSat
        "int",    // FIELD_dstSat
        "int",    // FIELD_srcGs
        "int",    // FIELD_dstGs
        "int",    // FIELD_failureTarget
    };
    return (field >= 0 && field < 6) ? fieldTypeStrings[field] : nullptr;
}

const char **DDRARoutingHeaderDescriptor::getFieldPropertyNames(int field) const
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

const char *DDRARoutingHeaderDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_type:
            if (!strcmp(propertyName, "enum")) return "routing::ddra::PacketType";
            return nullptr;
        default: return nullptr;
    }
}

int DDRARoutingHeaderDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void DDRARoutingHeaderDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'DDRARoutingHeader'", field);
    }
}

const char *DDRARoutingHeaderDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string DDRARoutingHeaderDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        case FIELD_type: return enum2string(static_cast<int>(pp->getType()), "routing::ddra::PacketType");
        case FIELD_srcSat: return long2string(pp->getSrcSat());
        case FIELD_dstSat: return long2string(pp->getDstSat());
        case FIELD_srcGs: return long2string(pp->getSrcGs());
        case FIELD_dstGs: return long2string(pp->getDstGs());
        case FIELD_failureTarget: return long2string(pp->getFailureTarget());
        default: return "";
    }
}

void DDRARoutingHeaderDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        case FIELD_srcSat: pp->setSrcSat(string2long(value)); break;
        case FIELD_dstSat: pp->setDstSat(string2long(value)); break;
        case FIELD_srcGs: pp->setSrcGs(string2long(value)); break;
        case FIELD_dstGs: pp->setDstGs(string2long(value)); break;
        case FIELD_failureTarget: pp->setFailureTarget(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'DDRARoutingHeader'", field);
    }
}

omnetpp::cValue DDRARoutingHeaderDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        case FIELD_type: return static_cast<int>(pp->getType());
        case FIELD_srcSat: return pp->getSrcSat();
        case FIELD_dstSat: return pp->getDstSat();
        case FIELD_srcGs: return pp->getSrcGs();
        case FIELD_dstGs: return pp->getDstGs();
        case FIELD_failureTarget: return pp->getFailureTarget();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'DDRARoutingHeader' as cValue -- field index out of range?", field);
    }
}

void DDRARoutingHeaderDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        case FIELD_srcSat: pp->setSrcSat(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstSat: pp->setDstSat(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcGs: pp->setSrcGs(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_dstGs: pp->setDstGs(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_failureTarget: pp->setFailureTarget(omnetpp::checked_int_cast<int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'DDRARoutingHeader'", field);
    }
}

const char *DDRARoutingHeaderDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr DDRARoutingHeaderDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void DDRARoutingHeaderDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    DDRARoutingHeader *pp = omnetpp::fromAnyPtr<DDRARoutingHeader>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'DDRARoutingHeader'", field);
    }
}

}  // namespace ddra
}  // namespace routing

namespace omnetpp {

}  // namespace omnetpp


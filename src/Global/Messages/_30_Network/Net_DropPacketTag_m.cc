//
// Generated file, do not edit! Created by opp_msgtool 6.3 from Global/Messages/_30_Network/Net_DropPacketTag.msg.
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
#include "Net_DropPacketTag_m.h"

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

namespace satellite {

Register_Class(Sat_Net_DropPacketTag)

Sat_Net_DropPacketTag::Sat_Net_DropPacketTag() : ::inet::TagBase()
{
}

Sat_Net_DropPacketTag::Sat_Net_DropPacketTag(const Sat_Net_DropPacketTag& other) : ::inet::TagBase(other)
{
    copy(other);
}

Sat_Net_DropPacketTag::~Sat_Net_DropPacketTag()
{
}

Sat_Net_DropPacketTag& Sat_Net_DropPacketTag::operator=(const Sat_Net_DropPacketTag& other)
{
    if (this == &other) return *this;
    ::inet::TagBase::operator=(other);
    copy(other);
    return *this;
}

void Sat_Net_DropPacketTag::copy(const Sat_Net_DropPacketTag& other)
{
    this->silent = other.silent;
    this->reason = other.reason;
}

void Sat_Net_DropPacketTag::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::TagBase::parsimPack(b);
    doParsimPacking(b,this->silent);
    doParsimPacking(b,this->reason);
}

void Sat_Net_DropPacketTag::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::TagBase::parsimUnpack(b);
    doParsimUnpacking(b,this->silent);
    doParsimUnpacking(b,this->reason);
}

bool Sat_Net_DropPacketTag::getSilent() const
{
    return this->silent;
}

void Sat_Net_DropPacketTag::setSilent(bool silent)
{
    this->silent = silent;
}

::inet::PacketDropReason Sat_Net_DropPacketTag::getReason() const
{
    return this->reason;
}

void Sat_Net_DropPacketTag::setReason(::inet::PacketDropReason reason)
{
    this->reason = reason;
}

class Sat_Net_DropPacketTagDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_silent,
        FIELD_reason,
    };
  public:
    Sat_Net_DropPacketTagDescriptor();
    virtual ~Sat_Net_DropPacketTagDescriptor();

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

Register_ClassDescriptor(Sat_Net_DropPacketTagDescriptor)

Sat_Net_DropPacketTagDescriptor::Sat_Net_DropPacketTagDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(satellite::Sat_Net_DropPacketTag)), "inet::TagBase")
{
    propertyNames = nullptr;
}

Sat_Net_DropPacketTagDescriptor::~Sat_Net_DropPacketTagDescriptor()
{
    delete[] propertyNames;
}

bool Sat_Net_DropPacketTagDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<Sat_Net_DropPacketTag *>(obj)!=nullptr;
}

const char **Sat_Net_DropPacketTagDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *Sat_Net_DropPacketTagDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int Sat_Net_DropPacketTagDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 2+base->getFieldCount() : 2;
}

unsigned int Sat_Net_DropPacketTagDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_silent
        FD_ISEDITABLE,    // FIELD_reason
    };
    return (field >= 0 && field < 2) ? fieldTypeFlags[field] : 0;
}

const char *Sat_Net_DropPacketTagDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "silent",
        "reason",
    };
    return (field >= 0 && field < 2) ? fieldNames[field] : nullptr;
}

int Sat_Net_DropPacketTagDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "silent") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "reason") == 0) return baseIndex + 1;
    return base ? base->findField(fieldName) : -1;
}

const char *Sat_Net_DropPacketTagDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "bool",    // FIELD_silent
        "inet::PacketDropReason",    // FIELD_reason
    };
    return (field >= 0 && field < 2) ? fieldTypeStrings[field] : nullptr;
}

const char **Sat_Net_DropPacketTagDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_reason: {
            static const char *names[] = { "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *Sat_Net_DropPacketTagDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_reason:
            if (!strcmp(propertyName, "enum")) return "inet::PacketDropReason";
            return nullptr;
        default: return nullptr;
    }
}

int Sat_Net_DropPacketTagDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void Sat_Net_DropPacketTagDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'Sat_Net_DropPacketTag'", field);
    }
}

const char *Sat_Net_DropPacketTagDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string Sat_Net_DropPacketTagDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        case FIELD_silent: return bool2string(pp->getSilent());
        case FIELD_reason: return enum2string(static_cast<int>(pp->getReason()), "inet::PacketDropReason");
        default: return "";
    }
}

void Sat_Net_DropPacketTagDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        case FIELD_silent: pp->setSilent(string2bool(value)); break;
        case FIELD_reason: pp->setReason((inet::PacketDropReason)string2enum(value, "inet::PacketDropReason")); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Sat_Net_DropPacketTag'", field);
    }
}

omnetpp::cValue Sat_Net_DropPacketTagDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        case FIELD_silent: return pp->getSilent();
        case FIELD_reason: return static_cast<int>(pp->getReason());
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'Sat_Net_DropPacketTag' as cValue -- field index out of range?", field);
    }
}

void Sat_Net_DropPacketTagDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        case FIELD_silent: pp->setSilent(value.boolValue()); break;
        case FIELD_reason: pp->setReason(static_cast<inet::PacketDropReason>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Sat_Net_DropPacketTag'", field);
    }
}

const char *Sat_Net_DropPacketTagDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr Sat_Net_DropPacketTagDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void Sat_Net_DropPacketTagDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    Sat_Net_DropPacketTag *pp = omnetpp::fromAnyPtr<Sat_Net_DropPacketTag>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Sat_Net_DropPacketTag'", field);
    }
}

}  // namespace satellite

namespace omnetpp {

}  // namespace omnetpp


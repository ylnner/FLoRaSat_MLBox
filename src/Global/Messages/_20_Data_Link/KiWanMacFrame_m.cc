//
// Generated file, do not edit! Created by opp_msgtool 6.3 from Global/Messages/_20_Data_Link/KiWanMacFrame.msg.
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
#include "KiWanMacFrame_m.h"

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

namespace mac {

Register_Enum(mac::KiWanFrameType, (mac::KiWanFrameType::UL_DATA));

Register_Class(KiWanFrame)

KiWanFrame::KiWanFrame() : ::mac::Base_MacFrame()
{
}

KiWanFrame::KiWanFrame(const KiWanFrame& other) : ::mac::Base_MacFrame(other)
{
    copy(other);
}

KiWanFrame::~KiWanFrame()
{
}

KiWanFrame& KiWanFrame::operator=(const KiWanFrame& other)
{
    if (this == &other) return *this;
    ::mac::Base_MacFrame::operator=(other);
    copy(other);
    return *this;
}

void KiWanFrame::copy(const KiWanFrame& other)
{
    this->kind = other.kind;
    this->id = other.id;
    this->srcId = other.srcId;
    this->repetition = other.repetition;
    this->RSSI = other.RSSI;
    this->SNIR = other.SNIR;
    this->queuedAt = other.queuedAt;
    this->sentAt = other.sentAt;
}

void KiWanFrame::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::mac::Base_MacFrame::parsimPack(b);
    doParsimPacking(b,this->kind);
    doParsimPacking(b,this->id);
    doParsimPacking(b,this->srcId);
    doParsimPacking(b,this->repetition);
    doParsimPacking(b,this->RSSI);
    doParsimPacking(b,this->SNIR);
    doParsimPacking(b,this->queuedAt);
    doParsimPacking(b,this->sentAt);
}

void KiWanFrame::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::mac::Base_MacFrame::parsimUnpack(b);
    doParsimUnpacking(b,this->kind);
    doParsimUnpacking(b,this->id);
    doParsimUnpacking(b,this->srcId);
    doParsimUnpacking(b,this->repetition);
    doParsimUnpacking(b,this->RSSI);
    doParsimUnpacking(b,this->SNIR);
    doParsimUnpacking(b,this->queuedAt);
    doParsimUnpacking(b,this->sentAt);
}

int KiWanFrame::getKind() const
{
    return this->kind;
}

void KiWanFrame::setKind(int kind)
{
    handleChange();
    this->kind = kind;
}

int KiWanFrame::getId() const
{
    return this->id;
}

void KiWanFrame::setId(int id)
{
    handleChange();
    this->id = id;
}

int KiWanFrame::getSrcId() const
{
    return this->srcId;
}

void KiWanFrame::setSrcId(int srcId)
{
    handleChange();
    this->srcId = srcId;
}

int KiWanFrame::getRepetition() const
{
    return this->repetition;
}

void KiWanFrame::setRepetition(int repetition)
{
    handleChange();
    this->repetition = repetition;
}

double KiWanFrame::getRSSI() const
{
    return this->RSSI;
}

void KiWanFrame::setRSSI(double RSSI)
{
    handleChange();
    this->RSSI = RSSI;
}

double KiWanFrame::getSNIR() const
{
    return this->SNIR;
}

void KiWanFrame::setSNIR(double SNIR)
{
    handleChange();
    this->SNIR = SNIR;
}

double KiWanFrame::getQueuedAt() const
{
    return this->queuedAt;
}

void KiWanFrame::setQueuedAt(double queuedAt)
{
    handleChange();
    this->queuedAt = queuedAt;
}

double KiWanFrame::getSentAt() const
{
    return this->sentAt;
}

void KiWanFrame::setSentAt(double sentAt)
{
    handleChange();
    this->sentAt = sentAt;
}

class KiWanFrameDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_kind,
        FIELD_id,
        FIELD_srcId,
        FIELD_repetition,
        FIELD_RSSI,
        FIELD_SNIR,
        FIELD_queuedAt,
        FIELD_sentAt,
    };
  public:
    KiWanFrameDescriptor();
    virtual ~KiWanFrameDescriptor();

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

Register_ClassDescriptor(KiWanFrameDescriptor)

KiWanFrameDescriptor::KiWanFrameDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(mac::KiWanFrame)), "mac::Base_MacFrame")
{
    propertyNames = nullptr;
}

KiWanFrameDescriptor::~KiWanFrameDescriptor()
{
    delete[] propertyNames;
}

bool KiWanFrameDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<KiWanFrame *>(obj)!=nullptr;
}

const char **KiWanFrameDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *KiWanFrameDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int KiWanFrameDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 8+base->getFieldCount() : 8;
}

unsigned int KiWanFrameDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_kind
        FD_ISEDITABLE,    // FIELD_id
        FD_ISEDITABLE,    // FIELD_srcId
        FD_ISEDITABLE,    // FIELD_repetition
        FD_ISEDITABLE,    // FIELD_RSSI
        FD_ISEDITABLE,    // FIELD_SNIR
        FD_ISEDITABLE,    // FIELD_queuedAt
        FD_ISEDITABLE,    // FIELD_sentAt
    };
    return (field >= 0 && field < 8) ? fieldTypeFlags[field] : 0;
}

const char *KiWanFrameDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "kind",
        "id",
        "srcId",
        "repetition",
        "RSSI",
        "SNIR",
        "queuedAt",
        "sentAt",
    };
    return (field >= 0 && field < 8) ? fieldNames[field] : nullptr;
}

int KiWanFrameDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "kind") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "id") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "srcId") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "repetition") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "RSSI") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "SNIR") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "queuedAt") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "sentAt") == 0) return baseIndex + 7;
    return base ? base->findField(fieldName) : -1;
}

const char *KiWanFrameDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_kind
        "int",    // FIELD_id
        "int",    // FIELD_srcId
        "int",    // FIELD_repetition
        "double",    // FIELD_RSSI
        "double",    // FIELD_SNIR
        "double",    // FIELD_queuedAt
        "double",    // FIELD_sentAt
    };
    return (field >= 0 && field < 8) ? fieldTypeStrings[field] : nullptr;
}

const char **KiWanFrameDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_kind: {
            static const char *names[] = { "enum", "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *KiWanFrameDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        case FIELD_kind:
            if (!strcmp(propertyName, "enum")) return "KiWanFrameType";
            if (!strcmp(propertyName, "enum")) return "mac::KiWanFrameType";
            return nullptr;
        default: return nullptr;
    }
}

int KiWanFrameDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void KiWanFrameDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'KiWanFrame'", field);
    }
}

const char *KiWanFrameDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string KiWanFrameDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        case FIELD_kind: return enum2string(static_cast<int>(pp->getKind()), "mac::KiWanFrameType");
        case FIELD_id: return long2string(pp->getId());
        case FIELD_srcId: return long2string(pp->getSrcId());
        case FIELD_repetition: return long2string(pp->getRepetition());
        case FIELD_RSSI: return double2string(pp->getRSSI());
        case FIELD_SNIR: return double2string(pp->getSNIR());
        case FIELD_queuedAt: return double2string(pp->getQueuedAt());
        case FIELD_sentAt: return double2string(pp->getSentAt());
        default: return "";
    }
}

void KiWanFrameDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        case FIELD_kind: pp->setKind((mac::KiWanFrameType)string2enum(value, "mac::KiWanFrameType")); break;
        case FIELD_id: pp->setId(string2long(value)); break;
        case FIELD_srcId: pp->setSrcId(string2long(value)); break;
        case FIELD_repetition: pp->setRepetition(string2long(value)); break;
        case FIELD_RSSI: pp->setRSSI(string2double(value)); break;
        case FIELD_SNIR: pp->setSNIR(string2double(value)); break;
        case FIELD_queuedAt: pp->setQueuedAt(string2double(value)); break;
        case FIELD_sentAt: pp->setSentAt(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'KiWanFrame'", field);
    }
}

omnetpp::cValue KiWanFrameDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        case FIELD_kind: return pp->getKind();
        case FIELD_id: return pp->getId();
        case FIELD_srcId: return pp->getSrcId();
        case FIELD_repetition: return pp->getRepetition();
        case FIELD_RSSI: return pp->getRSSI();
        case FIELD_SNIR: return pp->getSNIR();
        case FIELD_queuedAt: return pp->getQueuedAt();
        case FIELD_sentAt: return pp->getSentAt();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'KiWanFrame' as cValue -- field index out of range?", field);
    }
}

void KiWanFrameDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        case FIELD_kind: pp->setKind((mac::KiWanFrameType)value.intValue()); break;
        case FIELD_id: pp->setId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_srcId: pp->setSrcId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_repetition: pp->setRepetition(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_RSSI: pp->setRSSI(value.doubleValue()); break;
        case FIELD_SNIR: pp->setSNIR(value.doubleValue()); break;
        case FIELD_queuedAt: pp->setQueuedAt(value.doubleValue()); break;
        case FIELD_sentAt: pp->setSentAt(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'KiWanFrame'", field);
    }
}

const char *KiWanFrameDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr KiWanFrameDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void KiWanFrameDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    KiWanFrame *pp = omnetpp::fromAnyPtr<KiWanFrame>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'KiWanFrame'", field);
    }
}

}  // namespace mac

namespace omnetpp {

}  // namespace omnetpp


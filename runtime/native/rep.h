// Copyright (C) 2018 Cornell University

//
// This file includes class declarations that represent the layout of
// Java objects, dispatch vectors, arrays, and more.
//
// Other native code can interact with these structures through the
// inlined functions defined here.
//
#pragma once

#include "interface.h"
#include <cinttypes>
#include <jni.h>
#include <type_traits>

// Ensure that our C++ representations are POD types,
// i.e., that they have the layout we'd expect from a C struct.
#define ASSERT_POD(layout)                                                     \
    static_assert(                                                             \
        std::is_trivial<layout>::value &&                                      \
            std::is_standard_layout<layout>::value,                            \
        "Java object representations should have the layout of C structs")

struct JObjectRep;
struct JArrayRep;
struct JStringRep;
struct JClassRep;

struct type_info {
    int32_t size;
    void *super_type_ids[];
};

// Class dispatch vector.
struct DispatchVector {
    JClassRep *Class() { return *class_; }
    void SetClassPtr(JClassRep **class_ptr) { class_ = class_ptr; }
    idv_ht *Idv() { return idv_; }
    void SetIdv(idv_ht *idv) { idv_ = idv; }
    type_info *SuperTypes() { return super_types_; }

  public:
    JClassRep **class_; // Notice: double-pointer.
    idv_ht *idv_;
    type_info *super_types_;
    void *methods_[0]; // a list of method pointers in dv.
};

// Currently unimplemented.
struct sync_vars {
    // pthread_mutex_t* mutex;
    // pthread_cond_t *condition_variable;
};

// Representation for java.lang.Object.
struct JObjectRep {
    DispatchVector *Cdv() { return cdv_; }
    sync_vars *SyncVars() { return sync_vars_; }
    jobject Wrap() { return reinterpret_cast<jobject>(this); }
    void SetCdv(DispatchVector *cdv) { cdv_ = cdv; }

  private:
    DispatchVector *cdv_;
    sync_vars *sync_vars_;
};
ASSERT_POD(JObjectRep);

// Representation for Java built-in arrays.
struct JArrayRep {
    jsize Length() { return len_; }
    jsize ElemSize() { return elem_size_; }
    void *Data() { return data_; }
    JObjectRep *Super() { return &header_; }
    jarray Wrap() { return reinterpret_cast<jarray>(this); }
    void SetLength(jsize len) { len_ = len; }
    void SetElemSize(jsize elem_size) { elem_size_ = elem_size; }

  private:
    JObjectRep header_;
    jsize len_;
    jsize elem_size_;
    char data_[0];
};
ASSERT_POD(JArrayRep);

// Representation for java.lang.String.
struct JStringRep {
    JArrayRep *Chars() { return value_; }
    JObjectRep *Super() { return &header_; }
    jstring Wrap() { return reinterpret_cast<jstring>(this); }

  private:
    JObjectRep header_;
    JArrayRep *value_;
};
ASSERT_POD(JStringRep);

// Representation for java.lang.Class.
// This representation is primarily meant to store metadata about the class
// The java.lang.Class object has many fields
struct JClassRep {
    // "Unwraps" the JClassRep into an object rep (i.e. returns the same pointer
    // but with a different type)
    JObjectRep *Super() { return &header_; }
    jclass Wrap() { return reinterpret_cast<jclass>(this); }

  private:
    JObjectRep header_;
};
ASSERT_POD(JClassRep);

// These functions convert an opaque Java object reference into
// a pointer to the appropriate C++ representation.
inline JObjectRep *Unwrap(jobject o) {
    return reinterpret_cast<JObjectRep *>(o);
}
inline JArrayRep *Unwrap(jarray o) { return reinterpret_cast<JArrayRep *>(o); }
inline JStringRep *Unwrap(jstring o) {
    return reinterpret_cast<JStringRep *>(o);
}
inline JClassRep *Unwrap(jclass o) { return reinterpret_cast<JClassRep *>(o); }
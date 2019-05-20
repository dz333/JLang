//Copyright (C) 2018 Cornell University

#pragma once

#include <jni.h>

extern "C" {

void *__getInterfaceMethod(jobject obj, int intf_id_hash, void *intf_id,
                           int method_index);

struct idv_ht_node {
    idv_ht_node *next;
    void *intf_id;
    void *idv;
};

} // extern "C"

// The hash table of interface dispatch vectors.
// representation type is actually a linked list
class idv_ht {
  public:
    /**
     * Constructor.
     * @param capacity must be a power of 2
     */
    idv_ht(size_t capacity);

    /**
     * Fetch an interface dispatch vector with its precomputed hash
     * code.
     */
    void *get(int hashcode, void *intf_id);

    /**
     * Add an interface dispatch vector with its precomputed hash
     * code.
     */
    void put(int hashcode, void *intf_id, void *idv);

  private:
    idv_ht_node **table;
    size_t capacity;
    size_t getIndexForHash(int h);
};

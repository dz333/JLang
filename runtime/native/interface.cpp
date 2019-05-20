// Copyright (C) 2018 Cornell University

#include "rep.h"
#include <cstdlib>
#include <inttypes.h>
#include <string.h>

#include "interface.h"

extern "C" {

void __createInterfaceTables(DispatchVector *D, int capacity, int size,
                             int intf_id_hashcodes[], void *intf_ids[],
                             void *intf_tables[]) {
    idv_ht *ittab = new idv_ht(capacity);
    for (int i = 0; i < size; ++i)
        ittab->put(intf_id_hashcodes[i], intf_ids[i], intf_tables[i]);
    D->SetIdv(ittab);
}

void *__getInterfaceMethod(jobject obj, int intf_id_hash, void *intf_id,
                           int method_index) {
    idv_ht *ittab = Unwrap(obj)->Cdv()->Idv();
    void **itab = reinterpret_cast<void **>(ittab->get(intf_id_hash, intf_id));
    return itab[method_index];
}

} // extern "C"

idv_ht::idv_ht(size_t capacity) {
    this->table = new idv_ht_node *[capacity];
    this->capacity = capacity;
}

void *idv_ht::get(int hashcode, void *intf_id) {
    int index = getIndexForHash(hashcode);
    idv_ht_node *node = table[index];
    while (node->next != nullptr) {
        if (node->intf_id == intf_id)
            return node->idv;
        node = node->next;
    }
    return node->idv;
}

void idv_ht::put(int hashcode, void *intf_id, void *idv) {
    idv_ht_node *node = new idv_ht_node;
    node->intf_id = intf_id;
    node->idv = idv;

    int index = getIndexForHash(hashcode);
    idv_ht_node *lst = table[index];
    if (lst == nullptr) {
        table[index] = node;
    } else {
        idv_ht_node *tail = lst;
        table[index] = node;
        node->next = tail;
    }
}

size_t idv_ht::getIndexForHash(int h) {
    size_t index = h & (this->capacity - 1);
    return index;
}

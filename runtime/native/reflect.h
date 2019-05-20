// Copyright (C) 2018 Cornell University

#pragma once

#include "class.h"
#include "factory.h"
#include "rep.h"
#include <stdio.h>
extern "C" {
bool InstanceOf(jobject obj, void *compare_type_id);
} // extern "C"

jobject CreateConstructor(jclass declaring_clazz,
                          const JavaClassInfo *clazz_info,
                          JavaMethodInfo ctor_info);

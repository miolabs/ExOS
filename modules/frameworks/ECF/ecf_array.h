//
//  ecf_array.h
//  ExOS Core Foundation
//
//  Created by GodShadow on 26/10/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#ifndef __ECF_ARRAY__
#define __ECF_ARRAY__

#include "ecf_types.h"

typedef struct ecf_array_struct ecf_array;


ecf_array *ecf_array_create();
void ecf_array_destroy(ecf_array *array);

UInt32 ecf_array_get_count(ecf_array *array);
void *ecf_array_get_object_at_index(ecf_array *array, UInt32 index);

void ecf_array_add_object(ecf_array *array, void *object);
void ecf_array_remove_object(ecf_array *array, void *object);
void ecf_array_remove_object_at_index(ecf_array *array, UInt32 index);


#endif /* defined(__ECF_ARRAY__) */

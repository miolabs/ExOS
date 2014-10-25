//
//  ECF_DATA.h
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/10/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#ifndef __ECF_DATA__
#define __ECF_DATA__

#include "ecf_types.h"

typedef struct ecf_data_struct ecf_data;

ecf_data *ecf_data_create(UInt32 size);
void ecf_data_destroy(ecf_data *data);

UInt32 ecf_data_get_len(ecf_data *data);

void ecf_data_append_byte(ecf_data *data, UInt8 byte);
void ecf_data_append_bytes(ecf_data *data, UInt8 *byte, int len);


#endif /* defined(__ECF_DATA__) */

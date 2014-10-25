//
//  ecf_data.c
//  EnerlinBoardTest
//
//  Created by GodShadow on 25/10/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_data.h"
#include <stdlib.h>

#define kECF_DATA_DEFAULT_SIZE 256

struct ecf_data_struct
{
    UInt8 *bytes;
    UInt32 size;
    UInt32 len;
};


ecf_data *ecf_data_create(UInt32 size)
{
    if (size == 0)
        return NULL;
    
    ecf_data *data = malloc (sizeof(struct ecf_data_struct));
    if (data == NULL)
        return NULL;
    
    data->bytes = malloc(size);
    if (data->bytes == NULL)
    {
        free(data);
        return NULL;
    }

    data->len = 0;
    data->size = size;

    return data;
}

void ecf_data_destroy(ecf_data *data)
{
    if (data == NULL)
        return;
    
    if (data->bytes != NULL)
    {
        free(data->bytes);
        data->bytes = NULL;
    }
    
    data->len = 0;
    data->size = 0;
    
    free(data);
}

UInt32 ecf_data_get_len(ecf_data *data)
{
    if (data == 0)
        return 0;
    
    return data->len;
}

void ecf_data_append_byte(ecf_data *data, UInt8 byte)
{
    
}

void ecf_data_append_bytes(ecf_data *data, UInt8 *byte, int len)
{
    
}

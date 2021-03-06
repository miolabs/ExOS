//
//  ecf_data.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/10/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_data.h"
#include <stdlib.h>
#include <string.h>

struct ecf_data_struct
{
    UInt8 *bytes;
    UInt32 size;
    UInt32 len;
};

// Private functions
void __ecf_data_add_bytes(ecf_data *data, UInt8 bytes[], UInt32 len);


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

ecf_data *ecf_data_create_with_bytes(UInt8 *bytes, UInt32 len)
{
    ecf_data *data = ecf_data_create(len);
    __ecf_data_add_bytes(data, bytes, len);
    
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

UInt8 *ecf_data_get_bytes(ecf_data *data)
{
    if (data == NULL)
        return NULL;
    
    return data->bytes;
}

void ecf_data_append_byte(ecf_data *data, UInt8 byte)
{
    __ecf_data_add_bytes(data, &byte, 1);
}

void ecf_data_append_bytes(ecf_data *data, UInt8 *bytes, UInt32 len)
{
    __ecf_data_add_bytes(data, bytes, len);
}

void ecf_data_append_data(ecf_data *data, ecf_data *new_data)
{
    __ecf_data_add_bytes(data, new_data->bytes, new_data->len);
}

#pragma mark - Private functions

void __ecf_data_add_bytes(ecf_data *data, UInt8 bytes[], UInt32 len)
{
    if (len < 1)
        return;
    
    if (bytes == NULL)
        return;
    
    if (data->len + len < data->size)
    {
        memcpy(data->bytes + data->len, bytes, len);
        data->len += len;
    }
    else
    {
        UInt8 *buffer = (UInt8 *)malloc(data->len + len + kECF_DATA_DEFAULT_SIZE);
        if (buffer == NULL)
            return;
        // Copy old data
        memcpy(buffer, data->bytes, data->len);
        // Append new data
        memcpy(buffer + data->len, bytes, len);
        free(data->bytes);
        data->bytes = buffer;
        data->len += len;
        data->size = data->len + kECF_DATA_DEFAULT_SIZE;
    }
}


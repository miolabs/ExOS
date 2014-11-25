//
//  ecf_url_connection.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_url_connection.h"
#include <stdlib.h>

struct ecf_url_connection_struct
{
    char *url_string;
    
};

ecf_url_connection *ecf_url_connection_create(char *url_string)
{
    if (url_string == NULL)
        return NULL;
    
    ecf_url_connection *conn = malloc(sizeof(struct ecf_url_connection_struct));
    if (conn == NULL)
        return NULL;

    conn->url_string = url_string;
    
    return conn;
}

void ecf_url_connection_destroy(ecf_url_connection *url_connection)
{
    
}

void ecf_url_connection_set_http_method(ecf_url_connection *url_connection, char *http_method)
{
    
}

void ecf_url_connection_add_http_header_value(ecf_url_connection *url_connection, char *header, char *value)
{
    
}

void ecf_url_connection_set_http_body(ecf_url_connection *url_connection, ecf_data *data)
{
    
}

void ecf_url_connection_set_http_body_string(ecf_url_connection *url_connection, char *body)
{
    
}

void ecf_url_connection_start(ecf_url_connection *url_connection)
{
    
}

void ecf_url_connection_stop(ecf_url_connection *url_connection)
{
    
}

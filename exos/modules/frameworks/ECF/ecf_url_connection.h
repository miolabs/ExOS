//
//  ecf_url_connection.h
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#ifndef __ECF_URL_CONNECTION__
#define __ECF_URL_CONNECTION__

#include "ecf_types.h"
#include "ecf_data.h"
#include "ecf_url.h"

typedef struct ecf_url_connection_struct ecf_url_connection;

typedef void (*ecf_url_connection_did_connected_callback) (ecf_url_connection *url_connection);
typedef void (*ecf_url_connection_did_disconnected_callback) (ecf_url_connection *url_connection);
typedef void (*ecf_url_connection_did_fail_callback) (ecf_url_connection *url_connection);
typedef void (*ecf_url_connection_did_receive_data_callback) (ecf_url_connection *url_connection, ecf_data *data);

typedef struct ecf_url_connection_callbacks
{
    ecf_url_connection_did_connected_callback connected_cb;
    ecf_url_connection_did_disconnected_callback disconnected_cb;
    ecf_url_connection_did_fail_callback fail_cb;
    ecf_url_connection_did_receive_data_callback recived_data_cb;
    
}ecf_url_connection_callbacks;

ecf_url_connection *ecf_url_connection_create(ecf_url *url);
void ecf_url_connection_destroy(ecf_url_connection *url_connection);

void ecf_url_connection_set_callbacks(ecf_url_connection *url_connection, ecf_url_connection_callbacks *callbacks);

void ecf_url_connection_set_http_method(ecf_url_connection *url_connection, char *http_method);
void ecf_url_connection_add_http_header_value(ecf_url_connection *url_connection, char *header, char *value);
void ecf_url_connection_set_http_body(ecf_url_connection *url_connection, ecf_data *data);
void ecf_url_connection_set_http_body_string(ecf_url_connection *url_connection, char *string);

void ecf_url_connection_start(ecf_url_connection *url_connection);
void ecf_url_connection_stop(ecf_url_connection *url_connection);

#endif /* defined(__ECF_URL_CONNECTION__) */

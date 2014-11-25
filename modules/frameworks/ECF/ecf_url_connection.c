//
//  ecf_url_connection.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_url_connection.h"
#include <stdlib.h>

// POSIX Network headers. Change to ExOS network API!
#include <sys/socket.h>
#include <netinet/in.h>

// TEST URL: http://www.enerlin.es/webservice/nrlin/reading/insertcurve

struct ecf_url_connection_struct
{
    ecf_url *url;
};

ecf_url_connection *ecf_url_connection_create(ecf_url *url)
{
    if (url == NULL)
        return NULL;
    
    ecf_url_connection *conn = malloc(sizeof(struct ecf_url_connection_struct));
    if (conn == NULL)
        return NULL;

    conn->url = url;
    
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
/*    struct sockaddr_in server;
    
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("87.98.231.17");
    server.sin_port = htons(80);
    
    char message[200];
    char *ws = type == 0 ? kSendRFIDValueWS : kSendTempValueWS;
    sprintf(message, "GET %s%d HTTP/1.1\nHost: adingo.es\n\n", ws, value);
    
    int ret = connect(sd, (const struct sockaddr *)&server, sizeof(server));
    if (ret == -1)
    {
        close(sd);
        return;
    }
    size_t len = strlen(message);
    write(sd, message, len);
    
    read(sd, message, sizeof(message));
    close(sd);*/
}

void ecf_url_connection_stop(ecf_url_connection *url_connection)
{
    if (url_connection != NULL)
        free(url_connection);
}

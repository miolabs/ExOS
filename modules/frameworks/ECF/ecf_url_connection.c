//
//  ecf_url_connection.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_url_connection.h"
#include <stdlib.h>
#include <string.h>

// POSIX Network headers. Change to ExOS network API!
#include <arpa/inet.h>
#include <unistd.h>

struct ecf_url_connection_struct
{
    ecf_url *url;
    char http_method[4];
    ecf_data *http_body;
};

ecf_url_connection *ecf_url_connection_create(ecf_url *url)
{
    if (url == NULL)
        return NULL;
    
    ecf_url_connection *conn = malloc(sizeof(struct ecf_url_connection_struct));
    if (conn == NULL)
        return NULL;

    conn->url = url;
    strcpy(conn->http_method, "GET");
    conn->http_body = NULL;
    
    return conn;
}

void ecf_url_connection_destroy(ecf_url_connection *url_connection)
{
    if (url_connection == NULL)
        return;
    
    ecf_url_destroy(url_connection->url);
    free(url_connection->http_method);
}

void ecf_url_connection_set_http_method(ecf_url_connection *url_connection, char *http_method)
{
    if (url_connection == NULL || http_method == NULL)
        return;
    
    strcpy(url_connection->http_method, http_method);
}

void ecf_url_connection_add_http_header_value(ecf_url_connection *url_connection, char *header, char *value)
{
    
}

void ecf_url_connection_set_http_body(ecf_url_connection *url_connection, ecf_data *data)
{
    if (url_connection == NULL || data == NULL)
        return;
    
    ecf_data_destroy(url_connection->http_body);
    url_connection->http_body = data;
}

void ecf_url_connection_set_http_body_string(ecf_url_connection *url_connection, char *string)
{
    if (url_connection == NULL)
        return;
    
    UInt32 len = (UInt32)strlen(string);
    ecf_data *body = ecf_data_create(len + 1);
    
    ecf_data_append_bytes(body, (UInt8 *)string, len);
    
    ecf_url_connection_set_http_body(url_connection, body);
}

bool _ecf_url_connection_write(int sd, UInt8 *bytes, size_t len)
{
    if (len == 0)
        return false;
    
    int count = 0;
    
    ssize_t written_bytes = 0;
    do
    {
        written_bytes = write(sd, bytes, len);
        if (written_bytes > 0)
            count += written_bytes;
        
    } while (count < len && written_bytes != -1);
    
    if (written_bytes == -1)
        return false;
    
    return true;
}

bool _ecf_url_connection_write_string(int sd, const char *string)
{
    size_t len = strlen(string);
    return _ecf_url_connection_write(sd, (UInt8 *)string, len);
}

void _ecf_url_connecion_read(int sd)
{
    char buffer[256];
    
    int read_bytes = 0;
    do {
        read_bytes = read(sd, buffer, 256);
        printf("%s", buffer);
    } while (read_bytes > 0);
}

void ecf_url_connection_start(ecf_url_connection *url_connection)
{
    if (url_connection == NULL)
        return;
    
    const char *http_method = url_connection->http_method;
    const char *host = ecf_url_get_host(url_connection->url);
    const char *relative_path = ecf_url_get_relative_path(url_connection->url);
    
    struct sockaddr_in server;
    
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_port = htons(80);
    
    
    int ret = connect(sd, (const struct sockaddr *)&server, sizeof(server));
    if (ret == -1)
    {
        close(sd);
        return;
    }

    char message[200];
    sprintf(message, "%s /%s HTTP/1.1\nHost: %s\n", http_method, relative_path, "enerlin.es");
    _ecf_url_connection_write_string(sd, message);
    
    if (strcmp(http_method, "GET") == 0)
    {
        sprintf(message, "\n");
        _ecf_url_connection_write_string(sd, message);
    }
    else if (strcmp(http_method, "POST") == 0)
    {
        sprintf(message, "Content-Type: application/x-www-form-urlencoded\n");
        _ecf_url_connection_write_string(sd, message);
        
        sprintf(message, "Content-Length: %d\n\n", ecf_data_get_len(url_connection->http_body));
        _ecf_url_connection_write_string(sd, message);
        
        _ecf_url_connection_write(sd, ecf_data_get_bytes(url_connection->http_body), ecf_data_get_len(url_connection->http_body));
    }
    
    _ecf_url_connecion_read(sd);
    close(sd);
}

void ecf_url_connection_stop(ecf_url_connection *url_connection)
{
    if (url_connection != NULL)
        free(url_connection);
}

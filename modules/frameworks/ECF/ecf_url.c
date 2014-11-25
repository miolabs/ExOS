//
//  ecf_url.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_url.h"

#include <stdlib.h>
#include <string.h>

struct ecf_url_struct
{
    char *scheme;
    char *host;
    char *relative_path;
};


#pragma mark - Private functions

char *_ecf_url_get_token_until_char(const char *string, char end_char, int *offset)
{
    int count = 0;
    char buffer[255];
    char *token = NULL;
    char ch;
    bool exit = false;

    do
    {
        ch = *(string + count + (*offset));
        
        if (ch == end_char)
        {
            token = malloc(sizeof(char) * count);
            strncpy(token, buffer, count);
            (*offset) += count;
            
            exit = true;
        }
        else
            buffer[count] = ch;
        
        count++;
        
    } while (exit == false);
    
    return token;
}

void _ecf_url_parse_string(ecf_url *url, const char *url_string)
{
    int offset = 0;
    
    url->scheme = _ecf_url_get_token_until_char(url_string, ':', &offset);
    // skip the double '/'
    offset += 2;
    url->host = _ecf_url_get_token_until_char(url_string, '/', &offset);
    url->relative_path = _ecf_url_get_token_until_char(url_string, '\0', &offset);
}

#pragma mark - Public functions

ecf_url *ecf_url_create_with_string(const char *url_string)
{
    ecf_url *url = malloc(sizeof(struct ecf_url_struct));
    
    _ecf_url_parse_string(url, url_string);
    
    return url;
}

void ecf_url_destroy(ecf_url *url)
{
    if (url == NULL)
        return;
    
    free(url->scheme);
    free(url->host);
    free(url->relative_path);
    
    free(url);
}



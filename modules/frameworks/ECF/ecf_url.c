//
//  ecf_url.c
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#include "ecf_url.h"
#include <stdlib.h>

struct ecf_url_struct
{
    char *host;
    char *path;
};

ecf_url *ecf_url_create_with_string(const char *url_string)
{
    ecf_url *url = malloc(sizeof(struct ecf_url_struct));
    
    // Parse url string
    
    return url;
}

void ecf_url_destroy(ecf_url *url)
{
    
}

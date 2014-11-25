//
//  ecf_url.h
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/11/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#ifndef __ECF_URL__
#define __ECF_URL__

#include "ecf_types.h"

typedef struct ecf_url_struct ecf_url;

ecf_url *ecf_url_create_with_string(const char *url_string);
void ecf_url_destroy(ecf_url *url);


#endif /* defined(__ECF_URL__) */

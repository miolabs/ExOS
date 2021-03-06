//
//  ecf_types.h
//  ExOS Core Foundation
//
//  Created by GodShadow on 25/10/14.
//  Copyright (c) 2014 MIO Research Labs. All rights reserved.
//

#ifndef __ECF_TYPES__
#define __ECF_TYPES__

#include <stdio.h>
#include <stdbool.h>

#ifndef UInt8
#define UInt8 uint8_t
#endif

#ifndef UInt16
#define UInt16 uint16_t
#endif

#ifndef UInt32
#define UInt32 uint32_t
#endif

#define LOBYTE(x) ((unsigned char) ((x) & 0xff))
#define HIBYTE(x) ((unsigned char) ((x) >> 8 & 0xff))

#endif

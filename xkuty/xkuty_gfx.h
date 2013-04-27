#ifndef XCUTY_GFX_H
#define XCUTY_GFX_H


const static unsigned int _bmp_mi [] = 
{
0x0, 0x0, 0x1000, 0x0, 
0x361000, 0x491000, 0x491000, 0x491000, 
0x0, 0x0
};
const static unsigned int _bmp_km [] = 
{
0x0, 0x48000000, 0x50d80000, 0x61240000, 
0x51240000, 0x49240000, 0x0
};
const static unsigned int _bmp_kmh [] = 
{
0x4000, 0x24004000, 0x286c7000, 0x30924800, 
0x28924800, 0x24924800, 0x0
};
const static unsigned int _bmp_mph [] = 
{
0x0, 0x8000, 0x8000, 0x3638e000, 
0x49249000, 0x49249000, 0x49389000, 0x200000, 
0x200000, 0x0
};

const static unsigned int _bmp_battery [] = 
{
0x0, 0x0, 0x1fff000, 0x1fff000, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x0, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x0, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x0, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x3fffff80, 0x3fffff80, 0x3fffff80, 
0x3fffff80, 0x0, 0x0
};
const static unsigned int _bmp_battery_empty [] = 
{ 
0x0, 0x0, 0x1fff000, 0x1803000, 
0x3f803f80, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x3fffff80, 
0x0, 0x3fffff80, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x3fffff80, 0x0, 0x3fffff80, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x3fffff80, 0x0, 
0x3fffff80, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x30000180, 
0x30000180, 0x30000180, 0x30000180, 0x3fffff80, 
0x3fffff80, 0x0, 0x0
};

const static unsigned int _bmp_lock []  = 
{
0x0, 0x3ffffc00, 0x40000200, 0x40000200, 
0x40000200, 0x4f00f200, 0x4f80f200, 0x4fc0f200, 
0x4fe0f200, 0x4ff0f200, 0x4ff8f200, 0x4ffcf200, 
0x4f7ef200, 0x4f3ff200, 0x4f1ff200, 0x4f0ff200, 
0x4f07f200, 0x4f03f200, 0x4f01f200, 0x4f00f200, 
0x40000200, 0x40000200, 0x40000200, 0x3ffffc00, 
0x0
};

const static unsigned int _bmp_warning []  = 
{
0x0, 0x0, 0x0, 0x180000, 
0x3c0000, 0x660000, 0x420000, 0x990000, 
0x1998000, 0x1188000, 0x318c000, 0x2184000, 
0x6006000, 0x4182000, 0xc183000, 0x8001000, 
0xffff000, 0x0, 0x0
};

const static unsigned int _bmp_fatal_error [] = 
{ 
0x0, 0x3fc0000, 0x600000, 0xffc0000, 
0x58040000, 0x50060000, 0x5002e000, 0x5002a000, 
0x7003a000, 0x70002000, 0x50002000, 0x50002000, 
0x5003a000, 0x5e02a000, 0x386e000, 0xfc0000, 
0x0
};

const static unsigned int _bmp_cruisin [] = 
{
0x0, 0x60000000, 0x70000000, 0x3a000000, 
0x1e3f0000, 0xec4c000, 0x1f042000, 0x3041000, 
0x5802800, 0x8c04400, 0x8600400, 0x83c0400, 
0xf123c00, 0x8120400, 0x80c0400, 0x8000400, 
0x4000800, 0x2001000, 0x0
};

const static unsigned int _bmp_nums_speed [] = 
{ 
0xfffc000, 0x3ffff000, 0x7ffff800, 0x7ffff800, 
0xf8007c00, 0xf0003c00, 0xf0003c00, 0xf0003c00, 
0xf0003c00, 0xf0003c00, 0xf0003c00, 0xf0003c00, 
0xf0003c00, 0xf0003c00, 0xf0003c00, 0xf0003c00, 
0xf8007c00, 0x7ffff800, 0x7ffff800, 0x3ffff000, 
0xfffc000, 0x7fc0000, 0x7fc0000, 0x7fc0000, 
0x7fc0000, 0x3c0000, 0x3c0000, 0x3c0000, 
0x3c0000, 0x3c0000, 0x3c0000, 0x3c0000, 
0x3c0000, 0x3c0000, 0x3c0000, 0x3c0000, 
0x3c0000, 0x3c0000, 0x7ffe000, 0x7ffe000, 
0x7ffe000, 0x7ffe000, 0x3fffc000, 0x3ffff000, 
0x3ffff800, 0x3ffff800, 0x7c00, 0x3c00, 
0x3c00, 0x7c00, 0xffff800, 0x3ffff800, 
0x7ffff000, 0x7fffc000, 0xf8000000, 0xf0000000, 
0xf0000000, 0xf0000000, 0xf0000000, 0xfffff800, 
0xfffff800, 0xfffff800, 0xfffff800, 0x3fff0000, 
0x3fffc000, 0x3fffe000, 0x3fffe000, 0x1f000, 
0xf000, 0xf000, 0x1f000, 0x7ff000, 
0x7fe000, 0x7fe000, 0x7ff000, 0xf800, 
0x7800, 0x7800, 0x7800, 0xf800, 
0x3ffff000, 0x3ffff000, 0x3fffe000, 0x3fff8000, 
0xf001e000, 0xf001e000, 0xf001e000, 0xf001e000, 
0xf001e000, 0xf001e000, 0xf001e000, 0xf001e000, 
0xf001e000, 0xf001e000, 0xf001e000, 0xf001e000, 
0xf801e000, 0xfffffc00, 0x7ffffc00, 0x3ffffc00, 
0x1ffffc00, 0x1e000, 0x1e000, 0x1e000, 
0x1e000, 0xfffff800, 0xfffff800, 0xfffff800, 
0xfffff800, 0xf0000000, 0xf0000000, 0xf0000000, 
0xf8000000, 0x7fff8000, 0x7fffe000, 0x3ffff000, 
0xffff000, 0xf800, 0x7800, 0x7800, 
0x7800, 0xf800, 0x3ffff000, 0x3ffff000, 
0x3fffe000, 0x3fff8000, 0x7fff800, 0x1ffff800, 
0x3ffff800, 0x3ffff800, 0x7c000000, 0x78000000, 
0x78000000, 0x7fffc000, 0x7ffff000, 0x7ffff800, 
0x7ffff800, 0x78007c00, 0x78003c00, 0x78003c00, 
0x78003c00, 0x78003c00, 0x7c007c00, 0x3ffff800, 
0x3ffff800, 0x1ffff000, 0x7ffc000, 0xfffff800, 
0xfffff800, 0xfffff800, 0xfffff800, 0xf000f000, 
0xf001e000, 0x3e000, 0x7c000, 0xf8000, 
0x1f0000, 0x1e0000, 0x3c0000, 0x7c0000, 
0xf80000, 0x1f00000, 0x1e00000, 0x1e00000, 
0x1e00000, 0x1e00000, 0x1e00000, 0x1e00000, 
0x7ffc000, 0x1ffff000, 0x3ffff800, 0x3ffff800, 
0x7c007c00, 0x78003c00, 0x78003c00, 0x7c007c00, 
0x3ffff800, 0x3ffff800, 0x3ffff800, 0x7ffffc00, 
0xfc003c00, 0xf8001c00, 0xf8001c00, 0xf8001c00, 
0xfc003c00, 0x7ffffc00, 0x7ffffc00, 0x3ffff800, 
0xfffe000, 0xfff8000, 0x3fffe000, 0x7ffff000, 
0x7ffff000, 0xf800f800, 0xf0007800, 0xf0007800, 
0xf0007800, 0xf0007800, 0xf8007800, 0x7ffff800, 
0x7ffff800, 0x3ffff800, 0xffff800, 0x7800, 
0x7800, 0xf800, 0x3ffff000, 0x3ffff000, 
0x3fffe000, 0x3fff8000
};

const static unsigned int _bmp_nums_distance [] = 
{ 
0x7f800000, 0xffc00000, 0xc0c00000, 0xc0c00000, 
0xc0c00000, 0xc0c00000, 0xc0c00000, 0xffc00000, 
0x7f800000, 0x3c000000, 0x3c000000, 0xc000000, 
0xc000000, 0xc000000, 0xc000000, 0xc000000, 
0x3f000000, 0x3f000000, 0x7f800000, 0x7fc00000, 
0xc00000, 0x7fc00000, 0xff800000, 0xc0000000, 
0xc0000000, 0xffc00000, 0xffc00000, 0x3f800000, 
0x3fc00000, 0xc00000, 0x7c00000, 0x7c00000, 
0xc00000, 0xc00000, 0x3fc00000, 0x3f800000, 
0xc1800000, 0xc1800000, 0xc1800000, 0xc1800000, 
0xc1800000, 0xffc00000, 0x7fc00000, 0x1800000, 
0x1800000, 0xffc00000, 0xffc00000, 0xc0000000, 
0xff800000, 0x7fc00000, 0xc00000, 0xc00000, 
0xffc00000, 0xff800000, 0x7f800000, 0xff800000, 
0xc0000000, 0xff800000, 0xffc00000, 0xc0c00000, 
0xc0c00000, 0xffc00000, 0x7f800000, 0xffc00000, 
0xffc00000, 0x81c00000, 0x3800000, 0x7000000, 
0xe000000, 0x1c000000, 0x18000000, 0x18000000, 
0x7f800000, 0xffc00000, 0xc0c00000, 0xffc00000, 
0x7f800000, 0xc0c00000, 0xc0c00000, 0xffc00000, 
0x7f800000, 0x7f800000, 0xffc00000, 0xc0c00000, 
0xc0c00000, 0xffc00000, 0x7fc00000, 0xc00000, 
0x7fc00000, 0x7f800000, 0x0, 0x0, 
0x0, 0x0, 0x0,
0x0, 0x0, 0x18000000, 0x18000000
};


static const unsigned int xkuty_bw [] = 
{ 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0xff00, 0x0, 0x0, 0x0, 
0x7ffe0, 0x0, 0x0, 0x60000000, 
0x7fff0, 0x0, 0x0, 0xf0000000, 
0x3fffc, 0x0, 0x0, 0xf0000000, 
0x61fffe, 0x0, 0x0, 0xf0000000, 
0xf001ff, 0x700e0c, 0x3830061, 0xff1e00f0, 
0x1f8007f, 0x80f81f1e, 0x7c780f3, 0xff9f01f0, 
0x1f8001f, 0x807c3e1e, 0xf8780f3, 0xff8f01e0, 
0x3f0000f, 0xc07c3e1e, 0x1f0780f1, 0xff0f83e0, 
0x7e03c07, 0xe03e7c1e, 0x3e0780f0, 0xf00f83e0, 
0x7e03f07, 0xe01e781e, 0x7c0780f0, 0xf00783c0, 
0x7c31f83, 0xe01ff81e, 0xf80780f0, 0xf007c7c0, 
0xfc38783, 0xf00ff01f, 0xf80780f0, 0xf003c7c0, 
0xf8783c1, 0xf007e01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfe0780f0, 0xf001ff80, 
0xf8701c1, 0xf01ff81f, 0xbf0781f0, 0xf001ff00, 
0xf8783c1, 0xf03e7c1f, 0x1f87c3f0, 0xf800ff00, 
0xf83c781, 0xf03c3c1e, 0xfc7fff0, 0xfcc0ff00, 
0xfc3ff83, 0xf07c3e1e, 0x7e7fff0, 0xffc0fe00, 
0x7c1ff03, 0xe0fc3f1e, 0x3e3fcf0, 0x7fc07e00, 
0x7e07c07, 0xe0781e0c, 0x1c1f860, 0x3f807e00, 
0x7e00007, 0xe0000000, 0x0, 0x3c00, 
0x3f0000f, 0xc0000000, 0x0, 0x3c00, 
0x3f8001f, 0x80020002, 0x2004120, 0x3c00, 
0x1fe007f, 0x80020002, 0x4020, 0x7800, 
0xff81ff, 0x3a1c77, 0x3a387124, 0x61c0f800, 
0x7ffffe, 0x4a2482, 0x22404928, 0x9207f000, 
0x3ffffc, 0xfa7c82, 0x22404930, 0xf3cfe000, 
0x1ffff0, 0x422082, 0x22404928, 0x804fc000, 
0x7ffe0, 0x399c71, 0xa2387124, 0x73878000, 
0xff00, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0
};

static const unsigned int xkuty2_bw [] = 
{ 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0xff00, 0x0, 0x0, 0x0, 
0x7ffe0, 0x0, 0x0, 0xf0000000, 
0x7fff0, 0x0, 0x0, 0xf0000000, 
0x3fffc, 0x0, 0x0, 0xf0000000, 
0x61fffe, 0x0, 0x0, 0xf0000000, 
0xf001ff, 0x781e1e, 0x3c780f3, 0xff9e00f0, 
0x1f8007f, 0x80781e1e, 0x7c780f3, 0xff9f01f0, 
0x1f8001f, 0x807c3e1e, 0xf8780f3, 0xff8f01e0, 
0x3f0000f, 0xc07c3e1e, 0x1f0780f3, 0xff8f83e0, 
0x7e07c07, 0xe03e7c1e, 0x3e0780f0, 0xf00f83e0, 
0x7e03f07, 0xe01e781e, 0x7c0780f0, 0xf00783c0, 
0x7c31f83, 0xe01ff81e, 0xf80780f0, 0xf007c7c0, 
0xfc38783, 0xf00ff01f, 0xf80780f0, 0xf003c7c0, 
0xf8783c1, 0xf007e01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfc0780f0, 0xf003ef80, 
0xf8701c1, 0xf00ff01f, 0xfe0780f0, 0xf001ff80, 
0xf8701c1, 0xf01ff81f, 0xbf0781f0, 0xf001ff00, 
0xf8783c1, 0xf03e7c1f, 0x1f87c3f0, 0xf800ff00, 
0xf83c781, 0xf03c3c1e, 0xfc7fff0, 0xfcc0ff00, 
0xfc3ff83, 0xf07c3e1e, 0x7e7fff0, 0xffc0fe00, 
0x7c1ff03, 0xe07c3e1e, 0x3e3fcf0, 0x7fc07e00, 
0x7e07c07, 0xe07c3e1e, 0x1e1f8f0, 0x3fc07e00, 
0x7e00007, 0xe0000000, 0x0, 0x3c00, 
0x3f0000f, 0xc0000000, 0x0, 0x3c00, 
0x3f8001f, 0x80002000, 0x40401048, 0x3c00, 
0x1fe007f, 0x80002000, 0x40001008, 0x7800, 
0xff81ff, 0x3a1ce, 0xe74e1c49, 0x31c0f800, 
0x7ffffe, 0x4a250, 0x4450124a, 0x4a0ff000, 
0x3ffffc, 0xfa7d0, 0x4450124c, 0x7bcfe000, 
0x1ffff0, 0x42210, 0x4450124a, 0x404fc000, 
0x7ffe0, 0x399ce, 0x344e1c49, 0x3b8f8000, 
0xff00, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0
};

static const unsigned int exos_bw [] = 
{ 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x80, 0x40000000, 
0x0, 0x0, 0x80, 0x40000000, 
0x0, 0x0, 0x80, 0x40000000, 
0xb807, 0x811103c1, 0x60f00e80, 0x5c104000, 
0xc408, 0x41110421, 0x81081180, 0x62088000, 
0x8210, 0x21290811, 0x2042080, 0x41088000, 
0x8210, 0x21290811, 0x2042080, 0x41088000, 
0x8210, 0x20aa0ff1, 0x3fc2080, 0x41050000, 
0x8210, 0x20aa0801, 0x2002080, 0x41050000, 
0x8210, 0x20aa0811, 0x2042080, 0x41050000, 
0xc408, 0x40440421, 0x1081180, 0x62050000, 
0xb807, 0x804403c1, 0xf00e80, 0x5c020000, 
0x8000, 0x0, 0x0, 0x20000, 
0x8000, 0x0, 0x0, 0x40000, 
0x8000, 0x0, 0x0, 0x180000, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0xff, 0xffc00003, 0xffff0000, 0x7f800000, 
0x3ff, 0xfff0000f, 0xffffc001, 0xffe00000, 
0x7ff, 0xfffc003f, 0xffffe003, 0xfff80000, 
0xfff, 0xfffe007f, 0xfffff007, 0xfffc0000, 
0x1fff, 0xffff00ff, 0xfffff80f, 0xfffc0000, 
0x3f80, 0x3f81fc, 0x3f81fc1f, 0xc0780000, 
0x7e00, 0x1fc3f8, 0x7f00fe3f, 0x80300000, 
0x7c00, 0xfe7f0, 0x7e007e3f, 0x0, 
0xf800, 0x7ffe0, 0xfc003f7e, 0x0, 
0xf800, 0x3ffc0, 0xf8001f7e, 0x0, 
0xffff, 0xfe01ff80, 0xf8001f7f, 0xffff8000, 
0xffff, 0xfc00ff00, 0xf8001f7f, 0xffff8000, 
0xffff, 0xf800ff00, 0xf8001f7f, 0xffff8000, 
0xffff, 0xf001ff80, 0xf8001f3f, 0xffff8000, 
0xf800, 0x3ffc0, 0xf8001f00, 0x1f8000, 
0xf800, 0x7ffe0, 0xfc003f00, 0x1f8000, 
0x7c00, 0xfe7f0, 0x7e007e00, 0x1f8000, 
0x7e00, 0x1fc3f8, 0x7f00fe00, 0x3f8000, 
0x3f80, 0x3f81fc, 0x3f81fc00, 0x7f0000, 
0x1fff, 0xffff00ff, 0xffffffff, 0xffff0000, 
0xfff, 0xfffe007f, 0xffffffff, 0xfffe0000, 
0x7ff, 0xfffc003f, 0xffffffff, 0xfffc0000, 
0x3ff, 0xfff0000f, 0xffffffff, 0xfff80000, 
0xff, 0xffc00003, 0xffffffff, 0xffe00000, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0
};

static unsigned int _dummy_mask[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

static MONO_SPR _font_spr_big   = { 24, 21, _bmp_nums_speed, _dummy_mask, 1,0};
static MONO_SPR _font_spr_small = { 12, 9, _bmp_nums_distance, _dummy_mask, 1,0};

static MONO_SPR _km_spr =  { 15, 7, _bmp_km, _dummy_mask, 1,0};
static MONO_SPR _mi_spr =  { 21, 10, _bmp_mi, _dummy_mask, 1,0};
static MONO_SPR _kmh_spr =  { 23, 7, _bmp_kmh, _dummy_mask, 1,0};
static MONO_SPR _mph_spr =  { 21, 10, _bmp_mph, _dummy_mask, 1,0};
static MONO_SPR _lock_spr =  { 30,25, _bmp_lock, _dummy_mask, 1,0};
static MONO_SPR _warning_spr =  { 24,19, _bmp_warning, _dummy_mask, 1,0};
static MONO_SPR _fatal_error_spr =  { 20,17, _bmp_fatal_error, _dummy_mask, 1,0};
static MONO_SPR _cruisin_spr =  { 24,19, _bmp_cruisin, _dummy_mask, 1,0};
static MONO_SPR _battery_full  = { 27, 59, _bmp_battery, _dummy_mask, 1,0};
static MONO_SPR _battery_empty = { 27, 59, _bmp_battery_empty, _dummy_mask, 1,0};

#endif // XCUTY_GFX_H




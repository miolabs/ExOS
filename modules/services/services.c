
#include "services.h"
#include "bonjour/bonjour.h"

void __services__init()
{
    bonjour_init();
}
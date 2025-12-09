#ifndef GETTEMP_H
#define GETTEMP_H

#include <OneWire.h>

extern OneWire ds;   // Declare, not define so that its only used once
float getTemp();
#endif

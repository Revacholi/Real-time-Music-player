#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "Print.h"

void print(Serial *ser, char *format, ...) {
  	char printBuffer[1280];
	va_list args;
	va_start(args, format);
	vsnprintf(printBuffer, sizeof(printBuffer), format, args);
    va_end(args);
    SCI_WRITE(ser, printBuffer);

}
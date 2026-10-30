#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>

#define SAVECOUNT 6
#define SAVEHEADERSIZE 18

#define SAVE_FREE 0
#define SAVE_OCCUPIED 1
#define SAVE_INVALID 2

uint8_t classifySaveHeader( const uint8_t* header );
const char* saveTitle( uint8_t* header );

#endif

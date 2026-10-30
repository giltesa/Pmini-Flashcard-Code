#include "save.h"


uint8_t classifySaveHeader( const uint8_t* header ) {
  uint8_t i, nonzero = 0;
  uint16_t sum = 0;

  // Conventional identity: four code bytes and twelve title bytes.
  for ( i = 0; i < 16; ++i ) {
    nonzero |= header[ i ];
    sum += header[ i ];
  }

  if ( !nonzero && !header[ 16 ] && !header[ 17 ] ) {
    return SAVE_FREE;
  }

  if ( nonzero && sum == ( (uint16_t) header[ 16 ] | ( (uint16_t) header[ 17 ] << 8 ) ) ) {
    return SAVE_OCCUPIED;
  }

  return SAVE_INVALID;
}


static uint8_t sanitizeTitle( uint8_t* text, uint8_t length ) {
  uint8_t i, c, useful = 0;

  for ( i = 0; i < length; ++i ) {
    c = text[ i ];
    if ( c >= 'a' && c <= 'z' ) {
      c -= 'a' - 'A';
    }

    if ( ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ) {
      useful = 1;
    } else if ( !c ) {
      c = ' ';
    } else if ( c < ' ' || c > '~' ) {
      c = '?';
    }
    text[ i ] = c;
  }

  // The caller provides one extra byte, after the fixed field.
  while ( length && text[ length - 1 ] == ' ' ) {
    --length;
  }
  text[ length ] = 0;
  return useful;
}


const char* saveTitle( uint8_t* header ) {
  // Only call after classification: this overwrites the stored checksum.
  if ( sanitizeTitle( header + 4, 12 ) ) {
    return (const char*) ( header + 4 );
  }

  if ( sanitizeTitle( header, 4 ) ) {
    return (const char*) header;
  }

  return "UNTITLED";
}

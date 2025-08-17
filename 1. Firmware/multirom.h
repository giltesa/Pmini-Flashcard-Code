#define NUM_GAMES 20

const uint8_t rom[ NUM_GAMES * 524288 ] __attribute__ ((section(".romStorage"))) = {
  'R', 'O', 'M', 'S', 'T', 'A', 'R', 'T'
};

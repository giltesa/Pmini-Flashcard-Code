#define NUM_GAMES 2
#define GAME_SIZE 524288

const uint8_t rom[ NUM_GAMES * GAME_SIZE ] __attribute__ ((section(".romStorage"))) = {
    'R','O','M','S','T','A','R','T',
    // The rest is automatically initialized to zero by the compiler
};
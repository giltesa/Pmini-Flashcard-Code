#include "pico/stdlib.h"

#include "rom.h"

#include "hardware/clocks.h"

#include "hardware/vreg.h"

#include "hardware/pio.h"
#include "hardware/dma.h"

#include "oe.pio.h"
#include "pushData.pio.h"
#include "lale.pio.h"
#include "hale.pio.h"


// We don't use the Flash cache.
#define XIP_CACHE   0x10000000
#define XIP_NOCACHE 0x13000000
#define XIP_NOCACHE_OFFSET (XIP_NOCACHE - XIP_CACHE)

// Pin Definitions.
#define A0A10 0
#define A1A11 1
#define A2A12 2
#define A3A13 3
#define A4A14 4
#define A5A15 5
#define A6A16 6
#define A7A17 7
#define A8A18 8
#define A9A19 9
#define A20 10

#define D0 17
#define D1 18
#define D2 19
#define D3 20
#define D4 21
#define D5 22
#define D6 23
#define D7 24

#define HALE 11
#define LALE 12
#define WE 13
#define OE 14
#define CS 15

void __not_in_flash_func( doPIOStuff() ) {
  // Set up PIOs.
  
  // OE toggle program.
  PIO pio = pio0;
  uint sm_oe = pio_claim_unused_sm( pio, false );
  uint offset_oe = pio_add_program( pio, &oe_toggle_program );
  
  // Push byte out.
  uint sm_pushData = pio_claim_unused_sm( pio, false );
  uint offset_pushData = pio_add_program( pio, &push_databits_program );
  
  // HALE latching.
  uint sm_hale = pio_claim_unused_sm( pio, false );
  uint offset_hale = pio_add_program( pio, &hale_latch_program );
  
  // LALE latching.
  uint sm_lale = pio_claim_unused_sm( pio, false );
  uint offset_lale = pio_add_program( pio, &lale_latch_program );
  
  
  // Create DMAs.
  int hale_dma = dma_claim_unused_channel( true );
  int lale_addr_dma = dma_claim_unused_channel( true );
  int data_dma = dma_claim_unused_channel( true );
  
  
  // Move high address to LALE SM.
  dma_channel_config c = dma_channel_get_default_config( hale_dma );

  channel_config_set_transfer_data_size( &c, DMA_SIZE_32 );
  channel_config_set_read_increment( &c, false );
  channel_config_set_write_increment( &c, false );
  channel_config_set_dreq( &c, pio_get_dreq( pio, sm_hale, false) );

  dma_channel_configure(
    hale_dma,
    &c,
    &pio->txf[ sm_lale ], // Write to the LALE SM
    &pio->rxf[ sm_hale ],  // Read from HALE RX FIFO
    1,                                          // Halt after each read
    false                                       // Don't start yet
  );
  
  // Move the adress from LALE SM to the third DMA channel.
  c = dma_channel_get_default_config( lale_addr_dma );

  channel_config_set_transfer_data_size( &c, DMA_SIZE_32 );
  channel_config_set_read_increment( &c, false );
  channel_config_set_write_increment( &c, false );
  channel_config_set_dreq( &c, pio_get_dreq( pio, sm_lale, false) );
  
  channel_config_set_chain_to( &c, hale_dma );     // Trigger the HALE channel again when done
  
  

  dma_channel_configure(
    lale_addr_dma,
    &c,
    &dma_hw->ch[ data_dma ].al3_read_addr_trig, // Write to READ_ADDR_TRIG of data channel
    &pio->rxf[ sm_lale ], // Read from LALE RX FIFO
    1,                                          // Halt after each read
    false                                       // Don't start yet
  );
  
  
  // Read the actual data.
  c = dma_channel_get_default_config( data_dma );

  channel_config_set_transfer_data_size( &c, DMA_SIZE_8 );
  channel_config_set_read_increment( &c, false );
  channel_config_set_write_increment( &c, false );
  channel_config_set_chain_to( &c, lale_addr_dma );     // Trigger the LALE channel again when done
  
  // Set to high priority.
  channel_config_set_high_priority( &c, true );

  dma_channel_configure(
    data_dma,
    &c,
    &pio->txf[ sm_pushData ], // Write to the byte push SM
    &rom[0], // Read from ROM array (will be overwritten)
    1,                                          // Halt after each read
    false                                       // Don't start yet
  );
  
  // Start the SMs.
  oe_toggle_program_init( pio, sm_oe, offset_oe, D0, OE );
  push_databits_program_init( pio, sm_pushData, offset_pushData, D0 );
  hale_latch_program_init( pio, sm_hale, offset_hale, A0A10, HALE );
  lale_latch_program_init( pio, sm_lale, offset_lale, A0A10, LALE );
  
  // Push the base address of the array.
  pio_sm_put( pio, sm_lale, ( ( (uint32_t) rom + XIP_NOCACHE_OFFSET ) ) >> 20 );

  
  // Start the DMA channels.
  dma_start_channel_mask( 1u << hale_dma );
  dma_start_channel_mask( 1u << lale_addr_dma );

  
  // Do nothing.
  while ( 1 ) {
    tight_loop_contents();
  }
}

int main() {
  
  // Set higher freq.
  sleep_ms(2);
  vreg_set_voltage(VREG_VOLTAGE_1_30);
  sleep_ms(2);
  set_sys_clock_khz(240000, true);
  
  doPIOStuff();
  
  return 0;
}

#ifndef BIT_READER_H
#define BIT_READER_H

#include "flac_decoder.h"
#include <stdint.h>

typedef struct {
  uint32_t acumulator;
  uint8_t bit_counter;
  flac_read_cb_t read_cb;
  void *user_data;

} bit_reader_t;

void bit_reader_init(bit_reader_t *br, flac_read_cb_t flac_read_cb,
                     void *user_data);

uint32_t bit_reader_read_bits(bit_reader_t *br, uint8_t num_bits);
uint8_t bit_reader_unary_decoding(bit_reader_t *br);
uint64_t bit_reader_read_utf8(bit_reader_t *br);
void bit_reader_skip_bits(bit_reader_t *br, uint8_t num);
void bit_reader_align(bit_reader_t *br);
#endif

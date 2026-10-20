#include "../include/bit_reader.h"
#include <stdint.h>

void bit_reader_init(bit_reader_t *br, flac_read_cb_t flac_read_cb,
                     void *user_data) {
  br->acumulator = 0;
  br->bit_counter = 0;
  br->read_cb = flac_read_cb;
  br->user_data = user_data;
}

uint32_t bit_reader_read_bits(bit_reader_t *br, uint8_t num_bits) {
  while (br->bit_counter < num_bits) {
    uint8_t byte = 0;
    br->read_cb(&byte, 1, br->user_data);
    br->acumulator <<= 8;
    br->acumulator |= byte;
    br->bit_counter += 8;
  }
  uint8_t shift_amount = br->bit_counter - num_bits;
  uint64_t result = (br->acumulator >> shift_amount) & ((1ULL << num_bits) - 1);
  br->bit_counter -= num_bits;
  return result;
}

uint8_t bit_reader_unary_decoding(bit_reader_t *br) {
  uint8_t count = 0;
  while (true) {
    if (bit_reader_read_bits(br, 1) == 0)
      count++;
    else
      return count;
  }
}

uint64_t bit_reader_read_utf8(bit_reader_t *br) {}

void bit_reader_skip_bits(bit_reader_t *br, uint8_t num) {
  while (num > 32) {
    bit_reader_read_bits(br, 32);
    num -= 32;
  }
  bit_reader_read_bits(br, num);
}

void bit_reader_align(bit_reader_t *br) {
  uint8_t x = br->bit_counter & 8;
  if (x > 0) {
    bit_reader_read_bits(br, x);
  }
}

#include "../include/bit_reader.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t mock_file[] = {0xAC, 0x04, 0xC3, 0xAA};
static size_t mock_file_pos = 0;

size_t mock_read_cb(uint8_t *buffer, size_t bytes_to_read, void *user_data) {
  size_t bytes_read = 0;
  while (bytes_read < bytes_to_read && mock_file_pos < sizeof(mock_file)) {
    buffer[bytes_read] = mock_file[mock_file_pos];
    mock_file_pos++;
    bytes_read++;
  }
  return bytes_read;
}

int main() {
  printf("Rozpoczynam testy bit_reader...\n");

  bit_reader_t br;
  bit_reader_init(&br, mock_read_cb, NULL);

  uint32_t val1 = bit_reader_read_bits(&br, 4);
  assert(val1 == 10);

  uint32_t val2 = bit_reader_read_bits(&br, 4);
  assert(val2 == 12);

  uint8_t unary_val = bit_reader_unary_decoding(&br);
  assert(unary_val == 5);

  bit_reader_align(&br);

  uint64_t utf8_val = bit_reader_read_utf8(&br);
  assert(utf8_val == 234);

  printf("Wszystkie testy ZALICZONE!\n");
  return 0;
}

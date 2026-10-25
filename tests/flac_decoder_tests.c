#include "../include/flac_decoder.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t mock_file[] = {
    // flac
    0x66, 0x4C, 0x61, 0x43,

    // 2.STREAMINFO: is_last=0, typ=0, length=34
    0x00, 0x00, 0x00, 0x22,

    // 3. STREAMINFO elements
    0x00, 0x10, 0x00, 0x10,             // min/max block size (16 i 16)
    0x00, 0x00, 0x17, 0x00, 0x00, 0x44, // min/max frame size (23 i 68)
    0x0A, 0xC4, 0x42, 0xF0, 0x00, 0x00, 0x00,
    0x13, // sample rate 44100, 2 ch, 16 bps, total samples
    // MD5
    0xD5, 0xB0, 0x56, 0x49, 0x75, 0xE9, 0x8B, 0x8D, 0x8B, 0x93, 0x04, 0x22,
    0x75, 0x7B, 0x81, 0x03,

    // 4.Next metadata block
    // Zaczynamy bajtem 0x84 (binarnie 10000100) -> is_last_block = 1, type =
    // Długość ustawiamy na 0, żeby pętla od razu zakończyła odczyt
    0x84, 0x00, 0x00, 0x00};
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
  printf("Beggining flac_decoder_init test...\n");

  flac_decoder_t *decoder = flac_decoder_create(mock_read_cb, NULL);
  assert(decoder != NULL);

  flac_info_t info = {0};

  int result = flac_decoder_init(decoder, &info);

  assert(result == 0);
  assert(info.sample_rate == 44100);
  assert(info.channels == 2);
  assert(info.bits_per_sample == 16);
  assert(info.min_block_size == 16);
  assert(info.max_block_size == 16);

  printf("All good!\n");

  flac_decoder_destroy(decoder);
  return 0;
}

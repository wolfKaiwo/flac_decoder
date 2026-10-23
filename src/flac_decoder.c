#include "../include/flac_decoder.h"
#include "../include/bit_reader.h"
#include <stdio.h>
#include <stdlib.h>

struct flac_decoder_t {
  bit_reader_t bit_reader;
};

flac_decoder_t *flac_decoder_create(flac_read_cb_t read_cb, void *user_data) {
  flac_decoder_t *decoder = (flac_decoder_t *)malloc(sizeof(flac_decoder_t));
  if (!decoder)
    return NULL;

  bit_reader_init(&decoder->bit_reader, read_cb, user_data);
  return decoder;
}

int flac_decoder_init(flac_decoder_t *decoder, flac_info_t *info) {
  if (!info)
    return -1;
  uint32_t marker_f = bit_reader_read_bits(&decoder->bit_reader, 8);
  uint32_t marker_l = bit_reader_read_bits(&decoder->bit_reader, 8);
  uint32_t marker_a = bit_reader_read_bits(&decoder->bit_reader, 8);
  uint32_t marker_c = bit_reader_read_bits(&decoder->bit_reader, 8);

  if (marker_f != 0x66 || marker_l != 0x4C || marker_a != 0x61 ||
      marker_c != 0x43) {
    printf("ERROR flac_decoder_init : This is not a FLAC file\n");
    return -1;
  }
  printf("FLAC code was foudn!\n");

  uint8_t is_last_block = bit_reader_read_bits(&decoder->bit_reader, 1);
  uint8_t block_type = bit_reader_read_bits(&decoder->bit_reader, 7);
  uint32_t block_length = bit_reader_read_bits(&decoder->bit_reader, 24);

  // 0 is the STREAMINFO block type
  if (block_type != 0) {
    printf("ERROR flac_decoder_init: First metadata block must be of type "
           "STREAMINFO\n");
    return -1;
  }
  info->min_block_size = bit_reader_read_bits(&decoder->bit_reader, 16);
  info->max_block_size = bit_reader_read_bits(&decoder->bit_reader, 16);

  info->min_frame_size = bit_reader_read_bits(&decoder->bit_reader, 24);
  info->max_frame_size = bit_reader_read_bits(&decoder->bit_reader, 24);
  info->sample_rate = bit_reader_read_bits(&decoder->bit_reader, 20);
  info->channels = bit_reader_read_bits(&decoder->bit_reader, 3) + 1;
  info->bits_per_sample = bit_reader_read_bits(&decoder->bit_reader, 5) + 1;

  // FOR NOW skipping the total numer of intechannel samples and MD5 checksum
  bit_reader_skip_bits(&decoder->bit_reader, 36);
  bit_reader_skip_bits(&decoder->bit_reader, 128);

  printf("Initialised: %lu Hz, %d channel(s), %d bit/sample\n",
         (unsigned long)info->sample_rate, info->channels,
         info->bits_per_sample);
  while (!is_last_block) { // Odczytujemy nagłówek kolejnego bloku
    is_last_block = bit_reader_read_bits(&decoder->bit_reader, 1);
    uint32_t current_block_type = bit_reader_read_bits(&decoder->bit_reader, 7);
    uint32_t current_block_length =
        bit_reader_read_bits(&decoder->bit_reader, 24);

    switch (current_block_type) {
    case 4: // VORBIS_COMMENT[cite: 2]
      printf("TODO: Gonna read tags here( title, artist)  - %u bytes",
             current_block_length);
      break;
    case 6: // PICTURE[cite: 2]
      printf("TODO: gonna read album cover here - %u bytes",
             current_block_length);
      break;
    default:
      printf("Skipping block of type: %u of size: %u bytes", current_block_type,
             current_block_length);
      break;
    }
    bit_reader_skip_bits(&decoder->bit_reader, current_block_length * 8);
  }
  printf("Metadata finished, Decdoer is ready for audio frames");
  return 0;
}

void flac_decoder_destroy(flac_decoder_t *decoder) {
  if (decoder) {
    free(decoder);
  }
}

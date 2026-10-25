#include "../include/flac_decoder.h"
#include "../include/bit_reader.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct flac_decoder_t {
  bit_reader_t bit_reader;
};

typedef enum {
  FLAC_CHANNEL_INDEPENDENT = 0, // all channels are coded independently
  FLAC_CHANNEL_LEFT_SIDE = 8,   // 2 channels: stereo left-side
  FLAC_CHANNEL_SIDE_RIGHT = 9,  // 2 channels: stereo side-right
  FLAC_CHANNEL_MID_SIDE = 10    // 2 channels: stereo mid-side
} flac_channel_assignment_t;

typedef struct {
  bool is_variable_block_size;
  bool frame_header_found;
  uint32_t frame_header_block_size;
  uint8_t uncommon_block_size;
  uint32_t frame_sample_rate;
  uint8_t uncommon_sample_rate;
  uint8_t channel;
  flac_channel_assignment_t channel_assignment;
} flac_frame_header_t;

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
  if (info->min_block_size < 16 || info->min_block_size > 65535) {
    printf("ERROR flac_decoder_init: min_block_size outside of range");
    return -1;
  }
  info->max_block_size = bit_reader_read_bits(&decoder->bit_reader, 16);
  if (info->max_block_size < 16 || info->max_block_size > 65535) {
    printf("ERROR flac_decoder_init: max_block_size outside of range");
    return -1;
  }
  if (info->min_block_size > info->max_block_size) {
    printf("ERROR flace_decoder_init: min_block_size > max_block_size");
    return -1;
  }
  info->min_frame_size = bit_reader_read_bits(&decoder->bit_reader, 24);
  info->max_frame_size = bit_reader_read_bits(&decoder->bit_reader, 24);
  info->sample_rate = bit_reader_read_bits(&decoder->bit_reader, 20);
  info->channels = bit_reader_read_bits(&decoder->bit_reader, 3) + 1;
  info->bits_per_sample = bit_reader_read_bits(&decoder->bit_reader, 5) + 1;

  // FOR NOW skipping the total numer of intechannel samples and MD5 checksum
  // bit_reader_skip_bits(&decoder->bit_reader, 36);
  uint64_t upper_bits = (uint64_t)bit_reader_read_bits(&decoder->bit_reader, 4);
  uint32_t lower_bits = bit_reader_read_bits(&decoder->bit_reader, 32);
  info->total_samples = (upper_bits << 32) | lower_bits;

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
    case 4: // VORBIS_COMMENT
      printf("TODO: Gonna read tags here( title, artist)  - %u bytes\n",
             current_block_length);
      break;
    case 6: // PICTURE
      printf("TODO: gonna read album cover here - %u bytes\n",
             current_block_length);
      break;
    default:
      printf("Skipping block of type: %u of size: %u bytes\n",
             current_block_type, current_block_length);
      break;
    }
    bit_reader_skip_bits(&decoder->bit_reader, current_block_length * 8);
  }
  printf("Metadata finished, Decdoer is ready for audio frames\n");
  return 0;
}

static int read_frame_header(flac_decoder_t *decoder, flac_frame_header_t *fh,
                             flac_info_t *info) {

  bit_reader_align(&decoder->bit_reader);
  uint8_t first_byte = bit_reader_read_bits(&decoder->bit_reader, 8);
  while (!fh->frame_header_found) {
    if (first_byte == 0xFF) {
      uint8_t second_byte = bit_reader_read_bits(&decoder->bit_reader, 8);
      while (second_byte == 0xFF) {
        second_byte = bit_reader_read_bits(&decoder->bit_reader, 8);
      }
      if (second_byte == 0xF9) {
        fh->is_variable_block_size = true;
        fh->frame_header_found = true;
      } else if (second_byte == 0xF8) {
        fh->is_variable_block_size = false;
        fh->frame_header_found = true;
      }
    }
  }

  uint8_t block_size_bits = bit_reader_read_bits(&decoder->bit_reader, 4);
  switch (block_size_bits) {
  case 0b0000:
    printf("ERROR read_frame_header: block_size_bits cant be 0");
    return -1;
    break;
  case 0b0001:
    fh->frame_header_block_size = 192;
    break;
  case 0b0010 ... 0b0101:
    fh->frame_header_block_size = 144 * pow(2, block_size_bits);
    break;
  case 0b0110:
    fh->uncommon_block_size = 8;
    break;
  case 0b0111:
    fh->uncommon_block_size = 16;
    break;
  case 0b1000 ... 0b1111:
    fh->frame_header_block_size = pow(2, block_size_bits);
    break;
  }

  uint8_t sample_rate_bits = bit_reader_read_bits(&decoder->bit_reader, 4);
  switch (sample_rate_bits) {
  case 0b0000:
    fh->frame_sample_rate = info->sample_rate;
    break;
  case 0b0001:
    fh->frame_sample_rate = 88200;
    break;
  case 0b0010:
    fh->frame_sample_rate = 176400;
    break;
  case 0b0011:
    fh->frame_sample_rate = 192000;
    break;
  case 0b0100:
    fh->frame_sample_rate = 8000;
    break;
  case 0b0101:
    fh->frame_sample_rate = 16000;
    break;
  case 0b0110:
    fh->frame_sample_rate = 22050;
    break;
  case 0b0111:
    fh->frame_sample_rate = 24000;
    break;
  case 0b1000:
    fh->frame_sample_rate = 32000;
    break;
  case 0b1001:
    fh->frame_sample_rate = 44100;
    break;
  case 0b1010:
    fh->frame_sample_rate = 48000;
    break;
  case 0b1011:
    fh->frame_sample_rate = 96000;
    break;
  case 0b1100:
    fh->uncommon_sample_rate = 8;
    break;
  case 0b1101:
    fh->uncommon_sample_rate = 16;
    break;
  case 0b1110:
    fh->uncommon_sample_rate = 17;
    break;
  case 0b1111:
    printf("ERROR read_frame_header: sample_rate_bits cant be 15");
  }
  uint8_t channel_bits = bit_reader_read_bits(&decoder->bit_reader, 4);
  switch (channel_bits) {
  case 0b0000 ... 0b0111:
    fh->channel = channel_bits + 1;
    fh->channel_assignment = FLAC_CHANNEL_INDEPENDENT;
    break;
  case 0b1000 ... 0b1010:
    fh->channel = 2;
    fh->channel_assignment = (flac_channel_assignment_t)channel_bits;
    break;
  case 0b1011 ... 0b1111:
    printf("ERROR read_frame_header: reead reserved value for channel_bits");
    break;
  }
}

int flac_decoder_read_frame(flac_decoder_t *decoder, int32_t *output_buffer,
                            flac_info_t *info) {
  flac_frame_header_t fh = {0};
  if (read_frame_header(decoder, &fh, info) != 0) {
    return -1;
  }
}

void flac_decoder_destroy(flac_decoder_t *decoder) {
  if (decoder) {
    free(decoder);
  }
}

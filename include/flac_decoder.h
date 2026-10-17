#ifndef FLAC_DECODER_H
#define FLAC_DECODER_H

#include <cstdint>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t sample_rate;
  uint8_t channels;
  uint8_t bits_per_sample;
  uint32_t total_samples;
  uint16_t min_block_size;
  uint16_t max_block_size;
} flac_info_t;

// typ funkcji zwrotnej, którą reszta programu dostarczy do czytania bajtów
// zwraca ilość faktycznie odczytanych bajtów
typedef size_t (*flac_read_cb_t)(uint8_t *buffer, size_t bytes_to_read,
                                 void *user_data);

// główny uchwyt dekodera
typedef struct flac_decoder_t flac_decoder_t;

// API

// 1. Tworzy instancje ekodera i przypisuje funkcje czytającą 
flac_decoder_t *flac_decoder_create(flac_read_cb_t read_cb, void *user_data);

// 2. Czyta nagłówki, weryfikuje sygnaturę fLaC i wypełnia struct flac_info_t
// zwraca 0 w przypadku sukcesu (< 0 to porażka i samobojstwo)
int flac_decoder_init(flac_decoder_t *decoder, flac_info_t *info);

// 3. Dekoduje jeden blok audio do dostarczonego bufora
// bufor musi być wystarczająco duży (max_block_size z STREAMINFO * ilość
// kanałów) zwracaa ilość odczytanych sampli na kanał abo zwraca błąd(< 0)
int flac_decoder_read_frame(flac_decoder_t *decoder, int32_t *output_buffer);

// 4. Zwalnia pamięć zajmowaną przez dekoder
void flac_decoder_destroy(flac_decoder_t *decoder);

#endif

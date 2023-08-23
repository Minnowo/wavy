
#include "wav.h"

#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int decode_wav_chunks(const uint8_t *buf, const size_t buf_size,

                             WAV_CHUNK *fmt_chunk, WAV_CHUNK *fact_chunk,
                             WAV_CHUNK *cue_chunk, WAV_CHUNK *playlist_chunk,
                             WAV_CHUNK *assoc_data_list_chunk,
                             WAV_CHUNK *data_chunk) {

  uint8_t *p = buf;

  if (read32be(p) != RIFF_CHUNK_ID) {

#ifdef DEBUG_WAV
    printf("Not a valid RIFF container\n");
#endif
    return 0;
  }

  p += 8;

  /*
      p += 4;

      int wav_file_size = read32be(p);

      p += 4;
  */

  if (read32be(p) != WAVE_CHUNK_ID) {

#ifdef DEBUG_WAV
    printf("Could not find WAVE chunk\n");
#endif
    return 0;
  }

  p += 4;
  fmt_chunk->chunk_id = read32be(p);

  p += 4;
  fmt_chunk->chunk_size = read32le(p);

  p += 4;
  fmt_chunk->chunk_data = p;

  if (fmt_chunk->chunk_id != fmt_CHUNK_ID) {

#ifdef DEBUG_WAV
    printf("fmt chunk does not exist after WAVE chunk\n");
#endif
    return 0;
  }

  p += fmt_chunk->chunk_size;

  for (;;) {
    WAV_CHUNK chunk;
    WAV_CHUNK *c = NULL;

    if (p + 8 >= buf + buf_size) {
#ifdef DEBUG_WAV
      printf("Unexpected EOF\n");
#endif
      return 0;
    }

    chunk.chunk_id = read32be(p);
    p += 4;

    chunk.chunk_size = read32le(p);
    p += 4;

    if (p + chunk.chunk_size >= buf + buf_size) {

#ifdef DEBUG_WAV
      printf("Chunk %u is truncated, should be len %u", chunk.chunk_id,
             chunk.chunk_size);
#endif

      chunk.chunk_size = buf_size - (p - buf);

#ifdef DEBUG_WAV
      printf(" but it is actually %u\n", chunk.chunk_size);
#endif
    }

    chunk.chunk_data = p;
    p += chunk.chunk_size;

    switch (chunk.chunk_id) {

    case cue_CHUNK_ID:
      if (cue_chunk != NULL)
        c = cue_chunk;
      break;
    case LIST_CHUNK_ID:
      if (assoc_data_list_chunk != NULL)
        c = assoc_data_list_chunk;
      break;
    case fact_CHUNK_ID:
      if (fact_chunk != NULL)
        c = fact_chunk;
      break;
    case playlist_CHUNK_ID:
      if (playlist_chunk != NULL)
        c = playlist_chunk;
      break;
    case data_CHUNK_ID:
      if (data_chunk != NULL)
        c = data_chunk;
      break;
    }

    if (c) {

      c->chunk_id = chunk.chunk_id;
      c->chunk_size = chunk.chunk_size;
      c->chunk_data = chunk.chunk_data;
    }

    if (chunk.chunk_id == data_CHUNK_ID)
      break;
  }

  return 1;
}

uint64_t read_n_le(uint8_t *buf, size_t buf_size, size_t n) {

  uint64_t value = 0;
  size_t shift = 0;

  for (int i = 0; i < n; i++) {

    value |= buf[i] << shift;

    shift += 8;
  }

  return value;
}

void print_info(const uint8_t *buf, const size_t buf_size) {

  if (buf == NULL || buf_size <= 0) {
    printf("Buffer cannot be null or have 0 size!\n");
    return;
  }

  printf("Buffer size: %ld\n", buf_size);

  WAV_CHUNK fmt_chunk;
  WAV_CHUNK data_chunk;

  if (!decode_wav_chunks(buf, buf_size, &fmt_chunk, NULL, NULL, NULL, NULL,
                         &data_chunk))
    return;

  printf("Decoded format chunk:\n");

  printf("  Data length           : %d\n", fmt_chunk.chunk_size);

  // must be at least 16
  if (fmt_chunk.chunk_size < sizeof(FMT_CHUNK_COMMON)) {
    printf("format chunk data length does not match expected structure!\n");
    return;
  }

  FMT_CHUNK_COMMON *d = (FMT_CHUNK_COMMON *)fmt_chunk.chunk_data;

  printf("  chunk id              : %d\n", fmt_chunk.chunk_id);
  printf("  format tag            : %d\n", d->format_tag);
  printf("  num chanels           : %d\n", d->num_channels);
  printf("  sample rate           : %d\n", d->samples_per_second);
  printf("  avg bytes per second  : %d\n", d->avg_bytes_per_second);
  printf("  block align           : %d\n", d->block_align);
  printf("  bits per sample       : %d\n", d->bit_per_sample);

  switch (d->format_tag) {
  default:
    printf("  format tag is unknown!\n");
    break;

  case IBM_FORMAT_MULAW:
    printf("  format tag readable   : IBM_FORMAT_MULAW\n");
    break;
  case IBM_FORMAT_ALAW:
    printf("  format tag readable   : IBM_FORMAT_ALAW\n");
    break;
  case IBM_FORMAT_ADPCM:
    printf("  format tag readable   : IBM_FORMAT_ADPCM\n");
    break;
  case WAVE_FORMAT_PCM:
    printf("  format tag readable   : Microsoft WAVE_FORMAT_PCM\n");
    uint32_t a =
        d->num_channels * d->samples_per_second * (d->bit_per_sample / 8);

    if (d->avg_bytes_per_second != a) {
      printf("    The avg_bytes_per_second field is not correct!\n");
      printf("    It should be %u\n", a);
    }

    a = d->num_channels * (d->bit_per_sample / 8);

    if (d->block_align != a) {
      printf("    The block_align field is not correct!\n");
      printf("    It should be %u\n", a);
    }

    uint8_t *p = data_chunk.chunk_data;
    size_t i = 0;
    unsigned int bytes_per_sample_channel = ((d->bit_per_sample + 7) / 8);
    unsigned int bytes_per_sample = bytes_per_sample_channel * d->num_channels;
    
    int precision = 2000;
    
    const int WIDTH = 300;
    const int HEIGHT = 20;
    char screen[WIDTH][HEIGHT];

    for (int y = 0; y < HEIGHT; y++)
      for (int x = 0; x < WIDTH; x++) {
        screen[x][y] = ' ';
      }

    const uint64_t MAX = pow(2, d->bit_per_sample) ;
    
    
    printf("abs function test %d\n", abs(-5));
    
    for(int x = 0; x < WIDTH; x++) {
        
        int64_t max = 0;
        for(int n = 0; n < d->samples_per_second / precision; n++) {

        for (int channel = 0; channel < d->num_channels; channel++) {

          int64_t sample =
              read_n_le(&p[i], data_chunk.chunk_size, bytes_per_sample_channel);
          
          if(channel == 0 && sample > abs(max)) {

              max = sample;
          }

          i += bytes_per_sample_channel;

          if (i >= data_chunk.chunk_size) {
            return;
          }
        }
        }
        
        if(max == 0) {
            continue;
        }

        uint64_t scaled = ((double)max / (double)MAX) * HEIGHT;
        
        if(scaled >= HEIGHT) {
            scaled = HEIGHT ;
        }
        
        for(int y = 0; y < scaled; y++) {

            screen[x][y] = '#';
        }
    }

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {

            putchar(screen[x][HEIGHT- y - 1]);
        }
        printf("\n");
    }

    break;
  }
}




void visualize(FILE *file) {
    
    uint8_t buf[1024 * 1024];
    
    fread(buf, 1, sizeof(buf),file);

    WAV_CHUNK fmt_chunk;
    WAV_CHUNK data_chunk;

    if (!decode_wav_chunks(buf,sizeof(buf), &fmt_chunk, NULL, NULL, NULL, NULL,
                           &data_chunk))
    return;
}
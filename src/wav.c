
#include "wav.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int decode_wav_chunks(const uint8_t *buf, const size_t buf_size,

                      WAV_CHUNK *fmt_chunk, WAV_CHUNK *fact_chunk,
                      WAV_CHUNK *cue_chunk, WAV_CHUNK *playlist_chunk,
                      WAV_CHUNK *assoc_data_list_chunk, WAV_CHUNK *data_chunk) {

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
  fmt_chunk->chunk_size = read32be(p);

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

    chunk.chunk_size = read32be(p);
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

static void print_info_PCM(FMT_CHUNK_COMMON *d) {
    
}

void print_info(const uint8_t* buf, const size_t buf_size) {
    
    if(buf == NULL || buf_size <= 0) {
        printf("Buffer cannot be null or have 0 size!\n");
        return ;
    }
    
    printf("Buffer size: %ld\n", buf_size);
    
    WAV_CHUNK fmt_chunk;

    if (!decode_wav_chunks(buf, buf_size, &fmt_chunk, NULL, NULL, NULL, NULL, NULL)) 
        return;
    
    printf("Decoded format chunk:\n");
    
    printf("  Data length           : %d\n", fmt_chunk.chunk_size);

    if (fmt_chunk.chunk_size != sizeof(FMT_CHUNK_COMMON)) {
        printf("format chunk data length does not match expected structure!\n");
        return;
    }


    FMT_CHUNK_COMMON *d = (FMT_CHUNK_COMMON *)fmt_chunk.chunk_data;

    printf("  format tag            : %d\n", d->format_tag);
    printf("  num chanels           : %d\n", d->num_channels);
    printf("  sample rate           : %d\n", d->samples_per_second);
    printf("  avg bytes per second  : %d\n", d->avg_bytes_per_second);
    printf("  block align           : %d\n", d->block_align);

    switch (d->format_tag) {
        default:
        printf("  format tag is unknown!\n");
        break;

    case WAVE_FORMAT_PCM:
        printf("  format tag readable   : Microsoft WAVE_FORMAT_PCM\n");
        print_info_PCM(d);
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
    }

}



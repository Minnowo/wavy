

// https://www.aelius.com/njh/wavemetatools/doc/riffmci.pdf

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DEBUG_WAV

#define read32le(x) ((x)[0] | ((x)[1] << 8) | ((x)[2] << 16) | ((x)[3] << 24))
#define read24le(x) ((x)[0] | ((x)[1] << 8) | ((x)[2] << 16))
#define read16le(x) ((x)[0] | ((x)[1] << 8))

#define read32be(x) ((x)[3] | ((x)[2] << 8) | ((x)[1] << 16) | ((x)[0] << 24))
#define read24be(x) ((x)[2] | ((x)[1] << 8) | ((x)[0] << 16))
#define read16be(x) ((x)[1] | ((x)[0] << 8))


#define _read32be(w, x, y, z) ((z) | ((y) << 8) | ((x) << 16) | ((w) << 24))

enum {

  RIFF_CHUNK_ID = _read32be('R', 'I', 'F', 'F'),
  WAVE_CHUNK_ID = _read32be('W', 'A', 'V', 'E'),
  LIST_CHUNK_ID = _read32be('L', 'I', 'S', 'T'),
  INFO_CHUNK_ID = _read32be('I', 'N', 'F', 'O'),
  fmt_CHUNK_ID = _read32be('f', 'm', 't', ' '),
  fact_CHUNK_ID = _read32be('f', 'a', 'c', 't'),
  cue_CHUNK_ID = _read32be('c', 'u', 'e', ' '),
  playlist_CHUNK_ID = _read32be('p', 'l', 's', 't'),
  data_CHUNK_ID = _read32be('d', 'a', 't', 'a')
};

typedef enum {
  WAVE_FORMAT_PCM = 0x0001,
  IBM_FORMAT_MULAW = 0x0101,
  IBM_FORMAT_ALAW = 0x0102,
  IBM_FORMAT_ADPCM = 0x0103,
} WAV_FORMAT_TAG;

typedef struct { 
    uint32_t chunk_id; 
    uint32_t chunk_size; 
    uint8_t* chunk_data; 

} WAV_CHUNK;



/*
 * This defines the <common-fields> chunk as part of <fmt-ck>
 */
typedef struct {
    uint16_t format_tag;
    uint16_t num_channels;
    uint32_t samples_per_second;
    uint32_t avg_bytes_per_second;
    uint16_t block_align;
    uint16_t bit_per_sample; // this is only bps if PCM, otherwise it's a size

} FMT_CHUNK_COMMON;



void print_info(const uint8_t* buf, const size_t buf_size);

void visualize(FILE* file);
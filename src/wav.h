

// https://www.aelius.com/njh/wavemetatools/doc/riffmci.pdf

#include <stddef.h>
#include <stdint.h>

#define DEBUG_WAV

#define read32be(x) ((x)[0] | ((x)[1] << 8) | ((x)[2] << 16) | ((x)[3] << 24))
#define read24be(x) ((x)[0] | ((x)[1] << 8) | ((x)[2] << 16))
#define read16be(x) ((x)[0] | ((x)[1] << 8))



enum {

//   RIFF_CHUNK_ID = read32be("RIFF"),
  RIFF_CHUNK_ID = 1179011410,

  // WAVE_CHUNK_ID = read32be("WAVE"),
  WAVE_CHUNK_ID = 1163280727,

  // LIST_CHUNK_ID = read32be("LIST"),
  LIST_CHUNK_ID = 1414744396,

  //   INFO_CHUNK_ID = read32be("INFO"),
  INFO_CHUNK_ID = 1330007625,

  //   fmt_CHUNK_ID = read32be("fmt "),
  fmt_CHUNK_ID = 544501094,

  //   fact_CHUNK_ID = read32be("fact"),
  fact_CHUNK_ID = 1952670054,

  //   cue_CHUNK_ID = read32be("cue "),
  cue_CHUNK_ID = 543520099,

  //   playlist_CHUNK_ID = read32be("plst"),
  playlist_CHUNK_ID = 1953721456,

  //   data_CHUNK_ID = read32be("data"),
  data_CHUNK_ID = 1635017060
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

typedef struct {
    uint16_t format_tag;
    uint16_t num_channels;
    uint32_t samples_per_second;
    uint32_t avg_bytes_per_second;
    uint16_t block_align;

} FMT_CHUNK_COMMON;

void print_info(const uint8_t* buf, const size_t buf_size);

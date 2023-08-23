
#include "wav.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int has_valid_wave_header(const uint8_t *buf, const size_t buf_size) {

    #define WAVE_CHUNK_ID_OFFSET  8
    
    return (
        buf != NULL && 
        buf_size >= 12 &&
        read32be(buf) == RIFF_CHUNK_ID &&
        read32be(buf + WAVE_CHUNK_ID_OFFSET) == WAVE_CHUNK_ID
    );
}

int read_format_chunk(const uint8_t* buf, const size_t buf_size, FMT_CHUNK_COMMON* chunk_out, size_t* offset) {
    
    #define FMT_CHUNK_SIZE 4 + 4 + sizeof(FMT_CHUNK_COMMON)

    if(!has_valid_wave_header(buf, buf_size) || 
       buf_size < FMT_CHUNK_SIZE ||
       read32be(buf + WAV_HEADER_SIZE) != fmt_CHUNK_ID ||
       read32be(buf + WAV_HEADER_SIZE + 4) < sizeof(FMT_CHUNK_COMMON)
       )
        return 0;
    
    *offset += WAV_HEADER_SIZE + sizeof(fmt_CHUNK_ID) + 4;

    memcpy(chunk_out, buf + *offset, sizeof(FMT_CHUNK_COMMON));
    
    *offset += sizeof(FMT_CHUNK_COMMON);
    
    return 1;
}

int next_chunk_offset(const uint8_t *buf, const size_t buf_size, uint32_t* current_chunk_id, size_t* offset) {
    
    if (buf_size < 8)
        return 0;

    print_32_bits(read32be(buf));
    *current_chunk_id = read32be(buf);
    
    *offset += 8 + read32le(buf + 4);

    return 1;
}

int read_copy_chunk(const uint8_t *buf, const size_t buf_size,
                    WAV_CHUNK *chunk_out, size_t* offset) {

    if (buf_size < 8)
        return 0;

    switch (read32be(buf)) {

    default:
        return 0;

    case LIST_CHUNK_ID:
    case INFO_CHUNK_ID:
    case fmt_CHUNK_ID:
    case fact_CHUNK_ID:
    case cue_CHUNK_ID:
    case playlist_CHUNK_ID:
    case data_CHUNK_ID:
        break;
    }

    chunk_out->chunk_id = read32be(buf);
    chunk_out->chunk_size = read32le(buf + 4);
    
    *offset += 8;

    chunk_out->chunk_data = malloc(chunk_out->chunk_size);
    
    if(chunk_out->chunk_data == NULL) 
        return 0;
    
    size_t tocp = chunk_out->chunk_size;

    if (tocp > buf_size)
        tocp = buf_size - 8;

    *offset += tocp;

    memcpy(chunk_out->chunk_data, buf + 8, tocp);

    return 1;
}

void free_chunk(WAV_CHUNK *chunk_out) {

    if(chunk_out->chunk_data == NULL)
        return;

    free(chunk_out->chunk_data);
}

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


void clear_terminal() {
    printf("\033[H\033[J");
}

void visualize(FILE *file) {
    
    size_t buf_size = 1024;
    uint8_t buf[buf_size];
    size_t offset = 0;

    if (fread(buf, 1, buf_size, file) <= 0) {

        printf("Could not read into buffer!\n");
        return;
    }

    FMT_CHUNK_COMMON fmt_chunk;

    if (!read_format_chunk(buf, buf_size, &fmt_chunk, &offset)) {

        printf("Could not read format chunk!\n");
        return;
    }
    
    buf_size -= offset;
    
    uint32_t current_chunk_id;
    size_t next_chunk = 0;

    for(;;) {

        int success = next_chunk_offset(buf + offset, buf_size, &current_chunk_id, &next_chunk );
        
        printf("Next chunk offset: %zu\n", next_chunk);

        if (!success) {
            return;
        }
        else if(current_chunk_id == data_CHUNK_ID) {
            break;
        }
        
        offset += next_chunk;

        while (next_chunk + 8 > buf_size) {
            
            next_chunk -= buf_size;

            offset = next_chunk;

            buf_size = sizeof(buf);

            unsigned long read;
            
            if((read = fread(buf, 1, buf_size, file)) < buf_size) {
                
                if(next_chunk > read) {
                    return;
                }
                
                buf_size = read;
            } 
        }
    }
    
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        printf("Could not get terminal size!\n");
        return ;
    }
    
    ws.ws_row -= 5;
    ws.ws_col -= 5;

    printf("Terminal size %d %d\n", ws.ws_row, ws.ws_row);
    
    offset += 4;

    size_t data_chunk_length = read32le(buf + offset);

    offset += 4;

    printf("Found data chunk! At offset: %zu\n", offset);
    printf("Data chunk size: %zu\n", data_chunk_length);
    
    int bytes_per_sample = (fmt_chunk.bit_per_sample + 7 ) / 8;
    int bytes_per_sample_channel = bytes_per_sample * fmt_chunk.num_channels;
    
    uint8_t *screen_buff = malloc(ws.ws_col * ws.ws_row);
    
    
    uint64_t max_sample_value = pow(2, fmt_chunk.bit_per_sample);

    uint32_t precision = 1000;
    uint32_t batch_size = fmt_chunk.samples_per_second / precision;
    
    int64_t max_per_channel[fmt_chunk.num_channels];
    
    int64_t bits_in = 0;
    printf("Batch size: %u\n", batch_size);
    
    for(int frame = 0; frame < 50; frame += 1) {

        memset(screen_buff, ' ',ws.ws_col * ws.ws_row);
    
    for(int x = 0; x < ws.ws_col; x++) {
        

        for(int n = 0; n < batch_size; n++) {
            
            bits_in ++;

            offset += bytes_per_sample_channel;

            if (offset + bytes_per_sample_channel > buf_size) {

                memcpy(buf, buf + offset, buf_size - offset);

                offset = buf_size - offset;

                if (fread(buf + offset, 1, buf_size - offset, file) <
                    bytes_per_sample_channel) {

                    printf("Hit EOF\n");
                    goto cleanup;
                }

                offset = 0;
            }

            for (int channel = 0; channel < fmt_chunk.num_channels; channel++) {
                max_per_channel[channel] = 0;
            }

            for (int channel = 0; channel < fmt_chunk.num_channels; channel++) {

                int64_t sample =
                    read_n_le(buf + offset + channel * bytes_per_sample,
                              buf_size - offset, bytes_per_sample);

                if (sample < 0)
                    sample = -1 * sample;

                if (max_per_channel[channel] < sample)
                    max_per_channel[channel] = sample;

                    // max_per_channel[channel] += sample;
            }
        }
            // for (int channel = 0; channel < fmt_chunk.num_channels; channel++) {
            //         max_per_channel[channel] /= fmt_chunk.num_channels / 2;
            // }

        // printf("Max value %zu\n", max_per_channel[0]);
        
        if(max_per_channel[0] == 0)
            continue;

        uint64_t scaled =
            ((double)max_per_channel[0] / (double)max_sample_value) *
            ws.ws_row;

        if (scaled >= ws.ws_row) {
            scaled = ws.ws_row;
        }
        
        // #define frombottom

        #ifdef frombottom 

        for (int y = 0; y < scaled; y++) {

            screen_buff[y  * ws.ws_col + x] = '#';
        }
        
        #else

        
        for (int y = (ws.ws_row - scaled)/2; y < scaled; y++) {

            screen_buff[y  * ws.ws_col + x] = '#';
        }

        #endif


        // int height = ws.ws_row / fmt_chunk.num_channels;

        // for(int channel = 0; channel < fmt_chunk.num_channels; channel++) {

        //     uint64_t scaled = ((double)max_per_channel[channel] / (double)max_sample_value) * ws.ws_row;

        //     if (scaled >= height) {
        //         scaled = height;
        //     }

        //     for (int y = 0; y < scaled; y++) {

        //         screen_buff[(y + channel*height)*ws.ws_col + x] = '#';
        //     }
        // }
    }
    
    clear_terminal();
    fflush(stdout);

    for (int y = 0; y < ws.ws_row; y++) {
        for (int x = 0; x < ws.ws_col; x++) {
            

            putchar(screen_buff[(ws.ws_row- y - 1) *ws.ws_col + x]);
        }
        printf("\n");
    }
    fflush(stdout);
    usleep(
        1000000
    );
    }
    
    cleanup:

    for (int y = 0; y < ws.ws_row; y++) {
        for (int x = 0; x < ws.ws_col; x++) {
            

            putchar(screen_buff[(ws.ws_row- y - 1) *ws.ws_col + x]);
        }
        printf("\n");
    }
    printf("Seconds of display: %f\n",(double) fmt_chunk.samples_per_second /bits_in);
    

    free(screen_buff);

    return;
}
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"

void print_help(const char *argv[])
{
  printf("Unpacks bzImage file.\n"
      "Usage:\n"
      "%s <bzImage_path>\n", argv[0]);
}

#define CHUNK 16384

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, 16 + MAX_WBITS);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zlib: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

int main(int argc, const char *argv[])
{
  if (argc < 2)
  {
    print_help(argv);
    return 1;
  }

  unsigned char gzip_magic[] = {0x1F, 0x8B, 0x08};
  const char * in_file_path = argv[1];
  FILE * in_file_ptr = NULL;
  uint8_t * buffer = NULL;
  size_t in_file_size = 0;

  in_file_ptr = fopen(in_file_path, "rb");

  if (in_file_ptr == NULL) 
  {
    perror("Error opening file");
    return 1;
  }
  else
  {
    fseek(in_file_ptr, 0, SEEK_END); // may be non-portable
    in_file_size = ftell(in_file_ptr);
    fseek(in_file_ptr, 0, SEEK_SET);

    buffer = (uint8_t *) malloc(in_file_size);
    fread(buffer, 1, in_file_size, in_file_ptr);
  }

  int success = 0;
  size_t image_offset = 0;
  char out_file_name[255];
  strncpy(out_file_name, argv[1], strlen(argv[1]));
  strcat(out_file_name,  "_unpacked");

  // try until gunzip unpacking success or end of file
  for (size_t i = 0; !success && i < in_file_size - sizeof(gzip_magic); i++) 
  {
    // is gzip magic found at this offset
    if (!memcmp(buffer + i, gzip_magic, sizeof(gzip_magic)))
    {
      image_offset = i;
      printf("Found gzip magic at 0x%08lX\n", image_offset);

      fseek(in_file_ptr, i, SEEK_SET);
      FILE* out_file_ptr = fopen(out_file_name, "w");
      int ret = inf(in_file_ptr, out_file_ptr);

      success = ret == Z_OK;

      if (!success)
        zerr(ret);

      fclose(out_file_ptr);
    }
  }

  fclose(in_file_ptr);

  if (success)
  {
    printf("Success: unpacked image was saved in file %s\n", out_file_name);
    return 0;
  }
  else
  {
    printf("Error: packed image was not found in file %s\n", argv[1]);
    return 1;
  }
}

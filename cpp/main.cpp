#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "zlib.h"

void print_help(const char *argv[])
{
  std::cout << "Unpacks bzImage file.\nUsage:\n"
    << argv[0] << " <bzImage_path>\n";
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
    exit(1);
  }

  // open file
  auto fs = std::ifstream(argv[1]);

  if (!fs)
  {
    std::cerr << "Error opening file " << argv[1] << "\n";
    exit(1);
  }

  // get file size
  fs.seekg(0, std::ios::end);
  size_t length = fs.tellg();
  fs.seekg(0, std::ios::beg);

  // read file
  std::vector<char> buffer(length);
  fs.read(&buffer[0], length);

  bool success = false;
  size_t image_offset = 0;
  unsigned char gzip_magic[] = {0x1F, 0x8B, 0x08};
  std::string unpacked_file_name, uncompressed_file_name;

  // try until gunzip unpacking success or end of file
  for (size_t i = 0; !success && i < length - sizeof(gzip_magic); i++) 
  {
    // is gzip magic found at this offset
    if (!memcmp(buffer.data() + i, gzip_magic, sizeof(gzip_magic)))
    {
      image_offset = i;
      std::cout << "Found gzip magic at 0x" << std::hex << image_offset << "\n";

      // save in output file data from gzip magic to end of input file
      unpacked_file_name = std::string(argv[1]) + ".gz";
      auto unpacked_file_stream = std::ofstream(unpacked_file_name);
      unpacked_file_stream.write(buffer.data() + image_offset, length - image_offset);
      unpacked_file_stream.close();

      uncompressed_file_name = std::string(argv[1]) + "_unpacked";
      FILE* unpacked_file_ptr = fopen(unpacked_file_name.c_str(), "rb");
      FILE* uncompressed_file_ptr = fopen(uncompressed_file_name.c_str(), "w");

      if (unpacked_file_ptr == NULL || uncompressed_file_ptr == NULL) 
      {
        perror("Error opening file");
        return 1;
      }

      int ret = inf(unpacked_file_ptr, uncompressed_file_ptr);
      success = ret == Z_OK;

      if (!success)
        zerr(ret);

      fclose(unpacked_file_ptr);
      fclose(uncompressed_file_ptr);
    }
  }

  if (success)
    std::cerr << "Success: unpacked image was saved in file " << unpacked_file_name << "\n";
  else
  {
    std::cerr << "Error: packed image was not found in file " << argv[1] << "\n";
    exit(1);
  }

  return 0;
}

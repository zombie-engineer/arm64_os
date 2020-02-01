#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define getnext_char_expected(ch) if (read(fd, &tmp_char, 1) != 1 || tmp_char != ch) abort() 
#define getnext_char(ch) if (read(fd, &tmp_char, 1) != 1) abort()

#define call_err_ret(fn, desc, ...) \
  ret = fn(__VA_ARGS__);\
  if (ret) {\
    perror(desc);\
    return ret;\
  }

typedef struct font_desc {
  char name[32];
  int glyph_size;
  void *bitmap;
  int bitmap_sz;
} font_desc_t;
  
int read_pbm_number(int fd)
{
  char to_atoi[128] = { 0 };
  int to_atoi_len;
  char tmp_char;
  to_atoi_len = 0;
  while(1) {
    getnext_char();
    if (tmp_char == 0xa)
      break;
    to_atoi[to_atoi_len++] = tmp_char;
  }
  return strtol(to_atoi, 0, 10);
}

const char *read_pbm_image(const char *filepath, int *width, int *height)
{
  int fd;
  char magic[2];
  char tmp_char;
  int max_value;
  int bytes_per_pixel;
  int memory_size;
  char *raw_img;

  fd = open(filepath, O_RDONLY);
  if (fd == -1)
    abort();
  read(fd, magic, 2);
  getnext_char_expected(0xa);
  *width = read_pbm_number(fd);
  *height = read_pbm_number(fd);
  max_value = read_pbm_number(fd);
  bytes_per_pixel = max_value / 255;
  memory_size = bytes_per_pixel * (*width) * (*height);
  raw_img = malloc(memory_size);
  if (read(fd, raw_img, memory_size) != memory_size)
    abort();

  close(fd);
  return raw_img;
}

char *fmtcvt_pbm_to_bitmap_nokia(int width, int height, const char *raw_img, int *img_size)
{
  char *img;
  int x, y, bit;
  int new_image_size = width * height / 8;
  img = malloc(new_image_size);
  for (y = 0; y < height / 8; ++y) {
    for (x = 0; x < width; ++x) {
      char new_value = 0;
      for (bit = 0; bit < 8; ++bit) {
        char value = *(raw_img + (y * 8 + bit) * width + x);
        if (value) 
          new_value |= 1 << bit;
      } 
      *(img + y * width + x) = new_value;
    }
  }
  return img;
}

int fmtcvt_pbm_to_bitmap(font_desc_t *d, int width, int height, const char *raw_img)
{
  int x, y, bit;
  d->bitmap_sz = width * height / 8;
  d->bitmap = malloc(d->bitmap_sz);
  char *dst = (char *)d->bitmap;
  char byteval = 0;
  bit = 0;
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      char value = *(raw_img + y * width + x);
      byteval |= (value ? 1 : 0) << bit;
      if (bit++ == 8) {
        bit = 0;
        *(dst++) = byteval;
        byteval = 0;
      }
    } 
  }
  return 0;
}

#define log_syserr(msgfmt, ...) \
  fprintf(stderr, "%s:%s:%d:"msgfmt", err: %d(%s)\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__, errno, strerror(errno))

#define log_err(msgfmt, ...) \
  fprintf(stderr, "%s:%s:%d:"msgfmt"\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

int store_buf(void *buf, int bufsz, const char *filename)
{
  int fd, ret;
  fd = open(filename, O_WRONLY|O_CREAT);
  if (fd == -1) {
    log_syserr("Failed to open file: %s", filename);
    return -1;
  }

  ret = write(fd, buf, bufsz);
  if (ret == -1) {
    log_syserr("Failed to write to file: %d(%s)", fd, filename);
    return -1;
  }
  if (ret != bufsz) {
    log_err("Failed to completely write to file: %d(%s)", fd, filename);
    return -1;
  }
  return 0;
}

int bitmap_print_as_char_array(FILE *fp, const char *font_name, const unsigned char *bitmap, int bitmap_sz, int n_cols)
{
  int i; 
  fprintf(fp, "char font_bitmap_raw_%s[] = {", font_name);
  fprintf(fp, "\n    ");
  for (i = 0; i < bitmap_sz - 1; ++i) {
    fprintf(fp, "%02x", bitmap[i]);
    if ((i + 1) % n_cols)
      fprintf(fp, ", ");
    else 
      fprintf(fp, ",\n    ");
  }
  fprintf(fp, "%02x\n}\n;", bitmap[i]);
  return 0;
}

int generate_font_header_file(font_desc_t *d, const char *filename)
{
  FILE *fp;
  int ret;
  fp = fopen(filename, "w");
  if (!fp)
    return -1;
  ret = bitmap_print_as_char_array(fp, d->name, d->bitmap, d->bitmap_sz, 16);
  fclose(fp);
  return ret;
}

int main(int argc, char ** argv)
{
  const char *raw_img;
  const char *header_img;
  int width, height;
  font_desc_t d = { 0 };
  raw_img = read_pbm_image(argv[1], &width, &height);

  strcpy(d.name, "myfont");
  d.glyph_size = 8;

  if (fmtcvt_pbm_to_bitmap(&d, width, height, raw_img))
    return -1;
  if (generate_font_header_file(&d, "myfont.h"))
    return -1;
  // store_buf(img, img_size, argv[2]);
  return 0;
}

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "stdlib.h"
#include "stdio.h"


unsigned char* read_entire_file(FILE* f) {
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  unsigned char* buff = malloc(len);
  fseek(f, 0, SEEK_SET);
  fread(buff, 1, len, f);
  return buff;
}

void render_data(unsigned char* pixels, int rows, int cols) {
  printf("+");
  for (int i = 2; i < cols; ++i) printf("_");
  printf("+\n");

  for (int i = 0; i < rows; ++i) {
    printf("|");
    for (int j = 0; j < cols; ++j) {
      unsigned char pix = pixels[i * cols + j];
      if (pix < 10) printf("  ");
      else if (pix < 20) printf("..");
      else if (pix < 40) printf(",,");
      else if (pix < 100) printf("oo");
      else if (pix < 150) printf("##");
      else printf("\u2588\u2588");
    }
    printf("|\n");
  }

  printf("+");
  for (int i = 2; i < cols; ++i) printf("_");
  printf("+\n");
}

unsigned char maxv(unsigned char* chars, int n) {
  unsigned char ret = 0;
  for (int j = 0; j < n; ++j) {
    ret = ret < chars[j] ? chars[j] : ret;
  }
  return ret;
}

#define IMG_SIZE 512
#define CHARS 30 
void old_api(unsigned char* font_data) {
  unsigned char pixels[IMG_SIZE][IMG_SIZE] = {0};
  stbtt_bakedchar baked[CHARS] = {0};
  stbtt_fontinfo info = {0};

  int n = stbtt_GetNumberOfFonts(font_data);
  printf("Num of fonts = %d\n", n);
  
  char char_to_draw = 'a';
  int font = stbtt_InitFont(&info, &pixels[0][0], 0);
  int char_index = stbtt_FindGlyphIndex(&info, char_to_draw);
  printf("%c(%d) = %d\n", char_to_draw, char_to_draw, char_index);
  n = stbtt_BakeFontBitmap(font_data, 0,
                              12,                     // height of font in pixels
                              &pixels[0][0], IMG_SIZE, IMG_SIZE,  // bitmap to be filled in
                              char_to_draw, CHARS,          // characters to bake
                              baked);             // you allocate this, it's num_chars long


  for (int i = 0; i < CHARS; ++i) {
    printf("x0=%u y0=%u x1=%u y1=%u, %f %f %f\n", baked[i].x0, baked[i].y0, baked[i].x1, baked[i].y1, baked[i].xoff, baked[i].yoff, baked[i].xadvance);
  }
  printf("%d - %d\n", n, maxv(&pixels[0][0], IMG_SIZE*IMG_SIZE));
  render_data(&pixels[0][0], IMG_SIZE, IMG_SIZE);
}

void new_api(unsigned char* font_data, unsigned char* pixels) {
  stbtt_pack_context cntx = {0};
  stbtt_packedchar charz[CHARS] = {0};

  int res = stbtt_PackBegin(&cntx, pixels, IMG_SIZE, IMG_SIZE, 0, 1, NULL);
  if (res == 0) fprintf(stderr, "Failed to pack begin\n");
  stbtt_PackSetOversampling(&cntx, 2, 2);
  res = stbtt_PackFontRange(&cntx, font_data, 0, 6.f, 'a', CHARS, &charz[0]);
  //res = stbtt_PackFontRange(&cntx, font_data, 0, 12.f, 'a', CHARS, &charz[0]);

  render_data(pixels, IMG_SIZE, IMG_SIZE);
}


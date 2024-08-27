#include "external/raylib.h"
#include "external/stb_rect_pack.h"
#include "stdio.h"

#include "ft2build.h"
#include "freetype/freetype.h"
#include "freetype/ftoutln.h"

#define LOG_COMMON(type, fmt, ...) fprintf(stderr, "[" type "][%s:%d]" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGI(fmt, ...) LOG_COMMON(" INFO", fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) LOG_COMMON("ERROR", fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) do { LOG_COMMON("FATAL", fmt, ##__VA_ARGS__); \
  exit(1); \
} while(0)
#define TRY(expr) err= (expr); if (err != 0) LOGF("Failed to '" #expr "': %s(%d)", FT_Error_String(err), err); LOGI(#expr " OK")

void render_data(unsigned char* pixels, int rows, int cols, int pitch) {
  printf("+");
  for (int i = 2; i < cols; ++i) printf("_");
  printf("+\n");

  for (int i = 0; i < rows; ++i) {
    printf("|");
    for (int j = 0; j < cols; ++j) {
      unsigned char pix = pixels[i * pitch + j];
      if (pix < 10) printf(" ");
      else if (pix < 20) printf(".");
      else if (pix < 40) printf(",");
      else if (pix < 100) printf("o");
      else if (pix < 150) printf("#");
      else printf("\u2588");
    }
    printf("|\n");
  }

  printf("+");
  for (int i = 2; i < cols; ++i) printf("_");
  printf("+\n");
}
typedef struct { 
  unsigned char* start;
  int rows;
  int cols;
  int padding;
} ch;

#define CHAR_COUNT ('z' - 'a' + 1)

int main(void) {
  FT_Library lib = {0};
  FT_Error err = {0};
  FT_Face face = {0};
  ch all[CHAR_COUNT];
  stbrp_context rp_context = { 0 };
  stbrp_rect rects[CHAR_COUNT];

  TRY(FT_Init_FreeType(&lib));
  TRY(FT_New_Face(lib, "/usr/share/fonts/noto/NotoSans-Regular.ttf", 0, &face));
  TRY(FT_Load_Glyph(face, 0, FT_LOAD_TARGET_LCD));
  for (char i = 'a'; i <= 'z'; ++i) {
    LOGI("pitch = %d", face->glyph->bitmap.pitch);
    TRY(FT_Load_Char(face, (int)i, FT_LOAD_VERTICAL_LAYOUT | FT_LOAD_COLOR | FT_LOAD_DEFAULT));
    TRY(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LCD));
    
    all[i - 'a'] = (ch) {
      .start = face->glyph->bitmap.buffer,
      .rows = face->glyph->bitmap.rows,
      .cols = face->glyph->bitmap.width,
      .padding = face->glyph->bitmap.pitch,
    };
  }

  stbrp_init_target (&rp_context, 32, 32, stbrp_node *nodes, int num_nodes);
  TRY(!stbrp_pack_rects(&rp_context, stbrp_rect *rects, int num_rects));
  FT_GlyphSlot g = face->glyph;
  while (g != NULL) {
    ch c = {
      .start = g->bitmap.buffer,
      .rows = g->bitmap.rows,
      .cols = g->bitmap.width,
      .padding = g->bitmap.pitch,
    };
    g = g->next;
    render_data(c.start, c.rows, c.cols, c.padding);
  }
  TRY(FT_Done_Face(face));
  TRY(FT_Done_FreeType(lib));
  return 0;
}
// gcc -I /usr/include/freetype2 tex_ft2.c -lfreetype && ./a.out 

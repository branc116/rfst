#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define STB_DS_IMPLEMENTATION
#include "external/stb_ds.h"

#define BR_SHADERS_IMPLEMENTATION
#define BR_HAS_SHADER_RELOAD 1
#define simple_fs "simple.fs"
#define simple_vs "simple.vs"
#define BR_ALL_SHADERS(X, X_VEC, X_BUF) \
  X(simple, 1024,                       \
      X_VEC(color, 3)                   \
      X_VEC(resolution, 2)              \
      X_VEC(atlas, TEX),                \
                                        \
      X_BUF(pos, 2)                     \
      X_BUF(tex_pos, 2)                 \
    )
#include "external/br_shaders.h"

#include "raylib.h"

#include "stdlib.h"
#include "stdio.h"

#define IMG_SIZE 512
#define CHARS 30

unsigned char* read_entire_file(FILE* f) {
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  unsigned char* buff = malloc(len);
  fseek(f, 0, SEEK_SET);
  fread(buff, 1, len, f);
  return buff;
}

typedef struct {
  char key;
  stbtt_packedchar value;
} key_to_packedchar_t;

typedef struct {
  int key;
  key_to_packedchar_t* value;
} size_to_font;

typedef struct {
  int size;
  char ch;
} char_sz;

typedef struct {
  char_sz key;
  unsigned char value; 
} to_bake_t;

typedef struct {
  br_shader_simple_t** shader;
  size_to_font* sizes;
  to_bake_t* to_bake;
  unsigned char* bitmap_pixels;
  int bitmap_pixels_len;
  int bitmap_pixels_height;
  int bitmap_pixels_width;
  int bitmap_texture_id;
  stbtt_pack_context pack_cntx;
  unsigned char* font_data;
} br_text_renderer_t;


br_text_renderer_t br_text_renderer_malloc(int bitmap_width, int bitmap_height, unsigned char* font_data, br_shader_simple_t** shader) {
  br_text_renderer_t renderer = {
    .shader = shader,
    .sizes = NULL,
    .to_bake = NULL,
    .bitmap_pixels = BR_MALLOC(bitmap_width * bitmap_height),
    .bitmap_pixels_len = bitmap_width * bitmap_height,
    .bitmap_pixels_width = bitmap_width,
    .bitmap_pixels_height = bitmap_height,
    .bitmap_texture_id = 0,
    .pack_cntx = {0},
    .font_data = font_data
  };
  int res = stbtt_PackBegin(&renderer.pack_cntx, renderer.bitmap_pixels, bitmap_width, bitmap_height, 0, 1, NULL);
  stbtt_PackSetOversampling(&renderer.pack_cntx, 2, 2);
  if (res == 0) fprintf(stderr, "Failed to pack begin\n");
  return renderer;
}

int br_text_renderer_sort_by_size(const void* s1, const void* s2) {
  to_bake_t a = *(to_bake_t*)s1, b = *(to_bake_t*)s2;
  if (a.key.size < b.key.size) return -1;
  else if (a.key.size > b.key.size) return 1;
  else if (a.key.ch < b.key.ch) return -1;
  else if (a.key.ch > b.key.ch) return 1;
  else return 0;
}

void br_text_renderer_dump(br_text_renderer_t* r) {
  stbtt_packedchar charz[256] = {0};
  int hm_len = stbds_hmlen(r->to_bake);
  if (hm_len > 0) {
    qsort(r->to_bake, hm_len, sizeof(r->to_bake[0]), br_text_renderer_sort_by_size);
    int old_size = r->to_bake[0].key.size;
    int pack_from = 0;

    for (int i = 1; i < hm_len; ++i) {
      int new_size = r->to_bake[i].key.size;
      int p_len = r->to_bake[i].key.ch - r->to_bake[i - 1].key.ch;
      if (new_size == old_size && p_len < 10) continue;
      p_len = 1 + r->to_bake[i - 1].key.ch - r->to_bake[pack_from].key.ch;
      stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, old_size, r->to_bake[pack_from].key.ch, p_len, &charz[0]);
      int k = stbds_hmgeti(r->sizes, old_size);
      if (k == -1) {
        stbds_hmput(r->sizes, old_size, NULL);
        k = stbds_hmgeti(r->sizes, old_size);
      }
      for (int j = 0; j < p_len; ++j) {
        stbds_hmput(r->sizes[k].value, r->to_bake[pack_from].key.ch, charz[j]);
      }
      old_size = new_size;
      pack_from = i;
    }
    if (pack_from < hm_len) {
      int s_len = 1 + r->to_bake[hm_len - 1].key.ch - r->to_bake[pack_from].key.ch;
      stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, old_size, r->to_bake[pack_from].key.ch, s_len, &charz[0]);
      int k = stbds_hmgeti(r->sizes, old_size);
      if (k == -1) {
        stbds_hmput(r->sizes, old_size, NULL);
        k = stbds_hmgeti(r->sizes, old_size);
      }
      for (int j = 0; j < s_len; ++j) {
        stbds_hmput(r->sizes[k].value, r->to_bake[pack_from].key.ch + j, charz[j]);
      }
    }

    stbds_hmfree(r->to_bake);
    rlUnloadTexture(r->bitmap_texture_id);
    r->bitmap_texture_id = rlLoadTexture(r->bitmap_pixels, r->bitmap_pixels_width, r->bitmap_pixels_height, RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE, 1);
    Texture tex = {
        .id = r->bitmap_texture_id,
        .width = r->bitmap_pixels_width,
        .height = r->bitmap_pixels_height,
        .mipmaps = 1,
        .format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };
    DrawTexture(tex, 0, 0, WHITE);
  }
  br_shader_simple_t* simp = *r->shader;
  simp->uvs.resolution_uv = (Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() };
  simp->uvs.atlas_uv = r->bitmap_texture_id;
  br_shader_simple_draw(*r->shader);
  simp->len = 0;
}


void br_text_renderer_push(br_text_renderer_t* r, Vector2 loc, const char* text, int font_size) {
  int i = stbds_hmgeti(r->sizes, font_size);
  float og_x = loc.x;
  if (i == -1) {
    for (const char* c = text; *c != '\0'; ++c) {
      stbds_hmput(r->to_bake, ((char_sz){.size = font_size, .ch = *c}), 0);
    }
  } else {
    size_to_font stf = r->sizes[i];
    br_shader_simple_t* simp = *r->shader;
    int len_pos = simp->len * 3;
    int len_tex = simp->len * 3;
    Vector2* pos = (void*)simp->pos_vbo;
    Vector2* tpos = (void*)simp->tex_pos_vbo;

    for (const char* c = text; *c != '\0'; ++c) {
      int i = stbds_hmgeti(stf.value, *c);
      if (*c == '\n') {
        loc.y += font_size * 1.1;
        loc.x = og_x;
        continue;
      }
      if (*c == '\r') {
        continue;
      }
      if (i == -1) {
        stbds_hmput(r->to_bake, ((char_sz){.size = font_size, .ch = *c}), 0);
      } else {
        if (simp->len * 3 >= simp->cap) {
          br_text_renderer_dump(r);
          len_pos = simp->len * 3;
          len_tex = simp->len * 3;
          pos = (void*)simp->pos_vbo;
          tpos = (void*)simp->tex_pos_vbo;
        }
        stbtt_aligned_quad q;
        stbtt_packedchar ch = stf.value[i].value;
        stbtt_GetPackedQuad(&ch, r->bitmap_pixels_width, r->bitmap_pixels_height,
                                       0,
                                       &loc.x, &loc.y,
                                       &q,
                                       false);
        pos[len_pos++] = (Vector2) { q.x0, q.y0 };
        pos[len_pos++] = (Vector2) { q.x0, q.y1 };
        pos[len_pos++] = (Vector2) { q.x1, q.y1 };
        pos[len_pos++] = (Vector2) { q.x1, q.y1 };
        pos[len_pos++] = (Vector2) { q.x0, q.y0 };
        pos[len_pos++] = (Vector2) { q.x1, q.y0 };
        tpos[len_tex++] = (Vector2) { q.s0, q.t0 };
        tpos[len_tex++] = (Vector2) { q.s0, q.t1 };
        tpos[len_tex++] = (Vector2) { q.s1, q.t1 };
        tpos[len_tex++] = (Vector2) { q.s1, q.t1 };
        tpos[len_tex++] = (Vector2) { q.s0, q.t0 };
        tpos[len_tex++] = (Vector2) { q.s1, q.t0 };
        simp->len += 3;
      }
    }
  }
}

void app2(unsigned char* font_data) {
  InitWindow(800, 400, "Init");
  SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);
  br_shaders_t br = br_shaders_malloc();
  br_text_renderer_t r = br_text_renderer_malloc(4*1024, 4*1024, font_data, &br.simple);
  rlDisableBackfaceCulling();
  int size = 15;
  int tex_pos_x = 0;
  int tex_pos_y = 0;
  int text_pos_y = 0;
  FILE* f = fopen(__FILE__, "rb");
  unsigned char* text = read_entire_file(f);
  br_shaders_start_refreshing(br);

  while (WindowShouldClose() == false) {
    BeginDrawing();
    ClearBackground(BLACK);
    if (br.reload->should_reload == true) br_shaders_refresh(br);
    if (false == IsKeyDown(KEY_T)) {
      text_pos_y += GetMouseWheelMove() * 10;
      br_text_renderer_push(&r, (Vector2) {100, 100}, "hello my friend", size);
      br_text_renderer_push(&r, (Vector2) {100, text_pos_y}, text, size);
      if (IsKeyDown(KEY_DOWN)) size -= 1;
      if (IsKeyDown(KEY_UP)) size += 1;
      br_text_renderer_dump(&r);
    } else {
      if (IsKeyDown(KEY_LEFT)) tex_pos_x -= 1;
      if (IsKeyDown(KEY_RIGHT)) tex_pos_x += 1;
      if (IsKeyDown(KEY_DOWN)) tex_pos_y -= 1;
      if (IsKeyDown(KEY_UP)) tex_pos_y += 1;
      Texture tex = {
          .id = r.bitmap_texture_id,
          .width = r.bitmap_pixels_width,
          .height = r.bitmap_pixels_height,
          .mipmaps = 1,
          .format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
      };
      DrawTexture(tex, tex_pos_x, tex_pos_y, WHITE);
    }
    EndDrawing();
  }
}

void app(unsigned char* font_data) {
  unsigned char pixels[IMG_SIZE][IMG_SIZE] = {0};
  stbtt_pack_context cntx = {0};
  stbtt_packedchar charz[CHARS] = {0};

  int res = stbtt_PackBegin(&cntx, &pixels[0][0], IMG_SIZE, IMG_SIZE, 0, 2, NULL);
  if (res == 0) fprintf(stderr, "Failed to pack begin\n");
  //stbtt_PackSetOversampling(&cntx, 2, 2);
  res = stbtt_PackFontRange(&cntx, font_data, 0, 72.f, 'A', CHARS, &charz[0]);

  InitWindow(800, 400, "Init");
  SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);
  br_shaders_t br = br_shaders_malloc();

  unsigned int textureId = rlLoadTexture(pixels, IMG_SIZE, IMG_SIZE, RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE, 1);

  Texture tex = {
      .id = textureId,
      .width = IMG_SIZE,
      .height = IMG_SIZE,
      .mipmaps = 1,
      .format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
  };

  rlDisableBackfaceCulling();
  br.reload->path = ".";
  br_shaders_start_refreshing(br);
  
  int f = 0;
  float cury = 72.0f;
  float initx = 0.f;
  float dx = 0.0f, dy = 0.0f;

  while (WindowShouldClose() == false) {
    Vector2* pos = (Vector2*)br.simple->pos_vbo;
    Vector2* tpos = (Vector2*)br.simple->tex_pos_vbo;
    int mod = 2;


    if (IsKeyDown(KEY_DOWN)) dy -= 1.1f;
    if (IsKeyDown(KEY_UP)) dy += 1.1f;
    if (IsKeyDown(KEY_LEFT)) dx -= 1.1f;
    if (IsKeyDown(KEY_RIGHT)) dx += 1.1f;
    Vector2 mwd = GetMouseWheelMoveV();
    initx += mwd.x * 20.;
    cury += mwd.y * 20.f;

    cury += dy * GetFrameTime();
    initx += dx * GetFrameTime();
    float curx = (int)initx;
    dx *= 0.9;
    dy *= 0.9;

    BeginDrawing();
    ClearBackground(BLACK);
    if (br.reload->should_reload == true) br_shaders_refresh(br);
    //DrawRectangle(0, 0, 100, 100, RED);
    br.simple->uvs.resolution_uv = (Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() };
    br.simple->uvs.atlas_uv = textureId;
    int len_pos = 0, len_tex = 0;
    char text[20];
    sprintf(text, "%f", cury);
    DrawText(text, initx + 100, cury + 100, 72, WHITE);
    for (int i = 0; i < CHARS; ++i) {
      {
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(&charz[i], 512, 512,
                                       0,
                                       &curx, &cury,
                                       &q,      // output: quad to draw
                                       false);
        pos[len_pos++] = (Vector2) { q.x0, q.y0 };
        pos[len_pos++] = (Vector2) { q.x0, q.y1 };
        pos[len_pos++] = (Vector2) { q.x1, q.y1 };
        pos[len_pos++] = (Vector2) { q.x1, q.y1 };
        pos[len_pos++] = (Vector2) { q.x0, q.y0 };
        pos[len_pos++] = (Vector2) { q.x1, q.y0 };
        tpos[len_tex++] = (Vector2) { q.s0, q.t0 };
        tpos[len_tex++] = (Vector2) { q.s0, q.t1 };
        tpos[len_tex++] = (Vector2) { q.s1, q.t1 };
        tpos[len_tex++] = (Vector2) { q.s1, q.t1 };
        tpos[len_tex++] = (Vector2) { q.s0, q.t0 };
        tpos[len_tex++] = (Vector2) { q.s1, q.t0 };
        
      }
      br.simple->len += 2;
    }
    br_shader_simple_draw(br.simple);
    if (f == 0) DrawTexture(tex, 100, 100, WHITE);
    br.simple->len = 0;
    EndDrawing();
    f++;
  }
}

int main() {
  FILE* f = fopen("/usr/share/fonts/noto/NotoSans-Regular.ttf", "rb");
  unsigned char* data = read_entire_file(f);
  //new_api(data);
  app2(data);
  return 0;
}
// gcc -ggdb main.c -lm -lraylib && ./a.out

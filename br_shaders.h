// cpp -I. -DDEBUG_MACROS -E ./src/shaders.c | sed 's/^#/\/\//' | clang-format > tmp.c

#pragma once
#ifndef DEBUG_MACROS
#  include "raylib.h"
#  include "rlgl.h"
#  include "stdlib.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NOP2(N, M)

#if !defined(BR_ALL_SHADERS)

#  define simple_fs "simple.fs"
#  define simple_vs "simple.vs"

#  define BR_ALL_SHADERS(X, X_VEC, X_BUF) \
      X(simple, 1024,               \
         X_VEC(m_mvp, 16),                 \
                                          \
         X_BUF(vertexPosition, 3)          \
         X_BUF(vertexNormal, 3)            \
         X_BUF(vertexColor, 3)             \
       )

#endif

#if !defined BR_MALLOC
#  define BR_MALLOC malloc
#endif

#if !defined BR_REALLOC
#  define BR_REALLOC realloc
#endif

#if !defined BR_FREE
#  define BR_FREE free
#endif

#if !defined BR_CALLOC
#  define BR_CALLOC calloc
#endif

#if !defined(BR_CUSTOM_LOGS)
#  define LOGE(fmt, ...) fprintf(stderr, "[ERROR][%s:%d]" fmt, __FILE__, __LINE__, ##__VA_ARGS__); 
#endif


#define VEC_TYPE2 Vector2
#define VEC_TYPE3 Vector3
#define VEC_TYPE4 Vector4
#define VEC_TYPE16 Matrix
#define VEC_TYPETEX unsigned int
#define X_U(NAME, DIM) VEC_TYPE ## DIM NAME ## _uv;
#define X(NAME, CAP, U_VEC, BUFFS) \
  typedef struct {                 \
    U_VEC                          \
  } br_shader_ ## NAME ## _uvs_t;
BR_ALL_SHADERS(X, X_U, NOP2)
#undef X
#undef X_U
#undef VEC_TYPE16
#undef VEC_TYPE4
#undef VEC_TYPE3
#undef VEC_TYPE2

#define X_U(NAME, DIM) int NAME ## _u;
#define X_B(NAME, DIM)                \
  unsigned int NAME ## _vbo_id;       \
  int NAME ## _loc;                   \
  float* NAME ## _vbo;
#define X(NAME, CAP, U_VEC, BUFFS)    \
  typedef struct {                    \
    br_shader_ ## NAME ## _uvs_t uvs; \
    U_VEC                             \
    BUFFS                             \
    int len, cap;                     \
    unsigned int id;                  \
    unsigned int vao_id;              \
  } br_shader_ ## NAME ## _t;
BR_ALL_SHADERS(X, X_U, X_B)
#undef X
#undef X_B
#undef X_U

typedef struct br_shader_reload_t {
  bool should_reload;
  bool should_stop;
  const char* path;
} br_shader_reload_t;

typedef struct br_shaders_t {
#define X(NAME, CAP, UNIF, BUFF) br_shader_ ## NAME ## _t* NAME;
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
#if BR_HAS_SHADER_RELOAD
  br_shader_reload_t* reload;
#endif
} br_shaders_t;

#define X(NAME, CAP, U_VEC, BUFF) void br_shader_ ## NAME ## _draw(br_shader_ ## NAME ## _t* shader);
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X

br_shaders_t br_shaders_malloc(void);
void br_shaders_free(br_shaders_t shaders);
void br_shaders_refresh(br_shaders_t shaders);

#if BR_HAS_SHADER_RELOAD
void br_shaders_start_refreshing(br_shaders_t shaders);
#endif

#ifdef __cplusplus
}
#endif

#if defined(BR_SHADERS_IMPLEMENTATION)

#if !defined(DEBUG_MACROS)
#  include <stdio.h>
#  include <stdint.h>
#  include <stdlib.h>
#endif


#ifdef RELEASE
#  if defined(PLATFORM_DESKTOP)
#    include ".generated/shaders.h"
#  elif defined(PLATFORM_WEB)
#    include ".generated/shaders_web.h"
#  else
#    error "Shaders for this platform arn't defined"
#  endif
#endif

#define X_BUF(NAME, LEN)                                                         \
  do {                                                                           \
    ret->NAME ## _vbo = BR_MALLOC((size_t)(LEN * cap * (int)sizeof(float))); \
    if (NULL == ret->NAME ## _vbo) {                                             \
      LOGE("Failed to allocate shader " #NAME " buffer, exitting...\n");         \
      exit(1);                                                                   \
    }                                                                            \
  } while (0);
#define X(NAME, CAP, U_VEC, BUFF)                                                   \
  inline static br_shader_ ## NAME ## _t* br_shader_ ## NAME ## _malloc(void) {     \
    const int cap = CAP;                                                            \
    br_shader_ ## NAME ## _t* ret = BR_CALLOC(1, sizeof(br_shader_ ## NAME ## _t)); \
    if (NULL == ret) {                                                              \
      LOGE("Failed to allocate shader " #NAME ", exitting...\n");                   \
      exit(1);                                                                      \
    }                                                                               \
    BUFF                                                                            \
    ret->cap = cap;                                                                 \
    return ret;                                                                     \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  shader->NAME ## _vbo_id = rlLoadVertexBuffer(shader->NAME ## _vbo, LEN * shader->cap * (int)sizeof(float), true); \
  rlSetVertexAttribute((uint32_t)shader->NAME ## _loc, LEN, RL_FLOAT, 0, 0, 0);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _upload(br_shader_ ## NAME ## _t* shader) { \
    shader->vao_id = rlLoadVertexArray(); \
    rlEnableVertexArray(shader->vao_id); \
    rlEnableShader(shader->id); \
    BUFF \
    rlDisableVertexArray(); \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  rlUpdateVertexBuffer((uint32_t)shader->NAME ## _vbo_id, shader->NAME ## _vbo, LEN * shader->cap * (int)sizeof(float), 0);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _update_vbs(br_shader_ ## NAME ## _t* shader) { \
    BUFF \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  rlUnloadVertexBuffer((uint32_t)shader->NAME ## _vbo_id); \
  BR_FREE(shader->NAME ## _vbo);
#define X_VEC(NAME, LEN) rlUnloadShaderProgram(shader->id);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _free(br_shader_ ## NAME ## _t* shader) { \
    rlUnloadVertexArray((uint32_t)shader->vao_id); \
    BUFF \
    U_VEC \
    BR_FREE(shader); \
  }
BR_ALL_SHADERS(X, X_VEC, X_BUF)
#undef X
#undef X_VEC
#undef X_BUF

#ifdef RELEASE
#  define READ_FILE(file_name) file_name
#  define FREE_FILE_CONTENT(file)
#  define FILE_CONTNET_TYPE const char*
#else
#  define READ_FILE(file_name) LoadFileText(file_name)
#  define FREE_FILE_CONTENT(file) BR_FREE(file)
#  define FILE_CONTNET_TYPE char*
#endif
#define X_BUF(NAME, LEN) shader->NAME ## _loc = rlGetLocationAttrib(shader->id, #NAME);
#define X_VEC(NAME, LEN) shader->NAME ## _u = rlGetLocationUniform(shader->id, #NAME);
#define X(NAME, CAP, VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _compile(br_shader_ ## NAME ## _t* shader) { \
    FILE_CONTNET_TYPE vs = READ_FILE(NAME ## _vs); \
    FILE_CONTNET_TYPE fs = READ_FILE(NAME ## _fs); \
    shader->id = rlLoadShaderCode(vs, fs); \
    FREE_FILE_CONTENT(fs); \
    FREE_FILE_CONTENT(vs); \
    BUFF \
    VEC \
  }
BR_ALL_SHADERS(X, X_VEC, X_BUF)
#undef X
#undef X_VEC
#undef X_BUF

#define SET_VECTEX(NAME) rlSetUniformSampler(shader->NAME ## _u, shader->uvs.NAME ## _uv)
#define SET_VEC16(NAME) rlSetUniformMatrix(shader->NAME ## _u, shader->uvs.NAME ## _uv)
#define SET_VEC4(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_VEC4, 1)
#define SET_VEC3(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_VEC3, 1)
#define SET_VEC2(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_VEC2, 1)
#define SET_VEC1(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_FLOAT, 1)
#define X_VEC(NAME, DIM) SET_VEC ## DIM(NAME);
#define X(NAME, CAP, UNIF, NOP2) \
  inline static void br_shader_ ## NAME ## _update_us(br_shader_ ## NAME ## _t* shader) { \
    UNIF \
  }
BR_ALL_SHADERS(X, X_VEC, NOP2)
#undef X
#undef X_VEC

#define X_BUF(NAME, LEN) \
  rlEnableVertexBuffer(shader->NAME ## _vbo_id); \
  rlSetVertexAttribute((uint32_t)shader->NAME ## _loc, LEN, RL_FLOAT, 0, 0, 0); \
  rlEnableVertexAttribute((uint32_t)shader->NAME ## _loc);
#define X(NAME, CAP, U_VEC, BUFF) \
  void br_shader_ ## NAME ## _draw(br_shader_ ## NAME ## _t* shader) { \
    rlEnableShader((uint32_t)shader->id); \
    br_shader_ ## NAME ## _update_us(shader); \
    br_shader_ ## NAME ## _update_vbs(shader); \
    rlEnableVertexArray(shader->vao_id); \
    BUFF \
    rlDrawVertexArray(0, shader->len*3); \
    rlDisableVertexArray(); \
    rlDisableVertexBuffer(); \
    rlDisableVertexBufferElement(); \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

br_shaders_t br_shaders_malloc(void) {
  br_shaders_t ret;
#define X(NAME, CAP, V, B) ret.NAME = br_shader_ ## NAME ## _malloc();
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _compile(ret.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _upload(ret.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
#if BR_HAS_SHADER_RELOAD
  ret.reload = BR_MALLOC(sizeof(*ret.reload));
#endif
  return ret;
}

void br_shaders_free(br_shaders_t shaders) {
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _free(shaders.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
}

#if BR_HAS_SHADER_RELOAD

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "unistd.h"
#include "pthread.h"
#include "sys/inotify.h"

static void* br_watch_shader_change(void* gv) {
  int fd = inotify_init();
  uint32_t buff[128];
  br_shader_reload_t* refresh = gv;
  inotify_add_watch(fd, refresh->path, IN_MODIFY | IN_CLOSE_WRITE);
  while(refresh->should_stop == false) {
    read(fd, buff, sizeof(buff));
    printf("DIRTY SHADERS\n");
    refresh->should_reload = true;
  }
  fprintf(stderr, "Stopping refresh shader thread\n");
  return NULL;
}

void br_shaders_start_refreshing(br_shaders_t shaders) {
  if (pthread_create(&(pthread_t) {}, &(pthread_attr_t) { 0 }, br_watch_shader_change, shaders.reload)) {
    LOGE("While creating thread %d:`%s`\n", errno, strerror(errno));
  }
}

#else
#  error "Unsupported Platform"
#endif

void br_shaders_refresh(br_shaders_t shaders) {
#  define X_BUF(NAME, LEN) shader->NAME ## _loc = rlGetLocationAttrib(shader->id, #NAME);
#  define X_VEC(NAME, LEN) shader->NAME ## _u = rlGetLocationUniform(shader->id, #NAME);
#  define X(NAME, CAP, VEC, BUFF)                             \
     {                                                        \
       FILE_CONTNET_TYPE vs = READ_FILE(NAME ## _vs);         \
       FILE_CONTNET_TYPE fs = READ_FILE(NAME ## _fs);         \
       unsigned int new_shader_id = rlLoadShaderCode(vs, fs); \
       if (new_shader_id > 0) {                               \
         br_shader_ ## NAME ## _t* shader = shaders.NAME;     \
         shader->id = new_shader_id;                          \
         BUFF                                                 \
         VEC                                                  \
       }                                                      \
       FREE_FILE_CONTENT(fs);                                 \
       FREE_FILE_CONTENT(vs);                                 \
     }
   BR_ALL_SHADERS(X, X_VEC, X_BUF)
#  undef X
#  undef X_VEC
#  undef X_BUF
  shaders.reload->should_reload = false;
}
#endif // HAS_SHADER_RELOAD

#endif

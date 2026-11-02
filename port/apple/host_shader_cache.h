#pragma once
#include <EGL/egl.h>
#include <EGL/eglext.h>
void halo_shader_cache_install(EGLDisplay display, PFNEGLGETPROCADDRESSPROC get_proc);
/* Drops the on-disk ANGLE blobs and, when save_root is set, the Metal warmup file. */
void halo_update_discard_shader_caches(const char *save_root);

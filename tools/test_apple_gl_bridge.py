"""Exercise production stack-argument bridges without linking a GL driver.

A direct driver call must fail to link; all calls must use the selected context's
resolver. The negative control reproduces the missing dispatch-header regression.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / 'port/macos/host/host_gl_bridge.c'

HARNESS = r'''
#include <assert.h>
#include <string.h>
#include <GLES3/gl32.h>
static int allocations, blits;
static unsigned char pixels[16];
static void allocation(GLenum t, GLint l, GLint f, GLsizei w, GLsizei h,
                       GLint b, GLenum format, GLenum type, const void *p) {
    assert(t==GL_TEXTURE_2D && l==0 && f==GL_RGBA8 && w==2 && h==2 && b==0);
    assert(format==GL_RGBA && type==GL_UNSIGNED_BYTE && p==pixels);
    allocations++;
}
static void blit(GLint a, GLint b, GLint c, GLint d, GLint e, GLint f,
                 GLint g, GLint h, GLbitfield mask, GLenum filter) {
    assert(a==0 && b==1 && c==2 && d==3 && e==4 && f==5 && g==6 && h==7);
    assert(mask==GL_COLOR_BUFFER_BIT && filter==GL_NEAREST); blits++;
}
void *halo_graphics_proc(const char *name) {
    if(!strcmp(name,"glTexImage2D"))return (void *)allocation;
    if(!strcmp(name,"glBlitFramebuffer"))return (void *)blit;
    assert(0); return 0;
}
void *host_gl_bridge(const char *name);
int main(void) {
    typedef void (*Texture)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,long long);
    typedef void (*Blit)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,long long,long long);
    Texture texture=(Texture)host_gl_bridge("glTexImage2D");
    texture(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,(long long)pixels);
    texture(GL_TEXTURE_2D,0,GL_RGBA8,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,(long long)pixels);
    ((Blit)host_gl_bridge("glBlitFramebuffer"))(0,1,2,3,4,5,6,7,GL_COLOR_BUFFER_BIT,GL_NEAREST);
    assert(allocations==2 && blits==1);
    assert(!host_gl_bridge("not_a_bridge"));
}
'''

def test_selected_renderer_handles_stack_arguments(tmp_path):
    harness=tmp_path/'probe.c'; harness.write_text(HARNESS)
    output=tmp_path/'probe'
    command=['clang','-DHALO_APPLE_GRAPHICS=1','-I'+str(ROOT/'build/macos/toolchain/gl'),
             '-fsanitize=address,undefined',str(harness)]
    subprocess.run([*command,str(BRIDGE),'-o',str(output)],check=True)
    subprocess.run([str(output)],check=True)
    broken=tmp_path/'broken.c'
    broken.write_text(BRIDGE.read_text().replace('#include "../../apple/host_gl_dispatch.h"',''))
    result=subprocess.run([*command,str(broken),'-o',str(tmp_path/'broken')],capture_output=True,text=True)
    assert result.returncode and 'Undefined symbols' in result.stderr, result.stderr

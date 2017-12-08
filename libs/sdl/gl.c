#define HL_NAME(n) sdl_##n
#include <hl.h>

#if defined(__APPLE__)
#	include <SDL2/SDL.h>
#	include <OpenGL/gl3.h>
#elif defined(_WIN32)
#	include <SDL.h>
#	include <GL/GLU.h>
#	include <glext.h>
#elif defined(HL_CONSOLE)
#	include <graphic/glapi.h>
#elif defined(HL_MESA)
# 	include <GLES3/gl3.h>
#	include <GL/osmesa.h>
#	define GL_IMPORT(fun, t)
#	define glBindFragDataLocation(...)
#	define glGetQueryObjectiv glGetQueryObjectuiv
#else
#	include <SDL2/SDL.h>
#	include <GL/glu.h>
#	include <GL/glext.h>
#endif

#if !defined(HL_CONSOLE) && !defined(HL_MESA)
#define GL_IMPORT(fun, t) PFNGL##t##PROC fun
#include "GLImports.h"
#undef GL_IMPORT
#define GL_IMPORT(fun,t)	fun = (PFNGL##t##PROC)SDL_GL_GetProcAddress(#fun); if( fun == NULL ) return 1
#endif

static int GLLoadAPI() {
#	include "GLImports.h"
	return 0;
}


#ifdef GL_LOG
#	define GLOG(fmt,...)	if( gl_log_active ) fprintf(gl_log_out, __FUNCTION__ "(" fmt ")\n", __VA_ARGS__)
#	define GLOGR(ret,v,fmt,...)	if( gl_log_active ) fprintf(gl_log_out, __FUNCTION__ "(" fmt ") = " ret "\n", __VA_ARGS__, v)
static FILE *gl_log_out;
static bool gl_log_active = true;

static char *hexlog( vbyte *b, int size ) {
	static char tmp[1024];
	static const char *HEX = "0123456789ABCDEF";
	int pos = 0;
	if( b == NULL )
		return "NULL";
	if( size >= (sizeof(tmp)>>1) ) size = (sizeof(tmp)>>1) - 1;
	while( size-- ) {
		unsigned char c = *b++;
		tmp[pos++] = HEX[c>>4];
		tmp[pos++] = HEX[c&15];
	}
	tmp[pos] = 0;
	return tmp;
}

#else
#	define GLOG(...)
#	define GLOGR(...)
#endif

#define ZIDX(val) ((val)?(val)->v.i:0)

// globals
HL_PRIM bool HL_NAME(gl_init)() {
#	ifdef GL_LOG
	gl_log_out = fopen("gllog.txt","wb");
#	endif
	return GLLoadAPI() == 0;
}

HL_PRIM bool HL_NAME(gl_is_context_lost)() {
	// seems like a GL context is rarely lost on desktop
	// let's look at it again on mobile
	return false;
}

HL_PRIM void HL_NAME(gl_clear)( int bits ) {
	GLOG("%d",bits);
	glClear(bits);
}

HL_PRIM int HL_NAME(gl_get_error)() {
	return glGetError();
}

HL_PRIM void HL_NAME(gl_scissor)( int x, int y, int width, int height ) {
	GLOG("%d,%d,%d,%d",x,y,width,height);
	glScissor(x, y, width, height);
}

HL_PRIM void HL_NAME(gl_clear_color)( double r, double g, double b, double a ) {
	GLOG("%g,%g,%g,%g",r,g,b,a);
	glClearColor((float)r, (float)g, (float)b, (float)a);
}

HL_PRIM void HL_NAME(gl_clear_depth)( double value ) {
	GLOG("%g",value);
	glClearDepth(value);
}

HL_PRIM void HL_NAME(gl_clear_stencil)( int value ) {
	GLOG("%d",value);
	glClearStencil(value);
}

HL_PRIM void HL_NAME(gl_viewport)( int x, int y, int width, int height ) {
	GLOG("%d,%d,%d,%d",x,y,width,height);
	glViewport(x, y, width, height);
}

HL_PRIM void HL_NAME(gl_flush)() {
	glFlush();
}

HL_PRIM void HL_NAME(gl_finish)() {
	GLOG("");
#	ifdef GL_LOG
	fflush(gl_log_out);
#	endif
	glFinish();
}

HL_PRIM void HL_NAME(gl_pixel_storei)( int key, int value ) {
	GLOG("%d,%d",key,value);
	glPixelStorei(key, value);
}

HL_PRIM vbyte *HL_NAME(gl_get_string)(int name) {
	GLOG("%d", name);
	return (vbyte*)glGetString(name);
}

// state changes

HL_PRIM void HL_NAME(gl_enable)( int feature ) {
	GLOG("%d",feature);
	glEnable(feature);
}

HL_PRIM void HL_NAME(gl_disable)( int feature ) {
	GLOG("%d",feature);
	glDisable(feature);
}

HL_PRIM void HL_NAME(gl_cull_face)( int face ) {
	GLOG("%d",face);
	glCullFace(face);
}

HL_PRIM void HL_NAME(gl_blend_func)( int src, int dst ) {
	GLOG("%d,%d",src,dst);
	glBlendFunc(src, dst);
}

HL_PRIM void HL_NAME(gl_blend_func_separate)( int src, int dst, int alphaSrc, int alphaDst ) {
	GLOG("%d,%d,%d,%d",src,dst,alphaSrc,alphaDst);
	glBlendFuncSeparate(src, dst, alphaSrc, alphaDst);
}

HL_PRIM void HL_NAME(gl_blend_equation)( int op ) {
	GLOG("%d",op);
	glBlendEquation(op);
}

HL_PRIM void HL_NAME(gl_blend_equation_separate)( int op, int alphaOp ) {
	GLOG("%d,%d",op,alphaOp);
	glBlendEquationSeparate(op, alphaOp);
}

HL_PRIM void HL_NAME(gl_depth_mask)( bool mask ) {
	GLOG("%d",mask);
	glDepthMask(mask);
}

HL_PRIM void HL_NAME(gl_depth_func)( int f ) {
	GLOG("%d",f);
	glDepthFunc(f);
}

HL_PRIM void HL_NAME(gl_color_mask)( bool r, bool g, bool b, bool a ) {
	GLOG("%d,%d,%d,%d",r,g,b,a);
	glColorMask(r, g, b, a);
}

HL_PRIM void HL_NAME(gl_stencil_mask_separate)(int face, int mask) {
	GLOG("%d,%d",face,mask);
	glStencilMaskSeparate(face, mask);
}

HL_PRIM void HL_NAME(gl_stencil_func_separate)(int face, int func, int ref, int mask ) {
	GLOG("%d,%d,%d,%d",face,func,ref,mask);
	glStencilFuncSeparate(face, func, ref, mask);
}

HL_PRIM void HL_NAME(gl_stencil_op_separate)(int face, int sfail, int dpfail, int dppass) {
	GLOG("%d,%d,%d,%d",face,sfail,dpfail,dppass);
	glStencilOpSeparate(face, sfail, dpfail, dppass);
}

// program

static vdynamic *alloc_i32(int v) {
	vdynamic *ret;
	ret = hl_alloc_dynamic(&hlt_i32);
	ret->v.i = v;
	return ret;
}

HL_PRIM vdynamic *HL_NAME(gl_create_program)() {
	int v = glCreateProgram();
	GLOGR("%d",v,"");
	if( v == 0 ) return NULL;
	return alloc_i32(v);
}

HL_PRIM void HL_NAME(gl_delete_program)( vdynamic *s ) {
	GLOG("%d",s->v.i);
	glDeleteProgram(s->v.i);
}

HL_PRIM void HL_NAME(gl_bind_frag_data_location)( vdynamic *p, int colNum, vstring *name ) {
	char *cname = hl_to_utf8(name->bytes);
	GLOG("%d,%d,%n",p->v.i,colNum,cname);
	glBindFragDataLocation(p->v.i, colNum, cname);
}

HL_PRIM void HL_NAME(gl_attach_shader)( vdynamic *p, vdynamic *s ) {
	GLOG("%d,%d",p->v.i,s->v.i);
	glAttachShader(p->v.i, s->v.i);
}

HL_PRIM void HL_NAME(gl_link_program)( vdynamic *p ) {
	GLOG("%d",p->v.i);
	glLinkProgram(p->v.i);
}

HL_PRIM vdynamic *HL_NAME(gl_get_program_parameter)( vdynamic *p, int param ) {
	switch( param ) {
	case 0x8B82 /*LINK_STATUS*/ : {
		int ret = 0;
		glGetProgramiv(p->v.i, param, &ret);
		GLOGR("%d",ret,"%d,%d",p->v.i,param);
		return alloc_i32(ret);
	}
	default:
		hl_error_msg(USTR("Unsupported param %d"),param);
	}
	return NULL;
}

HL_PRIM vbyte *HL_NAME(gl_get_program_info_bytes)( vdynamic *p ) {
	char log[4096];
	*log = 0;
	glGetProgramInfoLog(p->v.i, 4096, NULL, log);
	GLOGR("%s",log,"%d",p->v.i);
	return hl_copy_bytes((vbyte*)log,(int)strlen(log) + 1);
}

HL_PRIM vdynamic *HL_NAME(gl_get_uniform_location)( vdynamic *p, vstring *name ) {
	char *cname = hl_to_utf8(name->bytes);
	int u = glGetUniformLocation(p->v.i, cname);
	GLOGR("%d",u,"%d,%s",p->v.i,cname);
	if( u < 0 ) return NULL;
	return alloc_i32(u);
}

HL_PRIM int HL_NAME(gl_get_attrib_location)( vdynamic *p, vstring *name ) {
	char *cname = hl_to_utf8(name->bytes);
	int loc = glGetAttribLocation(p->v.i, cname);
	GLOGR("%d",loc,"%d,%s",p->v.i,cname);
	return loc;
}

HL_PRIM void HL_NAME(gl_use_program)( vdynamic *p ) {
	GLOG("%d",ZIDX(p));
	glUseProgram(ZIDX(p));
}

// shader

HL_PRIM vdynamic *HL_NAME(gl_create_shader)( int type ) {
	int s = glCreateShader(type);
	GLOGR("%d",s,"%d",type);
	if (s == 0) return NULL;
	return alloc_i32(s);
}

HL_PRIM void HL_NAME(gl_shader_source)( vdynamic *s, vstring *src ) {
	const GLchar *c = (GLchar*)hl_to_utf8(src->bytes);
	GLOG("%d,%s",s->v.i,c);
	glShaderSource(s->v.i, 1, &c, NULL);
}

HL_PRIM void HL_NAME(gl_compile_shader)( vdynamic *s ) {
	GLOG("%d",s->v.i);
	glCompileShader(s->v.i);
}

HL_PRIM vbyte *HL_NAME(gl_get_shader_info_bytes)( vdynamic *s ) {
	char log[4096];
	*log = 0;
	glGetShaderInfoLog(s->v.i, 4096, NULL, log);
	GLOGR("%s",log,"%d",s->v.i);
	return hl_copy_bytes((vbyte*)log, (int)strlen(log)+1);
}

HL_PRIM vdynamic *HL_NAME(gl_get_shader_parameter)( vdynamic *s, int param ) {
	switch( param ) {
	case 0x8B81/*COMPILE_STATUS*/:
	case 0x8B4F/*SHADER_TYPE*/:
	case 0x8B80/*DELETE_STATUS*/:
	{
		int ret = 0;
		glGetShaderiv(s->v.i, param, &ret);
		GLOGR("%d",ret,"%d,%d",s->v.i,param);
		return alloc_i32(ret);
	}
	default:
		hl_error_msg(USTR("Unsupported param %d"), param);
	}
	return NULL;
}

HL_PRIM void HL_NAME(gl_delete_shader)( vdynamic *s ) {
	GLOG("%d",s->v.i);
	glDeleteShader(s->v.i);
}

// texture

HL_PRIM vdynamic *HL_NAME(gl_create_texture)() {
	unsigned int t = 0;
	glGenTextures(1, &t);
	GLOGR("%d",t,"");
	return alloc_i32(t);
}

HL_PRIM void HL_NAME(gl_active_texture)( int t ) {
	GLOG("%d",t);
	glActiveTexture(t);
}

HL_PRIM void HL_NAME(gl_bind_texture)( int t, vdynamic *texture ) {
	GLOG("%d,%d",t,ZIDX(texture));
	glBindTexture(t, ZIDX(texture));
}

HL_PRIM void HL_NAME(gl_tex_parameteri)( int t, int key, int value ) {
	GLOG("%d,%d,%d",t,key,value);
	glTexParameteri(t, key, value);
}

HL_PRIM void HL_NAME(gl_tex_image2d)( int target, int level, int internalFormat, int width, int height, int border, int format, int type, vbyte *image ) {
	GLOG("%d,%d,%d,%d,%d,%d,%d,%d,%s",target,level,internalFormat,width,height,border,format,type,hexlog(image,16));
	glTexImage2D(target, level, internalFormat, width, height, border, format, type, image);
}

HL_PRIM void HL_NAME(gl_generate_mipmap)( int t ) {
	GLOG("%d",t);
	glGenerateMipmap(t);
}

HL_PRIM void HL_NAME(gl_delete_texture)( vdynamic *t ) {
	unsigned int tt = t->v.i;
	GLOG("%d",tt);
	glDeleteTextures(1, &tt);
}

// framebuffer

HL_PRIM vdynamic *HL_NAME(gl_create_framebuffer)() {
	unsigned int f = 0;
	glGenFramebuffers(1, &f);
	GLOGR("%d",f,"");
	return alloc_i32(f);
}

HL_PRIM void HL_NAME(gl_bind_framebuffer)( int target, vdynamic *f ) {
	GLOG("%d,%d",target,ZIDX(f));
	glBindFramebuffer(target, ZIDX(f));
}

HL_PRIM void HL_NAME(gl_framebuffer_texture2d)( int target, int attach, int texTarget, vdynamic *t, int level ) {
	GLOG("%d,%d,%d,%d,%d",target,attach,texTarget,ZIDX(t),level);
	glFramebufferTexture2D(target, attach, texTarget, ZIDX(t), level);
}

HL_PRIM void HL_NAME(gl_delete_framebuffer)( vdynamic *f ) {
	unsigned int ff = (unsigned)f->v.i;
	GLOG("%d",ff);
	glDeleteFramebuffers(1, &ff);
}

HL_PRIM void HL_NAME(gl_read_pixels)( int x, int y, int width, int height, int format, int type, vbyte *data ) {
	GLOG("%d,%d,%d,%d,%d,%d,%X",x,y,width,height,format,type,(int)(int_val)data);
	glReadPixels(x, y, width, height, format, type, data);
}

HL_PRIM void HL_NAME(gl_read_buffer)( int mode ) {
	GLOG("%d",mode);
	glReadBuffer(mode);
}

HL_PRIM void HL_NAME(gl_draw_buffers)( int count, unsigned int *buffers) {
	GLOG("%d",count);
	glDrawBuffers(count, buffers);
}

// renderbuffer

HL_PRIM vdynamic *HL_NAME(gl_create_renderbuffer)() {
	unsigned int buf = 0;
	glGenRenderbuffers(1, &buf);
	GLOGR("%d",buf,"");
	return alloc_i32(buf);
}

HL_PRIM void HL_NAME(gl_bind_renderbuffer)( int target, vdynamic *r ) {
	GLOG("%d,%d",target,ZIDX(r));
	glBindRenderbuffer(target, ZIDX(r));
}

HL_PRIM void HL_NAME(gl_renderbuffer_storage)( int target, int format, int width, int height ) {
	GLOG("%d,%d,%d,%d",target,format,width,height);
	glRenderbufferStorage(target, format, width, height);
}

HL_PRIM void HL_NAME(gl_framebuffer_renderbuffer)( int frameTarget, int attach, int renderTarget, vdynamic *b ) {
	GLOG("%d,%d,%d,%d",frameTarget,attach,renderTarget,ZIDX(b));
	glFramebufferRenderbuffer(frameTarget, attach, renderTarget, ZIDX(b));
}

HL_PRIM void HL_NAME(gl_delete_renderbuffer)( vdynamic *b ) {
	unsigned int bb = (unsigned)b->v.i;
	GLOG("%d",bb);
	glDeleteRenderbuffers(1, &bb);
}

// buffer

HL_PRIM vdynamic *HL_NAME(gl_create_buffer)() {
	unsigned int b = 0;
	glGenBuffers(1, &b);
	GLOGR("%d",b,"");
	return alloc_i32(b);
}

HL_PRIM void HL_NAME(gl_bind_buffer)( int target, vdynamic *b ) {
	GLOG("%d,%d",target,ZIDX(b));
	glBindBuffer(target, ZIDX(b));
}

HL_PRIM void HL_NAME(gl_buffer_data_size)( int target, int size, int param ) {
	GLOG("%d,%d,%d",target,size,param);
	glBufferData(target, size, NULL, param);
}

HL_PRIM void HL_NAME(gl_buffer_data)( int target, int size, vbyte *data, int param ) {
	GLOG("%d,%d,%s,%d",target,size,hexlog(data,size),param);
	glBufferData(target, size, data, param);
}

HL_PRIM void HL_NAME(gl_buffer_sub_data)( int target, int offset, vbyte *data, int srcOffset, int srcLength ) {
	GLOG("%d,%d,%s,%d",target,offset,hexlog(data+srcOffset,srcLength),srcLength);
	glBufferSubData(target, offset, srcLength, data + srcOffset);
}

HL_PRIM void HL_NAME(gl_enable_vertex_attrib_array)( int attrib ) {
	GLOG("%d",attrib);
	glEnableVertexAttribArray(attrib);
}

HL_PRIM void HL_NAME(gl_disable_vertex_attrib_array)( int attrib ) {
	GLOG("%d",attrib);
	glDisableVertexAttribArray(attrib);
}

HL_PRIM void HL_NAME(gl_vertex_attrib_pointer)( int index, int size, int type, bool normalized, int stride, int position ) {
	GLOG("%d,%d,%d,%d,%d,%d",index,size,type,normalized,stride,position);
	glVertexAttribPointer(index, size, type, normalized, stride, (void*)(int_val)position);
}

HL_PRIM void HL_NAME(gl_delete_buffer)( vdynamic *b ) {
	unsigned int bb = (unsigned)b->v.i;
	GLOG("%d",bb);
	glDeleteBuffers(1, &bb);
}

// uniforms

HL_PRIM void HL_NAME(gl_uniform1i)( vdynamic *u, int i ) {
	GLOG("%d,%d",u->v.i,i);
	glUniform1i(u->v.i, i);
}

HL_PRIM void HL_NAME(gl_uniform4fv)( vdynamic *u, vbyte *buffer, int bufPos, int count ) {
	GLOG("%d,%d",u->v.i,count);
#	ifdef GL_LOG
	if( gl_log_active ) {
		int i;
		fprintf(gl_log_out,"\t");
		for(i=0;i<count;i++) {
			fprintf(gl_log_out,"%g",((float*)buffer)[bufPos + i]);
			if( i < count-1 ) {
				fprintf(gl_log_out,",");
				if( ((i+1) & 3) == 0 ) fprintf(gl_log_out,"\n\t");
			}
		}
		fprintf(gl_log_out,"\n");
	}
#	endif
	glUniform4fv(u->v.i, count, (float*)buffer + bufPos);
}

// draw

HL_PRIM void HL_NAME(gl_draw_elements)( int mode, int count, int type, int start ) {
	GLOG("%d,%d,%d,%d",mode,count,type,start);
	glDrawElements(mode, count, type, (void*)(int_val)start);
}

// queries

HL_PRIM vdynamic *HL_NAME(gl_create_query)() {
	unsigned int t = 0;
	glGenQueries(1, &t);
	GLOGR("%d",t,"");
	return alloc_i32(t);
}

HL_PRIM void HL_NAME(gl_delete_query)( vdynamic *q ) {
	GLOG("%d",q->v.i);
	glDeleteQueries(1, (const GLuint *) &q->v.i);
}

HL_PRIM void HL_NAME(gl_begin_query)( int target, vdynamic *q ) {
	glBeginQuery(target,q->v.i);
}

HL_PRIM void HL_NAME(gl_end_query)( int target ) {
	glEndQuery(target);
}

HL_PRIM bool HL_NAME(gl_query_result_available)( vdynamic *q ) {
	int v = 0;
	glGetQueryObjectiv(q->v.i, GL_QUERY_RESULT_AVAILABLE, &v);
	return v == GL_TRUE;
}

HL_PRIM double HL_NAME(gl_query_result)( vdynamic *q ) {
	GLuint64 v = -1;
#	ifndef HL_MESA
	glGetQueryObjectui64v(q->v.i, GL_QUERY_RESULT, &v);
#	endif
	return (double)v;
}

HL_PRIM void HL_NAME(gl_query_counter)( vdynamic *q, int target ) {
#	ifndef HL_MESA
	glQueryCounter(q->v.i, target);
#	endif
}

// vertex array

HL_PRIM vdynamic *HL_NAME(gl_create_vertex_array)() {
	unsigned int f = 0;
	glGenVertexArrays(1, &f);
	GLOGR("%d",f,"");
	return alloc_i32(f);
}

HL_PRIM void HL_NAME(gl_bind_vertex_array)( vdynamic *b ) {
	unsigned int bb = (unsigned)b->v.i;
	GLOG("%d",bb);
	glBindVertexArray(bb);
}

HL_PRIM void HL_NAME(gl_delete_vertex_array)( vdynamic *b ) {
	unsigned int bb = (unsigned)b->v.i;
	GLOG("%d",bb);
	glDeleteVertexArrays(1, &bb);
}

DEFINE_PRIM(_BOOL,gl_init,_NO_ARG);
DEFINE_PRIM(_BOOL,gl_is_context_lost,_NO_ARG);
DEFINE_PRIM(_VOID,gl_clear,_I32);
DEFINE_PRIM(_I32,gl_get_error,_NO_ARG);
DEFINE_PRIM(_VOID,gl_scissor,_I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_clear_color,_F64 _F64 _F64 _F64);
DEFINE_PRIM(_VOID,gl_clear_depth,_F64);
DEFINE_PRIM(_VOID,gl_clear_stencil,_I32);
DEFINE_PRIM(_VOID,gl_viewport,_I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_finish,_NO_ARG);
DEFINE_PRIM(_VOID,gl_flush,_NO_ARG);
DEFINE_PRIM(_VOID,gl_pixel_storei,_I32 _I32);
DEFINE_PRIM(_BYTES,gl_get_string,_I32);
DEFINE_PRIM(_VOID,gl_enable,_I32);
DEFINE_PRIM(_VOID,gl_disable,_I32);
DEFINE_PRIM(_VOID,gl_cull_face,_I32);
DEFINE_PRIM(_VOID,gl_blend_func,_I32 _I32);
DEFINE_PRIM(_VOID,gl_blend_func_separate,_I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_blend_equation,_I32);
DEFINE_PRIM(_VOID,gl_blend_equation_separate,_I32 _I32);
DEFINE_PRIM(_VOID,gl_depth_mask,_BOOL);
DEFINE_PRIM(_VOID,gl_depth_func,_I32);
DEFINE_PRIM(_VOID,gl_color_mask,_BOOL _BOOL _BOOL _BOOL);
DEFINE_PRIM(_VOID,gl_stencil_mask_separate,_I32 _I32);
DEFINE_PRIM(_VOID,gl_stencil_func_separate,_I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_stencil_op_separate,_I32  _I32 _I32 _I32);
DEFINE_PRIM(_NULL(_I32),gl_create_program,_NO_ARG);
DEFINE_PRIM(_VOID,gl_delete_program,_NULL(_I32));
DEFINE_PRIM(_VOID,gl_bind_frag_data_location,_NULL(_I32) _I32 _STRING);
DEFINE_PRIM(_VOID,gl_attach_shader,_NULL(_I32) _NULL(_I32));
DEFINE_PRIM(_VOID,gl_link_program,_NULL(_I32));
DEFINE_PRIM(_DYN,gl_get_program_parameter,_NULL(_I32) _I32);
DEFINE_PRIM(_BYTES,gl_get_program_info_bytes,_NULL(_I32));
DEFINE_PRIM(_NULL(_I32),gl_get_uniform_location,_NULL(_I32) _STRING);
DEFINE_PRIM(_I32,gl_get_attrib_location,_NULL(_I32) _STRING);
DEFINE_PRIM(_VOID,gl_use_program,_NULL(_I32));
DEFINE_PRIM(_NULL(_I32),gl_create_shader,_I32);
DEFINE_PRIM(_VOID,gl_shader_source,_NULL(_I32) _STRING);
DEFINE_PRIM(_VOID,gl_compile_shader,_NULL(_I32));
DEFINE_PRIM(_BYTES,gl_get_shader_info_bytes,_NULL(_I32));
DEFINE_PRIM(_DYN,gl_get_shader_parameter,_NULL(_I32) _I32);
DEFINE_PRIM(_VOID,gl_delete_shader,_NULL(_I32));
DEFINE_PRIM(_NULL(_I32),gl_create_texture,_NO_ARG);
DEFINE_PRIM(_VOID,gl_active_texture,_I32);
DEFINE_PRIM(_VOID,gl_bind_texture,_I32 _NULL(_I32));
DEFINE_PRIM(_VOID,gl_tex_parameteri,_I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_tex_image2d,_I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID,gl_generate_mipmap,_I32);
DEFINE_PRIM(_VOID,gl_delete_texture,_NULL(_I32));
DEFINE_PRIM(_NULL(_I32),gl_create_framebuffer,_NO_ARG);
DEFINE_PRIM(_VOID,gl_bind_framebuffer,_I32 _NULL(_I32));
DEFINE_PRIM(_VOID,gl_framebuffer_texture2d,_I32 _I32 _I32 _NULL(_I32) _I32);
DEFINE_PRIM(_VOID,gl_delete_framebuffer,_NULL(_I32));
DEFINE_PRIM(_VOID,gl_read_pixels,_I32 _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID,gl_read_buffer,_I32);
DEFINE_PRIM(_VOID,gl_draw_buffers,_I32 _BYTES);
DEFINE_PRIM(_NULL(_I32),gl_create_renderbuffer,_NO_ARG);
DEFINE_PRIM(_VOID,gl_bind_renderbuffer,_I32 _NULL(_I32));
DEFINE_PRIM(_VOID,gl_renderbuffer_storage,_I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_framebuffer_renderbuffer,_I32 _I32 _I32 _NULL(_I32));
DEFINE_PRIM(_VOID,gl_delete_renderbuffer,_NULL(_I32));
DEFINE_PRIM(_NULL(_I32),gl_create_buffer,_NO_ARG);
DEFINE_PRIM(_VOID,gl_bind_buffer,_I32 _NULL(_I32));
DEFINE_PRIM(_VOID,gl_buffer_data_size,_I32 _I32 _I32);
DEFINE_PRIM(_VOID,gl_buffer_data,_I32 _I32 _BYTES _I32);
DEFINE_PRIM(_VOID,gl_buffer_sub_data,_I32 _I32 _BYTES _I32 _I32);
DEFINE_PRIM(_VOID,gl_enable_vertex_attrib_array,_I32);
DEFINE_PRIM(_VOID,gl_disable_vertex_attrib_array,_I32);
DEFINE_PRIM(_VOID,gl_vertex_attrib_pointer,_I32 _I32 _I32 _BOOL _I32 _I32);
DEFINE_PRIM(_VOID,gl_delete_buffer,_NULL(_I32));
DEFINE_PRIM(_VOID,gl_uniform1i,_NULL(_I32) _I32);
DEFINE_PRIM(_VOID,gl_uniform4fv,_NULL(_I32) _BYTES _I32 _I32);
DEFINE_PRIM(_VOID,gl_draw_elements,_I32 _I32 _I32 _I32);
DEFINE_PRIM(_NULL(_I32),gl_create_vertex_array,_NO_ARG);
DEFINE_PRIM(_VOID,gl_bind_vertex_array,_NULL(_I32));
DEFINE_PRIM(_VOID,gl_delete_vertex_array,_NULL(_I32));


DEFINE_PRIM(_NULL(_I32), gl_create_query, _NO_ARG);
DEFINE_PRIM(_VOID, gl_delete_query, _NULL(_I32));
DEFINE_PRIM(_VOID, gl_begin_query, _I32 _NULL(_I32));
DEFINE_PRIM(_VOID, gl_end_query, _I32);
DEFINE_PRIM(_BOOL, gl_query_result_available, _NULL(_I32));
DEFINE_PRIM(_VOID, gl_query_counter, _NULL(_I32) _I32);
DEFINE_PRIM(_F64, gl_query_result, _NULL(_I32));

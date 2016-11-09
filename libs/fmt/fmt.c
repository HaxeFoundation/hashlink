#define HL_NAME(n) fmt_##n
#ifdef _WIN32
#	include <turbojpeg.h>
#else
#	include <jpeglib.h>
#endif
#include <zlib.h>
#include <hl.h>
#include <png.h>

/* ------------------------------------------------- IMG --------------------------------------------------- */

typedef struct {
	unsigned char a,r,g,b;
} pixel;

HL_PRIM bool HL_NAME(jpg_decode)( vbyte *data, int dataLen, vbyte *out, int width, int height, int stride, int format, int flags ) {
#	ifdef _WIN32
	tjhandle h = tjInitDecompress();
	int result;
	result = tjDecompress2(h,data,dataLen,out,width,stride,height,format,(flags & 1 ? TJFLAG_BOTTOMUP : 0));
	tjDestroy(h);
	return result == 0;
#	else
	hl_error("JPG decode not yet supported on this OS");
	return false;
#	endif
}

HL_PRIM bool HL_NAME(png_decode)( vbyte *data, int dataLen, vbyte *out, int width, int height, int stride, int format, int flags ) {
	png_image img;
	memset(&img, 0, sizeof(img));
	img.version = PNG_IMAGE_VERSION;
	if( png_image_begin_read_from_memory(&img,data,dataLen) == 0 ) {
		png_image_free(&img);
		return false;
	}
	switch( format ) {
	case 0:
		img.format = PNG_FORMAT_RGB;
		break;
	case 1:
		img.format = PNG_FORMAT_BGR;
		break;
	case 7:
		img.format = PNG_FORMAT_RGBA;
		break;
	case 8:
		img.format = PNG_FORMAT_BGRA;
		break;
	case 9:
		img.format = PNG_FORMAT_ABGR;
		break;
	case 10:
		img.format = PNG_FORMAT_ARGB;
		break;
	default:
		png_image_free(&img);
		hl_error("Unsupported format");
		break;
	}
	if( img.width != width || img.height != height ) {
		png_image_free(&img);
		return false;
	}
	if( png_image_finish_read(&img,NULL,out,stride * (flags & 1 ? -1 : 1),NULL) == 0 ) {
		png_image_free(&img);
		return false;
	}
	png_image_free(&img);
	return true;
}

HL_PRIM void HL_NAME(img_scale)( vbyte *out, int outPos, int outStride, int outWidth, int outHeight, vbyte *in, int inPos, int inStride, int inWidth, int inHeight, int flags ) {
	int x, y;
	float scaleX = outWidth <= 1 ? 0.0f : (float)((inWidth - 1.001f) / (outWidth - 1));
	float scaleY = outHeight <= 1 ? 0.0f : (float)((inHeight - 1.001f) / (outHeight - 1));
	out += outPos;
	in += inPos;
	for(y=0;y<outHeight;y++) {
		for(x=0;x<outWidth;x++) {
			float fx = x * scaleX;
			float fy = y * scaleY;
			int ix = (int)fx;
			int iy = (int)fy;
			if( (flags & 1) == 0 ) {
				// nearest
				vbyte *rin = in + iy * inStride;
				*(pixel*)out = *(pixel*)(rin + (ix<<2));
				out += 4;
			} else {
				// bilinear
				float rx = fx - ix;
				float ry = fy - iy;
				float rx1 = 1.0f - rx;
				float ry1 = 1.0f - ry;
				int w1 = (int)(rx1 * ry1 * 256.0f + 0.5f);
				int w2 = (int)(rx * ry1 * 256.0f + 0.5f);
				int w3 = (int)(rx1 * ry * 256.0f + 0.5f);
				int w4 = (int)(rx * ry * 256.0f + 0.5f); 
				vbyte *rin = in + iy * inStride;
				pixel p1 = *(pixel*)(rin + (ix<<2));
				pixel p2 = *(pixel*)(rin + ((ix + 1)<<2));
				pixel p3 = *(pixel*)(rin + inStride + (ix<<2));
				pixel p4 = *(pixel*)(rin + inStride + ((ix + 1)<<2));
				*out++ = (unsigned char)((p1.a * w1 + p2.a * w2 + p3.a * w3 + p4.a * w4 + 128)>>8);
				*out++ = (unsigned char)((p1.r * w1 + p2.r * w2 + p3.r * w3 + p4.r * w4 + 128)>>8);
				*out++ = (unsigned char)((p1.g * w1 + p2.g * w2 + p3.g * w3 + p4.g * w4 + 128)>>8);
				*out++ = (unsigned char)((p1.b * w1 + p2.b * w2 + p3.b * w3 + p4.b * w4 + 128)>>8);
			}
		}
		out += outStride - (outWidth << 2);
	}
}


DEFINE_PRIM(_BOOL, jpg_decode, _BYTES _I32 _BYTES _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_BOOL, png_decode, _BYTES _I32 _BYTES _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, img_scale, _BYTES _I32 _I32 _I32 _I32 _BYTES _I32 _I32 _I32 _I32 _I32);


/* ------------------------------------------------- ZLIB --------------------------------------------------- */

typedef struct _fmt_zip fmt_zip;
struct _fmt_zip {
	void (*finalize)( fmt_zip * );
	z_stream *z;
	int flush;
	bool inflate;
};

static void free_stream_inf( fmt_zip *v ) {
	if( v->inflate )
		inflateEnd(v->z); // no error
	else
		deflateEnd(v->z);
	free(v->z);
	v->z = NULL;
	v->finalize = NULL;
}

static void zlib_error( z_stream *z, int err ) {
	hl_buffer *b = hl_alloc_buffer();
	vdynamic *d;
	hl_buffer_cstr(b, "ZLib Error : ");
	if( z && z->msg ) {
		hl_buffer_cstr(b,z->msg);
		hl_buffer_cstr(b," (");
	}
	d = hl_alloc_dynamic(&hlt_i32);
	d->v.i = err;
	hl_buffer_val(b,d);
	if( z && z->msg )
		hl_buffer_char(b,')');
	d = hl_alloc_dynamic(&hlt_bytes);
	d->v.ptr = hl_buffer_content(b,NULL);
	hl_throw(d);
}

HL_PRIM fmt_zip *HL_NAME(inflate_init)( int wbits ) {
	z_stream *z;
	int err;
	fmt_zip *s;
	if( wbits == 0 )
		wbits = MAX_WBITS;
	z = (z_stream*)malloc(sizeof(z_stream));
	memset(z,0,sizeof(z_stream));
	if( (err = inflateInit2(z,wbits)) != Z_OK ) {
		free(z);
		zlib_error(NULL,err);
	}
	s = (fmt_zip*)hl_gc_alloc_finalizer(sizeof(fmt_zip));
	s->finalize = free_stream_inf;
	s->flush = Z_NO_FLUSH;
	s->z = z;
	s->inflate = true;
	return s;
}

HL_PRIM fmt_zip *HL_NAME(deflate_init)( int level ) {
	z_stream *z;
	int err;
	fmt_zip *s;
	z = (z_stream*)malloc(sizeof(z_stream));
	memset(z,0,sizeof(z_stream));
	if( (err = deflateInit(z,level)) != Z_OK ) {
		free(z);
		zlib_error(NULL,err);
	}
	s = (fmt_zip*)hl_gc_alloc_finalizer(sizeof(fmt_zip));
	s->finalize = free_stream_inf;
	s->flush = Z_NO_FLUSH;
	s->z = z;
	s->inflate = false;
	return s;
}

HL_PRIM void HL_NAME(zip_end)( fmt_zip *z ) {
	free_stream_inf(z);
}

HL_PRIM void HL_NAME(zip_flush_mode)( fmt_zip *z, int flush ) {
	switch( flush ) {
	case 0:
		z->flush = Z_NO_FLUSH;
		break;
	case 1:
		z->flush = Z_SYNC_FLUSH;
		break;
	case 2:
		z->flush = Z_FULL_FLUSH;
		break;
	case 3:
		z->flush = Z_FINISH;
		break;
	case 4:
		z->flush = Z_BLOCK;
		break;
	default:
		hl_error_msg(USTR("Invalid flush mode %d"),flush);
		break;
	}
}

HL_PRIM bool HL_NAME(inflate_buffer)( fmt_zip *zip, vbyte *src, int srcpos, int srclen, vbyte *dst, int dstpos, int dstlen, int *read, int *write ) {
	int slen, dlen, err;
	z_stream *z = zip->z;
	slen = srclen - srcpos;
	dlen = dstlen - dstpos;
	if( srcpos < 0 || dstpos < 0 || slen < 0 || dlen < 0 )
		hl_error("Out of range");
	z->next_in = (Bytef*)(src + srcpos);
	z->next_out = (Bytef*)(dst + dstpos);
	z->avail_in = slen;
	z->avail_out = dlen;
	if( (err = inflate(z,zip->flush)) < 0 )
		zlib_error(z,err);
	z->next_in = NULL;
	z->next_out = NULL;
	*read = slen - z->avail_in;
	*write = dlen - z->avail_out;
	return err == Z_STREAM_END;
}

HL_PRIM bool HL_NAME(deflate_buffer)( fmt_zip *zip, vbyte *src, int srcpos, int srclen, vbyte *dst, int dstpos, int dstlen, int *read, int *write ) {
	int slen, dlen, err;
	z_stream *z = zip->z;
	slen = srclen - srcpos;
	dlen = dstlen - dstpos;
	if( srcpos < 0 || dstpos < 0 || slen < 0 || dlen < 0 )
		hl_error("Out of range");
	z->next_in = (Bytef*)(src + srcpos);
	z->next_out = (Bytef*)(dst + dstpos);
	z->avail_in = slen;
	z->avail_out = dlen;
	if( (err = deflate(z,zip->flush)) < 0 )
		zlib_error(z,err);
	z->next_in = NULL;
	z->next_out = NULL;
	*read = slen - z->avail_in;
	*write = dlen - z->avail_out;
	return err == Z_STREAM_END;
}

HL_PRIM int HL_NAME(deflate_bound)( fmt_zip *zip, int size ) {
	return deflateBound(zip->z,size);
}

#define _ZIP _ABSTRACT(fmt_zip)

DEFINE_PRIM(_ZIP, inflate_init, _I32);
DEFINE_PRIM(_ZIP, deflate_init, _I32);
DEFINE_PRIM(_I32, deflate_bound, _ZIP _I32);
DEFINE_PRIM(_VOID, zip_end, _ZIP);
DEFINE_PRIM(_VOID, zip_flush_mode, _ZIP _I32);
DEFINE_PRIM(_BOOL, inflate_buffer, _ZIP _BYTES _I32 _I32 _BYTES _I32 _I32 _REF(_I32) _REF(_I32));
DEFINE_PRIM(_BOOL, deflate_buffer, _ZIP _BYTES _I32 _I32 _BYTES _I32 _I32 _REF(_I32) _REF(_I32));

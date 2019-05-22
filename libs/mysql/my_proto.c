/*
 * MYSQL 5.0 Protocol Implementation
 * Copyright (C)2005-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "my_proto.h"

#define MAX_PACKET_LENGTH 0xFFFFFF

int myp_recv( MYSQL *m, void *buf, int size ) {
	while( size ) {
		int len = psock_recv(m->s,(char*)buf,size);
		if( len <= 0 ) return size == 0 ? 1 : 0;
		buf = ((char*)buf) + len;
		size -= len;
	}
	return 1;
}


int myp_send( MYSQL *m, void *buf, int size ) {
	while( size ) {
		int len = psock_send(m->s,(char*)buf,size);
		if( len <= 0 ) return size == 0 ? 1 : 0;
		buf = ((char*)buf) + len;
		size -= len;
	}
	return 1;
}

int myp_read( MYSQL_PACKET *p, void *buf, int size ) {
	if( p->size - p->pos < size ) {
		p->error = 1;
		return 0;
	}
	memcpy(buf,p->buf + p->pos,size);
	p->pos += size;
	return 1;
}

unsigned char myp_read_byte( MYSQL_PACKET *p ) {
	unsigned char c;
	if( !myp_read(p,&c,1) )
		return 0;
	return c;
}

unsigned short myp_read_ui16( MYSQL_PACKET *p ) {
	unsigned short i;
	if( !myp_read(p,&i,2) )
		return 0;
	return i;
}

int myp_read_int( MYSQL_PACKET *p ) {
	int i;
	if( !myp_read(p,&i,4) )
		return 0;
	return i;
}

int myp_read_bin( MYSQL_PACKET *p ) {
	int c = myp_read_byte(p);
	if( c <= 250 )
		return c;
	if( c == 251 )
		return -1; // NULL
	if( c == 252 )
		return myp_read_ui16(p);
	if( c == 253 ) {
		c = 0;
		myp_read(p,&c,3);
		return c;
	}
	if( c == 254 )
		return myp_read_int(p);
	p->error = 1;
	return 0;
}

const char *myp_read_string( MYSQL_PACKET *p ) {
	char *str;
	if( p->pos >= p->size ) {
		p->error = 1;
		return "";
	}
	str = p->buf + p->pos;
	p->pos += strlen(str) + 1;
	return str;
}

char *myp_read_bin_str( MYSQL_PACKET *p ) {
	int size = myp_read_bin(p);
	char *str;
	if( size == -1 )
		return NULL;
	if( p->error || p->pos + size > p->size ) {
		p->error = 1;
		return NULL;
	}
	str = (char*)malloc(size + 1);
	memcpy(str,p->buf + p->pos, size);
	str[size] = 0;
	p->pos += size;
	return str;
}

int myp_read_packet( MYSQL *m, MYSQL_PACKET *p ) {
	unsigned int psize;
	p->pos = 0;
	p->error = 0;
	if( !myp_recv(m,&psize,4) ) {
		p->error = 1;
		p->size = 0;
		return 0;
	}
	//p->id = (psize >> 24);
	psize &= 0xFFFFFF;
	p->size = psize;
	if( p->mem < (int)psize ) {
		free(p->buf);
		p->buf = (char*)malloc(psize + 1);
		p->mem = psize;
	}
	p->buf[psize] = 0;
	if( psize == 0 || !myp_recv(m,p->buf,psize) ) {
		p->error = 1;
		p->size = 0;
		p->buf[0] = 0;
		return 0;
	}
	return 1;
}

int myp_send_packet( MYSQL *m, MYSQL_PACKET *p, int *packet_counter ) {
	unsigned int header;
	char *buf = p->buf;
	int size = p->size;
	int next = 1;
	while( next ) {
		int psize;
		if( size >= MAX_PACKET_LENGTH )
			psize = MAX_PACKET_LENGTH;
		else {
			psize = size;
			next = 0;
		}
		header = psize | (((*packet_counter)++) << 24);
		if( !myp_send(m,&header,4) || !myp_send(m,buf,psize) ) {
			p->error = 1;
			return 0;
		}
		buf += psize;
		size -= psize;
	}
	return 1;
}

void myp_begin_packet( MYSQL_PACKET *p, int minsize ) {
	if( p->mem < minsize ) {
		free(p->buf);
		p->buf = (char*)malloc(minsize + 1);
		p->mem = minsize;
	}
	p->error = 0;
	p->size = 0;
}

void myp_write( MYSQL_PACKET *p, const void *data, int size ) {
	if( p->size + size > p->mem ) {
		char *buf2;
		if( p->mem == 0 ) p->mem = 32;
		do {
			p->mem <<= 1;
		} while( p->size + size > p->mem );
		buf2 = (char*)malloc(p->mem + 1);
		memcpy(buf2,p->buf,p->size);
		free(p->buf);
		p->buf = buf2;
	}
	memcpy( p->buf + p->size , data, size );
	p->size += size;
}

void myp_write_byte( MYSQL_PACKET *p, int i ) {
	unsigned char c = (unsigned char)i;
	myp_write(p,&c,1);
}

void myp_write_ui16( MYSQL_PACKET *p, int i ) {
	unsigned short c = (unsigned char)i;
	myp_write(p,&c,2);
}

void myp_write_int( MYSQL_PACKET *p, int i ) {
	myp_write(p,&i,4);
}

void myp_write_string( MYSQL_PACKET *p, const char *str ) {
	myp_write(p,str,strlen(str) + 1);
}

void myp_write_string_eof( MYSQL_PACKET *p, const char *str ) {
	myp_write(p,str,strlen(str));
}

void myp_write_bin( MYSQL_PACKET *p, int size ) {
	if( size <= 250 ) {
		unsigned char l = (unsigned char)size;
		myp_write(p,&l,1);
	} else if( size < 0x10000 ) {
		unsigned char c = 252;
		unsigned short l = (unsigned short)size;
		myp_write(p,&c,1);
		myp_write(p,&l,2);
	} else if( size < 0x1000000 ) {
		unsigned char c = 253;
		unsigned int l = (unsigned short)size;
		myp_write(p,&c,1);
		myp_write(p,&l,3);
	} else {
		unsigned char c = 254;
		myp_write(p,&c,1);
		myp_write(p,&size,4);
	}
}

void myp_crypt( unsigned char *out, const unsigned char *s1, const unsigned char *s2, unsigned int len ) {
	unsigned int i;
	for(i=0;i<len;i++)
		out[i] = s1[i] ^ s2[i];
}

void myp_encrypt_password( const char *pass, const char *seed, SHA1_DIGEST out ) {
	SHA1_CTX ctx;
	SHA1_DIGEST hash_stage1, hash_stage2;
	// stage 1: hash password
	sha1_init(&ctx);
	sha1_update(&ctx,(unsigned char*)pass,strlen(pass));;
	sha1_final(&ctx,hash_stage1);
	// stage 2: hash stage 1; note that hash_stage2 is stored in the database
	sha1_init(&ctx);
	sha1_update(&ctx, hash_stage1, SHA1_SIZE);
	sha1_final(&ctx, hash_stage2);
	// create crypt string as sha1(message, hash_stage2)
	sha1_init(&ctx);
	sha1_update(&ctx, (unsigned char*)seed, SHA1_SIZE);
	sha1_update(&ctx, hash_stage2, SHA1_SIZE);
	sha1_final( &ctx, out );
	// xor the result
	myp_crypt(out,out,hash_stage1,SHA1_SIZE);
}

typedef struct {
	unsigned long seed1;
	unsigned long seed2;
	unsigned long max_value;
	double max_value_dbl;
} rand_ctx;

static void random_init( rand_ctx *r, unsigned long seed1, unsigned long seed2 ) {
	r->max_value = 0x3FFFFFFFL;
	r->max_value_dbl = (double)r->max_value;
	r->seed1 = seed1 % r->max_value ;
	r->seed2 = seed2 % r->max_value;
}

static double myp_rnd( rand_ctx *r ) {
	r->seed1 = (r->seed1 * 3 + r->seed2) % r->max_value;
	r->seed2 = (r->seed1 + r->seed2 + 33) % r->max_value;
	return (((double) r->seed1)/r->max_value_dbl);
}

static void hash_password( unsigned long *result, const char *password, int password_len ) {
	register unsigned long nr = 1345345333L, add = 7, nr2 = 0x12345671L;
	unsigned long tmp;
	const char *password_end = password + password_len;
	for(; password < password_end; password++) {
		if( *password == ' ' || *password == '\t' )
			continue;
		tmp = (unsigned long)(unsigned char)*password;
		nr ^= (((nr & 63)+add)*tmp)+(nr << 8);
		nr2 += (nr2 << 8) ^ nr;
		add += tmp;
	}
	result[0] = nr & (((unsigned long) 1L << 31) -1L);
	result[1] = nr2 & (((unsigned long) 1L << 31) -1L);
}

void myp_encrypt_pass_323( const char *password, const char seed[SEED_LENGTH_323], char to[SEED_LENGTH_323] ) {
	rand_ctx r;
	unsigned long hash_pass[2], hash_seed[2];
	char extra, *to_start = to;
	const char *seed_end = seed + SEED_LENGTH_323;
	hash_password(hash_pass,password,(unsigned int)strlen(password));
	hash_password(hash_seed,seed,SEED_LENGTH_323);
	random_init(&r,hash_pass[0] ^ hash_seed[0],hash_pass[1] ^ hash_seed[1]);
	while( seed < seed_end ) {
		*to++ = (char)(floor(myp_rnd(&r)*31)+64);
		seed++;
	}
	extra= (char)(floor(myp_rnd(&r)*31));
	while( to_start != to )
		*(to_start++) ^= extra;
}

// defined in mysql/strings/ctype-*.c
const char *myp_charset_name( int charset ) {
	switch( charset ) {
	case 8:
	case 31:
	case 47:
		return "latin1";
	case 63:
		return "binary";
	// 101+ : utf16
	// 160+ : utf32
	case 33:
	case 83:
	case 223:
	case 254:
		return "utf8";
	case 45:
	case 46:
		return "utf8mb4"; // superset of utf8 with up to 4 bytes per-char
	default:
		if( charset >= 192 && charset <= 211 )
			return "utf8";
		if( charset >= 224 && charset <= 243 )
			return "utf8mb4";
	}
	return NULL;
}

int myp_supported_charset( int charset ) {
	return myp_charset_name(charset) != NULL;
}

int myp_escape_string( int charset, char *sout, const char *sin, int length ) {
	// this is safe for UTF8 as well since mysql protects against invalid UTF8 char injection
	const char *send = sin + length;
	char *sbegin = sout;
	while( sin != send ) {
		char c = *sin++;
		switch( c ) {
		case 0:
			*sout++ = '\\';
			*sout++ = '0';
			break;
		case '\n':
			*sout++ = '\\';
			*sout++ = 'n';
			break;
		case '\r':
			*sout++ = '\\';
			*sout++ = 'r';
			break;
		case '\\':
			*sout++ = '\\';
			*sout++ = '\\';
			break;
		case '\'':
			*sout++ = '\\';
			*sout++ = '\'';
			break;
		case '"':
			*sout++ = '\\';
			*sout++ = '"';
			break;
		case '\032':
			*sout++ = '\\';
			*sout++ = 'Z';
			break;
		default:
			*sout++ = c;
		}
	}
	*sout = 0;
	return sout - sbegin;
}

int myp_escape_quotes( int charset, char *sout, const char *sin, int length ) {
	const char *send = sin + length;
	char *sbegin = sout;
	while( sin != send ) {
		char c = *sin++;
		*sout++ = c;
		if( c == '\'' )
			*sout++ = c;
	}
	*sout = 0;
	return sout - sbegin;
}

/* ************************************************************************ */

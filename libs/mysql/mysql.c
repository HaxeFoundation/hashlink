/*
 * Copyright (C)2005-2018 Haxe Foundation
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
#define HL_NAME(n)	mysql_ ## n
#include <hl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mysql.h"
#include <string.h>

HL_PRIM void error( MYSQL *m, const char *msg ) {
	hl_buffer *b = hl_alloc_buffer();
	hl_buffer_cstr(b,msg);
	hl_buffer_cstr(b," ");
	hl_buffer_cstr(b,mysql_error(m));
	hl_throw_buffer(b);
}

// ---------------------------------------------------------------
// Result

#undef CONV_FLOAT
typedef enum {
	CONV_INT,
	CONV_STRING,
	CONV_FLOAT,
	CONV_BINARY,
	CONV_DATE,
	CONV_DATETIME,
	CONV_JSON,
	CONV_BOOL
} CONV;

typedef struct {
	void *free;
	MYSQL_RES *r;
	int nfields;
	CONV *fields_convs;
	int *fields_ids;
	MYSQL_ROW current;
} result;

typedef struct {
	void *free;
	MYSQL *c;
} connection;

static vclosure *conv_string = NULL;
static vclosure *conv_json = NULL;
static vclosure *conv_bytes = NULL;
static vclosure *conv_date = NULL;

static void free_result( result *r ) {
	mysql_free_result(r->r);
	free(r->fields_ids);
	free(r->fields_convs);
}

HL_PRIM int HL_NAME(result_get_length)( result *r ) {
	if( r->r == NULL )
		return r->nfields;	
	return (int)mysql_num_rows(r->r);
}

HL_PRIM int HL_NAME(result_get_nfields)( result *r ) {
	return r->nfields;
}

HL_PRIM varray *HL_NAME(result_get_fields_names)( result *r ) {
	int k;
	MYSQL_FIELD *fields = mysql_fetch_fields(r->r);
	varray *a = hl_alloc_array(&hlt_bytes,r->nfields);
	for(k=0;k<r->nfields;k++)
		hl_aptr(a,vbyte*)[k] = (vbyte*)fields[k].name;
	return a;
}

HL_PRIM vdynamic *HL_NAME(result_next)( result *r ) {
	unsigned long *lengths = NULL;
	MYSQL_ROW row = mysql_fetch_row(r->r);
	if( row == NULL )
		return NULL;
	int i;
	struct tm t;
	vdynamic *obj = (vdynamic*)hl_alloc_dynobj();
	vdynamic arg;
	vdynamic length;
	vdynamic *pargs[2];	
	pargs[0] = &arg;
	pargs[1] = &length;
	length.t = &hlt_i32;
	r->current = row;
	for(i=0;i<r->nfields;i++) {
		if( row[i] == NULL ) continue;		
		vdynamic *value = NULL;
		switch( r->fields_convs[i] ) {
		case CONV_INT:
			hl_dyn_seti(obj, r->fields_ids[i], &hlt_i32, atoi(row[i]));
			break;
		case CONV_STRING:
			arg.t = &hlt_bytes;
			arg.v.ptr = row[i];
			value = hl_dyn_call(conv_string, pargs, 1);
			break;
		case CONV_JSON:
			arg.t = &hlt_bytes;
			arg.v.ptr = row[i];
			value = hl_dyn_call(conv_json, pargs, 1);
			break;
		case CONV_BOOL:
			hl_dyn_seti(obj, r->fields_ids[i], &hlt_bool, (int)(*row[i] != '0'));
			break;
		case CONV_FLOAT:
			hl_dyn_setd(obj, r->fields_ids[i], atof(row[i]));
			break;
		case CONV_BINARY:
			if( lengths == NULL ) {
				lengths = mysql_fetch_lengths(r->r);
				if( lengths == NULL ) hl_error("mysql_fetch_lengths");
			}
			arg.t = &hlt_bytes;
			arg.v.ptr = row[i];
			length.v.i = lengths[i];
			value = hl_dyn_call(conv_bytes, pargs, 2);
			break;
		case CONV_DATE:
			sscanf(row[i],"%4d-%2d-%2d",&t.tm_year,&t.tm_mon,&t.tm_mday);
			t.tm_hour = 0;
			t.tm_min = 0;
			t.tm_sec = 0;
			t.tm_isdst = -1;
			t.tm_year -= 1900;
			t.tm_mon--;
			arg.t = &hlt_i32;
			arg.v.i = (int)mktime(&t);
			value = hl_dyn_call(conv_date,pargs, 1);
			break;
		case CONV_DATETIME:
			sscanf(row[i],"%4d-%2d-%2d %2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec);
			t.tm_isdst = -1;
			t.tm_year -= 1900;
			t.tm_mon--;
			arg.t = &hlt_i32;
			arg.v.i = (int)mktime(&t);
			value = hl_dyn_call(conv_date,pargs, 1);
			break;
		default:
			break;
		}
		if( value ) hl_dyn_setp(obj, r->fields_ids[i], &hlt_dyn, value);

	}
	return obj;
}


HL_PRIM vbyte *HL_NAME(result_get)( result *r, int n ) {
	const char *str;
	if( n < 0 || n >= r->nfields )
		return NULL;
	if( !r->current ) {
		HL_NAME(result_next)(r);
		if( !r->current )
			return NULL;
	}
	str = r->current[n];
	return (vbyte*)(str ? str : "");
}

HL_PRIM int HL_NAME(result_get_int)( result *r, int n ) {
	const char *str;
	if( n < 0 || n >= r->nfields )
		return 0;
	if( !r->current ) {
		HL_NAME(result_next)(r);
		if( !r->current )
			return 0;
	}
	str = r->current[n];
	return str ? atoi(str) : 0;
}

HL_PRIM double HL_NAME(result_get_float)( result *r, int n ) {
	const char *str;
	if( n < 0 || n >= r->nfields )
		return 0.;
	if( !r->current ) {
		HL_NAME(result_next)(r);
		if( !r->current )
			return 0.;
	}
	str = r->current[n];
	return str ? atof(str) : 0.;
}

static CONV convert_type( enum enum_field_types t, int flags, unsigned int length ) {
	// FIELD_TYPE_TIME
	// FIELD_TYPE_YEAR
	// FIELD_TYPE_NEWDATE
	// FIELD_TYPE_NEWDATE + 2: // 5.0 MYSQL_TYPE_BIT
	switch( t ) {
	case FIELD_TYPE_TINY:
		if( length == 1 )
			return CONV_BOOL;
	case FIELD_TYPE_SHORT:
	case FIELD_TYPE_LONG:
	case FIELD_TYPE_INT24:
		return CONV_INT;
	case FIELD_TYPE_LONGLONG:
	case FIELD_TYPE_DECIMAL:
	case FIELD_TYPE_FLOAT:
	case FIELD_TYPE_DOUBLE:
	case 246: // 5.0 MYSQL_NEW_DECIMAL
		return CONV_FLOAT;
	case 245: // JSON
		return CONV_JSON;
	case FIELD_TYPE_BLOB:
	case FIELD_TYPE_TINY_BLOB:
	case FIELD_TYPE_MEDIUM_BLOB:
	case FIELD_TYPE_LONG_BLOB:
		if( (flags & BINARY_FLAG) != 0 )
			return CONV_BINARY;
		return CONV_STRING;
	case FIELD_TYPE_DATETIME:
	case FIELD_TYPE_TIMESTAMP:
		return CONV_DATETIME;
	case FIELD_TYPE_DATE:
		return CONV_DATE;
	case FIELD_TYPE_NULL:
	case FIELD_TYPE_ENUM:
	case FIELD_TYPE_SET:
	//case FIELD_TYPE_VAR_STRING:
	//case FIELD_TYPE_GEOMETRY:
	// 5.0 MYSQL_TYPE_VARCHAR
	default:
		if( (flags & BINARY_FLAG) != 0 )
			return CONV_BINARY;
		return CONV_STRING;
	}
}

static result *alloc_result( connection *c, MYSQL_RES *r ) {
	result *res = (result*)hl_gc_alloc_finalizer(sizeof(result));
	int num_fields = mysql_num_fields(r);
	int i;
	MYSQL_FIELD *fields = mysql_fetch_fields(r);
	res->free = free_result;
	res->r = r;
	res->current = NULL;
	res->nfields = num_fields;
	res->fields_ids = (int*)malloc(sizeof(int)*num_fields);
	res->fields_convs = (CONV*)malloc(sizeof(CONV)*num_fields);	
	for(i=0;i<num_fields;i++) {
		int id;
		if( strchr(fields[i].name,'(') )
			id = hl_hash_gen(USTR("???"),true); // looks like an inner request : prevent hashing + cashing it
		else
			id = hl_hash_gen(hl_to_utf16(fields[i].name),true);
		res->fields_ids[i] = id;
		res->fields_convs[i] = convert_type(fields[i].type,fields[i].flags,fields[i].length);
	}
	return res;
}

// ---------------------------------------------------------------
// Connection

HL_PRIM void HL_NAME(close_wrap)( connection *c ) {	
	if( c->c ) {
		mp_close(c->c);
		c->c = NULL;
	}
}

HL_PRIM bool HL_NAME(select_db_wrap)( connection *c, const char *db ) {
	return mysql_select_db(c->c,db) == 0;
}

HL_PRIM result *HL_NAME(request)( connection *c, const char *rq, int rqLen  ) {
	if( mysql_real_query(c->c,rq,rqLen) != 0 )
		error(c->c,rq);
	MYSQL_RES *res = mysql_store_result(c->c);
	if( res == NULL ) {
		if( mysql_field_count(c->c) != 0 )
			error(c->c,rq);
		result *r = (result*)hl_gc_alloc_noptr(sizeof(result));
		memset(r,0,sizeof(result));
		r->nfields = (int)mysql_affected_rows(c->c);
		return r;
	}
	return alloc_result(c,res);
}


HL_PRIM vbyte *HL_NAME(escape)( connection *c, const char *str, int len ) {
	int wlen = len * 2;
	vbyte *sout = hl_gc_alloc_noptr(wlen+1);
	wlen = mysql_real_escape_string(c->c,(char*)sout,str,len);
	if( wlen < 0 ) {
		hl_buffer *b = hl_alloc_buffer();
		hl_buffer_cstr(b,"Unsupported charset : ");
		hl_buffer_cstr(b,mysql_character_set_name(c->c));
		hl_throw_buffer(b);
	}
	sout[wlen] = 0;
	return sout;
}

typedef struct {
	hl_type *t;
	const char *host;
	const char *user;
	const char *pass;
	const char *socket;
	int port;
} cnx_params;

HL_PRIM connection *HL_NAME(connect_wrap)( cnx_params *p ) {
	connection *c = (connection*)hl_gc_alloc_finalizer(sizeof(connection));
	memset(c,0,sizeof(connection));
	c->free = HL_NAME(close_wrap);
	c->c = mysql_init(NULL);
	if( mysql_real_connect(c->c,p->host,p->user,p->pass,NULL,p->port,p->socket,0) == NULL ) {
		hl_buffer *b = hl_alloc_buffer();
		hl_buffer_cstr(b, "Failed to connect to mysql server : ");
		hl_buffer_cstr(b,mysql_error(c->c));
		mysql_close(c->c);
		hl_throw_buffer(b);
	}
	return c;
}

HL_PRIM void HL_NAME(set_conv_funs)( vclosure *fstring, vclosure *fbytes, vclosure *fdate, vclosure *fjson ) {
	conv_string = fstring;
	conv_bytes = fbytes;
	conv_date = fdate;
	conv_json = fjson;
}

// ---------------------------------------------------------------
// Registers

#define _CNX _ABSTRACT(mysql_cnx)
#define _RESULT _ABSTRACT(mysql_result)

DEFINE_PRIM(_CNX, connect_wrap, _OBJ(_BYTES _BYTES _BYTES _BYTES _I32) );
DEFINE_PRIM(_VOID, close_wrap, _CNX);
DEFINE_PRIM(_RESULT, request, _CNX _BYTES _I32);
DEFINE_PRIM(_BOOL, select_db_wrap, _CNX _BYTES);
DEFINE_PRIM(_BYTES, escape, _CNX _BYTES _I32);

DEFINE_PRIM(_I32, result_get_length, _RESULT);
DEFINE_PRIM(_I32, result_get_nfields, _RESULT);
DEFINE_PRIM(_ARR, result_get_fields_names, _RESULT);
DEFINE_PRIM(_DYN, result_next, _RESULT);
DEFINE_PRIM(_BYTES, result_get, _RESULT _I32);
DEFINE_PRIM(_I32, result_get_int, _RESULT _I32);
DEFINE_PRIM(_F64, result_get_float, _RESULT _I32);

DEFINE_PRIM(_VOID, set_conv_funs, _DYN _DYN _DYN _DYN);

/* ************************************************************************ */

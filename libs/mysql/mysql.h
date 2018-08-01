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
#ifndef MYSQL_H
#define MYSQL_H

struct _MYSQL;
struct _MYSQL_RES;
typedef struct _MYSQL MYSQL; 
typedef struct _MYSQL_RES MYSQL_RES;
typedef char **MYSQL_ROW;

typedef enum enum_field_types {
	FIELD_TYPE_DECIMAL = 0x00,
	FIELD_TYPE_TINY = 0x01,
	FIELD_TYPE_SHORT = 0x02,
	FIELD_TYPE_LONG = 0x03,
	FIELD_TYPE_FLOAT = 0x04,
	FIELD_TYPE_DOUBLE = 0x05,
	FIELD_TYPE_NULL = 0x06,
	FIELD_TYPE_TIMESTAMP = 0x07,
	FIELD_TYPE_LONGLONG = 0x08,
	FIELD_TYPE_INT24 = 0x09,
	FIELD_TYPE_DATE = 0x0A,
	FIELD_TYPE_TIME = 0x0B,
	FIELD_TYPE_DATETIME = 0x0C,
	FIELD_TYPE_YEAR = 0x0D,
	FIELD_TYPE_NEWDATE = 0x0E,
	FIELD_TYPE_VARCHAR = 0x0F,
	FIELD_TYPE_BIT = 0x10,
	FIELD_TYPE_NEWDECIMAL = 0xF6,
	FIELD_TYPE_ENUM = 0xF7,
	FIELD_TYPE_SET = 0xF8,
	FIELD_TYPE_TINY_BLOB = 0xF9,
	FIELD_TYPE_MEDIUM_BLOB = 0xFA,
	FIELD_TYPE_LONG_BLOB = 0xFB,
	FIELD_TYPE_BLOB = 0xFC,
	FIELD_TYPE_VAR_STRING = 0xFD,
	FIELD_TYPE_STRING = 0xFE,
	FIELD_TYPE_GEOMETRY = 0xFF
} FIELD_TYPE;

typedef enum {
	NOT_NULL_FLAG =	1,
	PRI_KEY_FLAG = 2,
	UNIQUE_KEY_FLAG = 4,
	MULTIPLE_KEY_FLAG = 8,
	BLOB_FLAG = 16,
	UNSIGNED_FLAG = 32,
	ZEROFILL_FLAG = 64,
	BINARY_FLAG	= 128,
	ENUM_FLAG = 256,
	AUTO_INCREMENT_FLAG = 512,
	TIMESTAMP_FLAG = 1024,
	SET_FLAG = 2048,
	NUM_FLAG = 32768,
} __FIELD_FLAG;

typedef struct {
	char *catalog;
	char *db;
	char *table;
	char *org_table;
	char *name;
	char *org_name;
	int charset;
	int length;
	int flags;
	int decimals;
	FIELD_TYPE type;
} MYSQL_FIELD;

#define	mysql_init			mp_init
#define mysql_real_connect	mp_real_connect
#define mysql_select_db		mp_select_db
#define mysql_real_query	mp_real_query
#define mysql_store_result	mp_store_result
#define mysql_field_count	mp_field_count
#define mysql_affected_rows	mp_affected_rows
#define mysql_escape_string	mp_escape_string
#define mysql_real_escape_string mp_real_escape_string
#define mysql_close			mp_close
#define mysql_error			mp_error
#define mysql_num_rows		mp_num_rows
#define mysql_num_fields	mp_num_fields
#define mysql_fetch_fields	mp_fetch_fields
#define mysql_fetch_lengths	mp_fetch_lengths
#define mysql_fetch_row		mp_fetch_row
#define mysql_free_result	mp_free_result

MYSQL *mysql_init( void * );
MYSQL *mysql_real_connect( MYSQL *m, const char *host, const char *user, const char *pass, void *unused, int port, const char *socket, int options );
int mysql_select_db( MYSQL *m, const char *dbname );
int mysql_real_query( MYSQL *m, const char *query, int qlength );
MYSQL_RES *mysql_store_result( MYSQL *m );
int mysql_field_count( MYSQL *m );
int mysql_affected_rows( MYSQL *m );
int mysql_escape_string( MYSQL *m, char *sout, const char *sin, int length );
int mysql_real_escape_string( MYSQL *m, char *sout, const char *sin, int length );
void mysql_close( MYSQL *m );
const char *mysql_error( MYSQL *m );
const char *mysql_character_set_name( MYSQL *m );

unsigned int mysql_num_rows( MYSQL_RES *r );
int mysql_num_fields( MYSQL_RES *r );
MYSQL_FIELD *mysql_fetch_fields( MYSQL_RES *r );
unsigned long *mysql_fetch_lengths( MYSQL_RES *r );
MYSQL_ROW mysql_fetch_row( MYSQL_RES * r );
void mysql_free_result( MYSQL_RES *r );

#endif
/* ************************************************************************ */

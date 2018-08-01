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
#ifndef MY_PROTO_H
#define MY_PROTO_H

#include "mysql.h"
#include "socket.h"
#include "sha1.h"

typedef enum {
	FL_LONG_PASSWORD = 1,
	FL_FOUND_ROWS = 2,
	FL_LONG_FLAG = 4,
	FL_CONNECT_WITH_DB = 8,
	FL_NO_SCHEMA = 16,
	FL_COMPRESS = 32,
	FL_ODBC = 64,
	FL_LOCAL_FILES = 128,
	FL_IGNORE_SPACE	= 256,
	FL_PROTOCOL_41 = 512,
	FL_INTERACTIVE = 1024,
	FL_SSL = 2048,
	FL_IGNORE_SIGPIPE = 4096,
	FL_TRANSACTIONS = 8192,
	FL_RESERVED = 16384,
	FL_SECURE_CONNECTION = 32768,
	FL_MULTI_STATEMENTS  = 65536,
	FL_MULTI_RESULTS = 131072,
} MYSQL_FLAG;

typedef enum {
	COM_SLEEP = 0x00,
	COM_QUIT = 0x01,
	COM_INIT_DB = 0x02,
	COM_QUERY = 0x03,
	COM_FIELD_LIST = 0x04,
	//COM_CREATE_DB = 0x05,
	//COM_DROP_DB = 0x06
	COM_REFRESH = 0x07,
	COM_SHUTDOWN = 0x08,
	COM_STATISTICS = 0x09,
	COM_PROCESS_INFO = 0x0A,
	//COM_CONNECT = 0x0B,
	COM_PROCESS_KILL = 0x0C,
	COM_DEBUG = 0x0D,
	COM_PING = 0x0E,
	//COM_TIME = 0x0F,
	//COM_DELAYED_INSERT = 0x10,
	COM_CHANGE_USER = 0x11,
	COM_BINLOG_DUMP = 0x12,
	COM_TABLE_DUMP = 0x13,
	COM_CONNECT_OUT = 0x14,
	COM_REGISTER_SLAVE = 0x15,
	COM_STMT_PREPARE = 0x16,
	COM_STMT_EXECUTE = 0x17,
	COM_STMT_SEND_LONG_DATA = 0x18,
	COM_STMT_CLOSE = 0x19,
	COM_STMT_RESET = 0x1A,
	COM_SET_OPTION = 0x1B,
	COM_STMT_FETCH = 0x1C,
} MYSQL_COMMAND;

typedef enum {
	SERVER_STATUS_IN_TRANS = 1,
	SERVER_STATUS_AUTOCOMMIT = 2,
	SERVER_MORE_RESULTS_EXISTS = 8,
	SERVER_QUERY_NO_GOOD_INDEX_USED = 16,
	SERVER_QUERY_NO_INDEX_USED = 32,
	SERVER_STATUS_CURSOR_EXISTS = 64,
	SERVER_STATUS_LAST_ROW_SENT = 128,
	SERVER_STATUS_DB_DROPPED = 256,
	SERVER_STATUS_NO_BACKSLASH_ESCAPES = 512,
} MYSQL_SERVER_STATUS;

typedef struct {
	unsigned char proto_version;
	char *server_version;
	unsigned int thread_id;
	unsigned int server_flags;
	unsigned char server_charset;
	unsigned short server_status;
} MYSQL_INFOS;

typedef struct {
	int id;
	int error;
	int size;
	int pos;
	int mem;
	char *buf;
} MYSQL_PACKET;

#define MAX_ERR_SIZE	1024
#define	IS_QUERY		-123456

struct _MYSQL {
	PSOCK s;
	MYSQL_INFOS infos;
	MYSQL_PACKET packet;
	int is41;
	int errcode;
	int last_field_count;
	int affected_rows;
	int last_insert_id;
	char last_error[MAX_ERR_SIZE];
};

typedef struct {
	char *raw;
	unsigned long *lengths;
	char **datas;
} MYSQL_ROW_DATA;

struct _MYSQL_RES {
	int nfields;
	MYSQL_FIELD *fields;
	MYSQL_ROW_DATA *rows;
	MYSQL_ROW_DATA *current;
	int row_count;
	int memory_rows;
};


// network
int myp_recv( MYSQL *m, void *buf, int size );
int myp_send( MYSQL *m, void *buf, int size );
int myp_read_packet( MYSQL *m, MYSQL_PACKET *p );
int myp_send_packet( MYSQL *m, MYSQL_PACKET *p, int *packet_counter );

// packet read
int myp_read( MYSQL_PACKET *p, void *buf, int size );
unsigned char myp_read_byte( MYSQL_PACKET *p );
unsigned short myp_read_ui16( MYSQL_PACKET *p );
int myp_read_int( MYSQL_PACKET *p );
const char *myp_read_string( MYSQL_PACKET *p );
int myp_read_bin( MYSQL_PACKET *p );
char *myp_read_bin_str( MYSQL_PACKET *p );

// packet write
void myp_begin_packet( MYSQL_PACKET *p, int minsize );
void myp_write( MYSQL_PACKET *p, const void *data, int size );
void myp_write_byte( MYSQL_PACKET *p, int b );
void myp_write_ui16( MYSQL_PACKET *p, int b );
void myp_write_int( MYSQL_PACKET *p, int b );

void myp_write_string( MYSQL_PACKET *p, const char *str );
void myp_write_string_eof( MYSQL_PACKET *p, const char *str );
void myp_write_bin( MYSQL_PACKET *p, int size );

// passwords
#define SEED_LENGTH_323 8

void myp_crypt( unsigned char *out, const unsigned char *s1, const unsigned char *s2, unsigned int len );
void myp_encrypt_password( const char *pass, const char *seed, SHA1_DIGEST out );
void myp_encrypt_pass_323( const char *pass, const char seed[SEED_LENGTH_323], char out[SEED_LENGTH_323] );

// escaping
int myp_supported_charset( int charset );
const char *myp_charset_name( int charset );
int myp_escape_string( int charset, char *sout, const char *sin, int length );
int myp_escape_quotes( int charset, char *sout, const char *sin, int length );

#endif
/* ************************************************************************ */

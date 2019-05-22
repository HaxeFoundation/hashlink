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
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "my_proto.h"

#ifdef OS_WINDOWS
#define strdup _strdup
#endif

static void error( MYSQL *m, const char *err, const char *param ) {
	if( param ) {
		unsigned int max = MAX_ERR_SIZE - (strlen(err) + 3);
		if( strlen(param) > max ) {
			char *p2 = (char*)malloc(max + 1);
			memcpy(p2,param,max-3);
			p2[max - 3] = '.';
			p2[max - 2] = '.';
			p2[max - 1] = '.';
			p2[max] = 0;
			sprintf(m->last_error,err,param);
			free(p2);
			return;
		}
	}
	sprintf(m->last_error,err,param);
	m->errcode = -1;
}

static void save_error( MYSQL *m, MYSQL_PACKET *p ) {
	int ecode;
	p->pos = 0;
	// seems like we sometimes get some FFFFFF sequences before
	// the actual error...
	do {
		if( myp_read_byte(p) != 0xFF ) {
			m->errcode = -1;
			error(m,"Failed to decode error",NULL);
			return;
		}
		ecode = myp_read_ui16(p);
	} while( ecode == 0xFFFF );
	if( m->is41 && p->buf[p->pos] == '#' )
		p->pos += 6; // skip sqlstate marker
	error(m,"%s",myp_read_string(p));
	m->errcode = ecode;
}

static int myp_ok( MYSQL *m, int allow_others ) {
	int code;
	MYSQL_PACKET *p = &m->packet;
	if( !myp_read_packet(m,p) ) {
		error(m,"Failed to read packet",NULL);
		return 0;
	}
	code = myp_read_byte(p);
	if( code == 0x00 )
		return 1;
	if( code == 0xFF )
		save_error(m,p);
	else if( allow_others )
		return 1;
	else
		error(m,"Invalid packet error",NULL);
	return 0;
}

static void myp_close( MYSQL *m ) {
	psock_close(m->s);
	m->s = INVALID_SOCKET;
}

MYSQL *mysql_init( void *unused ) {
	MYSQL *m = (MYSQL*)malloc(sizeof(struct _MYSQL));
	psock_init();
	memset(m,0,sizeof(struct _MYSQL));
	m->s = INVALID_SOCKET;
	error(m,"NO ERROR",NULL);
	m->errcode = 0;
	m->last_field_count = -1;
	m->last_insert_id = -1;
	m->affected_rows = -1;
	return m;
}

MYSQL *mysql_real_connect( MYSQL *m, const char *host, const char *user, const char *pass, void *unused, int port, const char *socket, int options ) {
	PHOST h;
	char scramble_buf[21];
	MYSQL_PACKET *p = &m->packet;
	int pcount = 1;
	if( socket && *socket ) {
		error(m,"Unix Socket connections are not supported",NULL);
		return NULL;
	}
	h = phost_resolve(host);
	if( h == UNRESOLVED_HOST ) {
		error(m,"Failed to resolve host '%s'",host);
		return NULL;
	}
	m->s = psock_create();
	if( m->s == INVALID_SOCKET ) {
		error(m,"Failed to create socket",NULL);
		return NULL;
	}
	psock_set_fastsend(m->s,1);
	psock_set_timeout(m->s,50); // 50 seconds
	if( psock_connect(m->s,h,port) != PS_OK ) {
		myp_close(m);
		error(m,"Failed to connect on host '%s'",host);
		return NULL;
	}
	if( !myp_read_packet(m,p) ) {
		myp_close(m);
		error(m,"Failed to read handshake packet",NULL);
		return NULL;
	}
	// process handshake packet
	{
		char filler[13];
		m->infos.proto_version = myp_read_byte(p);
		// this seems like an error packet
		if( m->infos.proto_version == 0xFF ) {
			myp_close(m);
			save_error(m,p);
			return NULL;
		}	
		m->infos.server_version = strdup(myp_read_string(p));
		m->infos.thread_id = myp_read_int(p);
		myp_read(p,scramble_buf,8);
		myp_read_byte(p); // should be 0
		m->infos.server_flags = myp_read_ui16(p);
		m->infos.server_charset = myp_read_byte(p);
		m->infos.server_status = myp_read_ui16(p);
		m->infos.server_flags |= myp_read_ui16(p) << 16;
		myp_read_byte(p); // len
		myp_read(p,filler,10);
		// try to disable 41
		m->is41 = (m->infos.server_flags & FL_PROTOCOL_41) != 0;
		if( !p->error && m->is41 )
			myp_read(p,scramble_buf + 8,13);
		if( p->pos != p->size )
			myp_read_string(p); // 5.5+
		if( p->error ) {
			myp_close(m);
			error(m,"Failed to decode server handshake",NULL);
			return NULL;
		}
		// fill answer packet
		{
			unsigned int flags = m->infos.server_flags;
			int max_packet_size = 0x01000000;
			SHA1_DIGEST hpass;
			char filler[23];
			flags &= (FL_PROTOCOL_41 | FL_TRANSACTIONS | FL_SECURE_CONNECTION);
			myp_begin_packet(p,128);
			if( m->is41 ) {
				myp_write_int(p,flags);
				myp_write_int(p,max_packet_size);
				myp_write_byte(p,m->infos.server_charset);
				memset(filler,0,23);
				myp_write(p,filler,23);
				myp_write_string(p,user);
				if( *pass ) {
					myp_encrypt_password(pass,scramble_buf,hpass);
					myp_write_bin(p,SHA1_SIZE);
					myp_write(p,hpass,SHA1_SIZE);
					myp_write_byte(p,0);
				} else
					myp_write_bin(p,0);
			} else {
				myp_write_ui16(p,flags);
				// max_packet_size
				myp_write_byte(p,0xFF);
				myp_write_byte(p,0xFF);
				myp_write_byte(p,0xFF);
				myp_write_string(p,user);
				if( *pass ) {
					char hpass[SEED_LENGTH_323 + 1];
					myp_encrypt_pass_323(pass,scramble_buf,hpass);
					hpass[SEED_LENGTH_323] = 0;
					myp_write(p,hpass,SEED_LENGTH_323 + 1);
				} else
					myp_write_bin(p,0);
			}
		}
	}
	// send connection packet
send_cnx_packet:
	if( !myp_send_packet(m,p,&pcount) ) {
		myp_close(m);
		error(m,"Failed to send connection packet",NULL);
		return NULL;
	}
	// read answer packet
	if( !myp_read_packet(m,p) ) {
		myp_close(m);
		error(m,"Failed to read packet",NULL);
		return NULL;
	}
	// increase packet counter (because we read one packet)
	pcount++;
	// process answer
	{
		int code = myp_read_byte(p);
		switch( code ) {
		case 0: // OK packet
			break;
		case 0xFF: // ERROR
			myp_close(m);
			save_error(m,p);
			return NULL;
		case 0xFE: // EOF
			// we are asked to send old password authentification
			if( p->size == 1 ) {
				char hpass[SEED_LENGTH_323 + 1];
				myp_encrypt_pass_323(pass,scramble_buf,hpass);
				hpass[SEED_LENGTH_323] = 0;
				myp_begin_packet(p,0);
				myp_write(p,hpass,SEED_LENGTH_323 + 1);
				goto send_cnx_packet;
			}
			// fallthrough
		default:
			myp_close(m);
			error(m,"Invalid packet error",NULL);
			return NULL;
		}
	}
	// we are connected, setup a longer timeout
	psock_set_timeout(m->s,18000);
	return m;
}

int mysql_select_db( MYSQL *m, const char *dbname ) {
	MYSQL_PACKET *p = &m->packet;
	int pcount = 0;
	myp_begin_packet(p,0);
	myp_write_byte(p,COM_INIT_DB);
	myp_write_string_eof(p,dbname);
	if( !myp_send_packet(m,p,&pcount) ) {
		error(m,"Failed to send packet",NULL);
		return -1;
	}
	return myp_ok(m,0) ? 0 : -1;
}

int mysql_real_query( MYSQL *m, const char *query, int qlength ) {
	MYSQL_PACKET *p = &m->packet;
	int pcount = 0;
	myp_begin_packet(p,0);
	myp_write_byte(p,COM_QUERY);
	myp_write(p,query,qlength);
	m->last_field_count = -1;
	m->affected_rows = -1;
	m->last_insert_id = -1;
	if( !myp_send_packet(m,p,&pcount) ) {
		error(m,"Failed to send packet",NULL);
		return -1;
	}
	if( !myp_ok(m,1) )
		return -1;
	p->id = IS_QUERY;
	return 0;
}

static int do_store( MYSQL *m, MYSQL_RES *r ) {
	int i;
	MYSQL_PACKET *p = &m->packet;
	p->pos = 0;
	r->nfields = myp_read_bin(p);
	if( p->error ) return 0;
	r->fields = (MYSQL_FIELD*)malloc(sizeof(MYSQL_FIELD) * r->nfields);
	memset(r->fields,0,sizeof(MYSQL_FIELD) * r->nfields);
	for(i=0;i<r->nfields;i++) {
		if( !myp_read_packet(m,p) )
			return 0;
		{
			MYSQL_FIELD *f = r->fields + i;
			f->catalog = m->is41 ? myp_read_bin_str(p) : NULL;
			f->db = m->is41 ? myp_read_bin_str(p) : NULL;
			f->table = myp_read_bin_str(p);
			f->org_table = m->is41 ? myp_read_bin_str(p) : NULL;
			f->name = myp_read_bin_str(p);
			f->org_name = m->is41 ? myp_read_bin_str(p) : NULL;
			if( m->is41 ) myp_read_byte(p);
			f->charset = m->is41 ? myp_read_ui16(p) : 0x08;
			f->length = m->is41 ? myp_read_int(p) : myp_read_bin(p);
			f->type = m->is41 ? myp_read_byte(p) : myp_read_bin(p);
			f->flags = m->is41 ? myp_read_ui16(p) : myp_read_bin(p);
			f->decimals = myp_read_byte(p);
			if( m->is41 ) myp_read_byte(p); // should be 0
			if( m->is41 ) myp_read_byte(p); // should be 0
			if( p->error )
				return 0;
		}
	}
	// first EOF packet
	if( !myp_read_packet(m,p) )
		return 0;
	if( myp_read_byte(p) != 0xFE || p->size >= 9 )
		return 0;
	// reset packet buffer (to prevent to store large buffer in row data)
	free(p->buf);
	p->buf = NULL;
	p->mem = 0;
	// datas
	while( 1 ) {
		if( !myp_read_packet(m,p) )
			return 0;
		// EOF : end of datas
		if( (unsigned char)p->buf[0] == 0xFE && p->size < 9 )
			break;
		// ERROR ?
		if( (unsigned char)p->buf[0] == 0xFF ) {
			save_error(m,p);
			return 0;
		}
		// allocate one more row
		if( r->row_count == r->memory_rows ) {
			MYSQL_ROW_DATA *rows;
			r->memory_rows = r->memory_rows ? (r->memory_rows << 1) : 1;
			rows = (MYSQL_ROW_DATA*)malloc(r->memory_rows * sizeof(MYSQL_ROW_DATA));
			memcpy(rows,r->rows,r->row_count * sizeof(MYSQL_ROW_DATA));
			free(r->rows);
			r->rows = rows;
		}
		// read row fields
		{
			MYSQL_ROW_DATA *current = r->rows + r->row_count++;
			int prev = 0;			
			current->raw = p->buf;
			current->lengths = (unsigned long*)malloc(sizeof(unsigned long) * r->nfields);
			current->datas = (char**)malloc(sizeof(char*) * r->nfields);
			for(i=0;i<r->nfields;i++) {
				int l = myp_read_bin(p);
				if( !p->error )
					p->buf[prev] = 0;
				if( l == -1 ) {
					current->lengths[i] = 0;
					current->datas[i] = NULL;
				} else {
					current->lengths[i] = l;
					current->datas[i] = p->buf + p->pos;
					p->pos += l;
				}
				prev = p->pos;
			}
			if( !p->error )
				p->buf[prev] = 0;
		}
		// the packet buffer as been stored, don't reuse it
		p->buf = NULL;
		p->mem = 0;
		if( p->error )
			return 0;
	}
	return 1;
}

MYSQL_RES *mysql_store_result( MYSQL *m ) {
	MYSQL_RES *r;
	MYSQL_PACKET *p = &m->packet;
	if( p->id != IS_QUERY )
		return NULL;
	// OK without result
	if( p->buf[0] == 0 ) {
		p->pos = 0;
		m->last_field_count = myp_read_byte(p); // 0
		m->affected_rows = myp_read_bin(p);
		m->last_insert_id = myp_read_bin(p);
		return NULL;
	}
	r = (MYSQL_RES*)malloc(sizeof(struct _MYSQL_RES));
	memset(r,0,sizeof(struct _MYSQL_RES));
	m->errcode = 0;
	if( !do_store(m,r) ) {
		mysql_free_result(r);
		if( !m->errcode )
			error(m,"Failure while storing result",NULL);
		return NULL;
	}
	m->last_field_count = r->nfields;
	return r;
}

int mysql_field_count( MYSQL *m ) {
	return m->last_field_count;
}

int mysql_affected_rows( MYSQL *m ) {
	return m->affected_rows;
}

int mysql_escape_string( MYSQL *m, char *sout, const char *sin, int length ) {
	return myp_escape_string(m->infos.server_charset,sout,sin,length);
}

const char *mysql_character_set_name( MYSQL *m ) {
	const char *name = myp_charset_name(m->infos.server_charset);
	if( name == NULL ) {
		static char tmp[512];
		sprintf(tmp,"#%d",m->infos.server_charset);
		return tmp;
	}
	return name;
}

int mysql_real_escape_string( MYSQL *m, char *sout, const char *sin, int length ) {
	if( !myp_supported_charset(m->infos.server_charset) )
		return -1;
	if( m->infos.server_status & SERVER_STATUS_NO_BACKSLASH_ESCAPES )
		return myp_escape_quotes(m->infos.server_charset,sout,sin,length);
	return myp_escape_string(m->infos.server_charset,sout,sin,length);
}

void mysql_close( MYSQL *m ) {
	myp_close(m);
	free(m->packet.buf);
	free(m->infos.server_version);
	free(m);
}

const char *mysql_error( MYSQL *m ) {
	return m->last_error;
}

// RESULTS API

unsigned int mysql_num_rows( MYSQL_RES *r ) {
	return r->row_count;
}

int mysql_num_fields( MYSQL_RES *r ) {
	return r->nfields;
}

MYSQL_FIELD *mysql_fetch_fields( MYSQL_RES *r ) {
	return r->fields;
}

unsigned long *mysql_fetch_lengths( MYSQL_RES *r ) {
	return r->current ? r->current->lengths : NULL;
}

MYSQL_ROW mysql_fetch_row( MYSQL_RES * r ) {
	MYSQL_ROW_DATA *cur = r->current;
	if( cur == NULL )
		cur = r->rows;
	else {
		// free the previous result, since we're done with it
		free(cur->datas);
		free(cur->lengths);
		free(cur->raw);
		cur->datas = NULL;
		cur->lengths = NULL;
		cur->raw = NULL;
		// next
		cur++;
	}
	if( cur >= r->rows + r->row_count ) {		
		free(r->rows);
		r->rows = NULL;
		r->memory_rows = 0;
		cur = NULL;	
	}
	r->current = cur;
	return cur ? cur->datas : NULL;
}

void mysql_free_result( MYSQL_RES *r ) {
	if( r->fields ) {
		int i;
		for(i=0;i<r->nfields;i++) {
			MYSQL_FIELD *f = r->fields + i;
			free(f->catalog);
			free(f->db);
			free(f->table);
			free(f->org_table);
			free(f->name);
			free(f->org_name);
		}
		free(r->fields);
	}
	if( r->rows ) {
		int i;
		for(i=0;i<r->row_count;i++) {
			MYSQL_ROW_DATA *row = r->rows + i;
			free(row->datas);
			free(row->lengths);
			free(row->raw);
		}
		free(r->rows);
	}
	free(r);
}

/* ************************************************************************ */

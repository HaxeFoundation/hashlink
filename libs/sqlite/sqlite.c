#define HL_NAME(n) sqlite_##n

#include <hl.h>
#include <string.h>
#include <sqlite3.h>

/**
	<doc>
	<h1>SQLite</h1>
	<p>
	Sqlite is a small embeddable SQL database that store all its data into
	a single file. See http://sqlite.org for more details.
	</p>
	</doc>
**/
typedef struct _database sqlite_database;
typedef struct _result sqlite_result;

struct _database {
	void (*finalize)( sqlite_database * );
	sqlite3 *db;
	sqlite_result *last;
};

struct _result {
	void (*finalize)(sqlite_result * );
	sqlite_database *db;
	int ncols;
	int count;
	int *names; //hashed_names
	int *bools;
	int done;
	int first;
	sqlite3_stmt *r;
};

static void HL_NAME(error)( sqlite3 *db, bool close ) {
	hl_buffer *b = hl_alloc_buffer();
	hl_buffer_str(b, USTR("SQLite error: "));
	hl_buffer_str(b, sqlite3_errmsg16(db));
	if ( close )
		sqlite3_close(db);
	hl_error("%s",hl_buffer_content(b,NULL));
}

static void HL_NAME(finalize_request)(sqlite_result *r, bool exc ) {
	r->first = 0;
	r->done = 1;
	if( r->ncols == 0 )
		r->count = sqlite3_changes(r->db->db);
	if( sqlite3_finalize(r->r) != SQLITE_OK && exc )
		hl_error("SQLite error: Could not finalize request");
	r->r = NULL;
	r->db->last = NULL;
	r->db = NULL;
	free(r->names);
	free(r->bools);
	r->names = NULL;
	r->bools = NULL;
}
static void HL_NAME(finalize_result)(sqlite_result *r ) {
	if (r && r->db) HL_NAME(finalize_request)(r, false);
}

/**
close : 'db -> void
<doc>Closes the database.</doc>
**/
HL_PRIM void HL_NAME(close)( sqlite_database *db ) {
	if (db->last != NULL)
		HL_NAME(finalize_request)(db->last, false);
	if (sqlite3_close(db->db) != SQLITE_OK) {
		// No exception : we shouldn't alloc memory in a finalizer anyway
	}
	db->db = NULL;
}
static void HL_NAME(finalize_database)( sqlite_database *db ) {
	if (db && db->db) HL_NAME(close)(db);
}


/**
	connect : filename:string -> 'db
	<doc>Open or create the database stored in the specified file.</doc>
**/
HL_PRIM sqlite_database *HL_NAME(connect)( vbyte *filename ) {
	sqlite_database *db;
	sqlite3 *sqlite;
	if( sqlite3_open16(filename, &sqlite) != SQLITE_OK ) {
		HL_NAME(error)(sqlite, true);
	}
	db = (sqlite_database*)hl_gc_alloc_finalizer(sizeof(sqlite_database));
	db->finalize = HL_NAME(finalize_database);
	db->db = sqlite;
	db->last = NULL;
	return db;
}

/**
	last_insert_id : 'db -> int
	<doc>Returns the last inserted auto_increment id.</doc>
**/
HL_PRIM int HL_NAME(last_id)(sqlite_database *db ) {
	return (int)sqlite3_last_insert_rowid(db->db);
}

/**
	request : 'db -> sql:string -> 'result
	<doc>Executes the SQL request and returns its result</doc>
**/
HL_PRIM sqlite_result *HL_NAME(request)(sqlite_database *db, vbyte *sql ) {
	sqlite_result *r;
	const char *tl;
	int i,j;

	r = (sqlite_result*)hl_gc_alloc_finalizer(sizeof(sqlite_result));
	r->finalize = HL_NAME(finalize_result);
	r->db = NULL;
	
	if( sqlite3_prepare16_v2(db->db, sql, -1, &r->r, &tl) != SQLITE_OK ) {
		HL_NAME(error)(db->db, false);
	}

	if( *tl ) {
		sqlite3_finalize(r->r);
		hl_error("SQLite error: Cannot execute several SQL requests at the same time");
	}

	r->db = db;
	r->ncols = sqlite3_column_count(r->r);
	r->names = (int*)malloc(sizeof(int)*r->ncols);
	r->bools = (int*)malloc(sizeof(int)*r->ncols);
	r->first = 1;
	r->done = 0;
	for(i=0;i<r->ncols;i++) {
		int id = hl_hash_gen((uchar*)sqlite3_column_name16(r->r,i), true);
		const char *dtype = sqlite3_column_decltype(r->r,i);
		for(j=0;j<i;j++)
			if( r->names[j] == id ) {
				if( strcmp(sqlite3_column_name16(r->r,i), sqlite3_column_name16(r->r,j)) == 0 ) {
					sqlite3_finalize(r->r);
					hl_buffer *b = hl_alloc_buffer();
					hl_buffer_str(b, USTR("SQLite error: Same field is two times in the request: "));
					hl_buffer_str(b, (uchar*)sql);

					hl_error("%s",hl_buffer_content(b, NULL));
				} else {
					hl_buffer *b = hl_alloc_buffer();
					hl_buffer_str(b, USTR("SQLite error: Same field ids for: "));
					hl_buffer_str(b, sqlite3_column_name16(r->r,i));
					hl_buffer_str(b, USTR(" and "));
					hl_buffer_str(b, sqlite3_column_name16(r->r,j));
					
					sqlite3_finalize(r->r);
					hl_error("%s",hl_buffer_content(b, NULL));
				}
			}
		r->names[i] = id;
		r->bools[i] = dtype?(strcmp(dtype,"BOOL") == 0):0;
	}
	// changes in an update/delete
	if( db->last != NULL )
		HL_NAME(finalize_request)(db->last, false);
	
	db->last = r;
	return db->last;
}

/**
	result_get_length : 'result -> int
	<doc>Returns the number of rows in the result or the number of rows changed by the request.</doc>
**/
HL_PRIM vdynamic *HL_NAME(result_get_length)( sqlite_result *r ) {
	if( r->ncols != 0 )
		return NULL;
	return hl_make_dyn(&r->count, &hlt_i32);
}

/**
	result_get_nfields : 'result -> int
	<doc>Returns the number of fields in the result.</doc>
**/
HL_PRIM int HL_NAME(result_get_nfields)( sqlite_result *r ) {
	return r->ncols;
}

/**
	result_get_fields : 'result -> array<string>
	<doc>Returns the array of field names in the result.</doc>
**/
HL_PRIM varray *HL_NAME(result_get_fields)( sqlite_result *r ) {
	varray *a = hl_alloc_array(&hlt_bytes, r->ncols);
	int i;
	for (i = 0; i < r->ncols; i++)
	{
		hl_aptr(a, vbyte*)[i] = (vbyte *)sqlite3_column_name16(r->r, i);
	}

	return a;
}


/**
	result_next : 'result -> object?
	<doc>Returns the next row in the result or [null] if no more result.</doc>
**/
HL_PRIM varray *HL_NAME(result_next)( sqlite_result *r ) {
	if( r->done )
		return NULL;
	switch( sqlite3_step(r->r) ) {
	case SQLITE_ROW:
		r->first = 0;
		varray *a = hl_alloc_array(&hlt_dyn, r->ncols);
		int i;
		for(i=0;i<r->ncols;i++)
		{
			vdynamic *v;
			switch( sqlite3_column_type(r->r,i) ) {
			case SQLITE_NULL:
				v = NULL;
				break;
			case SQLITE_INTEGER:
			{
				int vint = sqlite3_column_int(r->r, i);
				if (r->bools[i])
					v = hl_make_dyn(&vint, &hlt_bool);
				else
					v = hl_make_dyn(&vint, &hlt_i32);
				break;
			}
			case SQLITE_FLOAT:
			{
				double d = sqlite3_column_double(r->r, i);
				v = hl_make_dyn(&d, &hlt_f64);
				break;
			}
			case SQLITE_TEXT:
			{
				uchar *text16 = (uchar *)sqlite3_column_text16(r->r, i);
				vbyte *vb = hl_copy_bytes((vbyte *)text16, (int)(ustrlen(text16) + 1) * sizeof(uchar));
				v = hl_make_dyn(&vb, &hlt_bytes);
				break;
			}
			case SQLITE_BLOB:
			{
				int size = sqlite3_column_bytes(r->r, i);
				vbyte *blob = (vbyte *)sqlite3_column_blob(r->r, i);
				vbyte *vb = hl_copy_bytes(blob, size+1);
				
				varray *bytes_data = hl_alloc_array(&hlt_dyn, 2);
				hl_aptr(bytes_data, vdynamic*)[0] = hl_make_dyn(&vb, &hlt_bytes);
				hl_aptr(bytes_data, vdynamic*)[1] = hl_make_dyn(&size, &hlt_i32);
				
				v = hl_make_dyn(&bytes_data, &hlt_array);
				break;
			}
			default:
				hl_error("SQLite error: Unknown type #%d", sqlite3_column_type(r->r,i));
			}
			hl_aptr(a, vdynamic*)[i] = v;
		}
		return a;
	case SQLITE_DONE:
		HL_NAME(finalize_request)(r, true);
		return NULL;
	case SQLITE_BUSY:
		hl_error("SQLite error: Database is busy");
	case SQLITE_ERROR:
		HL_NAME(error)(r->db->db, false);
	default:
		return NULL;
	}
	return NULL;
}

/**
	result_get : 'result -> n:int -> string
	<doc>Return the [n]th field of the current result row.</doc>
**/
HL_PRIM vbyte *HL_NAME(result_get)( sqlite_result *r, int n ) {
	if (n < 0 || n >= r->ncols)
		return NULL;
	if( r->first )
		HL_NAME(result_next)(r);
	if( r->done )
		return NULL;
	return (vbyte*)sqlite3_column_text16(r->r, n);
}

/**
	result_get_int : 'result -> n:int -> int
	<doc>Return the [n]th field of the current result row as an integer.</doc>
**/
HL_PRIM vdynamic *HL_NAME(result_get_int)( sqlite_result *r, int n) {
	if (n < 0 || n >= r->ncols)
		return NULL;
	if( r->first )
		HL_NAME(result_next)(r);
	if( r->done )
		return NULL;
	int value = sqlite3_column_int(r->r, n);
	return hl_make_dyn(&value, &hlt_i32);
}

/**
	result_get_float : 'result -> n:int -> float
	<doc>Return the [n]th field of the current result row as a float.</doc>
**/
HL_PRIM vdynamic *HL_NAME(result_get_float)( sqlite_result *r, int n ) {
	if( n < 0 || n >= r->ncols )
		return NULL;
	if( r->first )
		HL_NAME(result_next)(r);
	if( r->done )
		return NULL;
	double value = sqlite3_column_double(r->r, n);
	return hl_make_dyn(&value, &hlt_f64);
}

#define _CONNECTION _ABSTRACT( sqlite_database )
#define _RESULT _ABSTRACT( sqlite_result )

DEFINE_PRIM(_CONNECTION, connect, _BYTES);
DEFINE_PRIM(_VOID,       close,   _CONNECTION);
DEFINE_PRIM(_RESULT,     request, _CONNECTION _BYTES);
DEFINE_PRIM(_I32,        last_id, _CONNECTION);

DEFINE_PRIM(_ARR,          result_next,      _RESULT);
DEFINE_PRIM(_NULL(_BYTES), result_get,       _RESULT _I32);
DEFINE_PRIM(_NULL(_I32),   result_get_int,   _RESULT _I32);
DEFINE_PRIM(_NULL(_F64),   result_get_float, _RESULT _I32);
DEFINE_PRIM(_NULL(_I32),   result_get_length, _RESULT);
DEFINE_PRIM(_I32,          result_get_nfields, _RESULT);
DEFINE_PRIM(_ARR,          result_get_fields, _RESULT);

/*
 * Copyright (C)2015-2026 Haxe Foundation
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
#ifdef S_TYPE

// is included by data_struct.h

#ifdef S_MAP
#	define S_ARGS	S_KEY k, S_VALUE v
#else
#	define S_ARGS	S_VALUE k
#	define S_KEY	S_VALUE
#	define keys		values
#endif

#ifndef S_DEFVAL
#	define S_DEFVAL	(S_VALUE)0
#endif

#ifndef S_CMP
#	define S_CMP(a,b) a > b
#endif

typedef struct {
	int cur;
	int max;
	S_KEY *keys;
#	ifdef S_MAP
	S_VALUE *values;
#	endif
} S_TYPE;

typedef S_VALUE	S_NAME(_value);

INLINE static void S_NAME(check_size)( hl_alloc *alloc, S_TYPE *st ) {
	if( st->cur == st->max ) {
		int n = st->max ? (st->max << 1) : STRUCT_DEF_SIZE;
		S_KEY *keys = (S_KEY*)hl_malloc(alloc,sizeof(S_KEY) * n);
		memcpy(keys,st->keys,sizeof(S_KEY) * st->cur);
		st->keys = keys;
#		ifdef S_MAP
		S_VALUE *vals = (S_VALUE*)hl_malloc(alloc,sizeof(S_VALUE) * n);
		memcpy(vals,st->values,sizeof(S_VALUE) * st->cur);
		st->values = vals;
#		endif
		st->max = n;
	}
}

#ifndef S_SORTED

INLINE static void S_NAME(add_impl)( hl_alloc *alloc, S_TYPE *st, S_ARGS ) {
	S_NAME(check_size)(alloc,st);
	st->keys[st->cur] = k;
#	ifdef S_MAP
	st->values[st->cur] = v;
#	endif
	st->cur++;
}

INLINE static bool S_NAME(exists)( S_TYPE st, S_KEY k ) {
	for(int i=0;i<st.cur;i++)
		if( st.keys[i] == k )
			return true;
	return false;
}

INLINE static bool S_NAME(remove)( S_TYPE *st, S_KEY k ) {
	for(int i=0;i<st->cur;i++)
		if( st->keys[i] == k ) {
			int pos = i;
			memmove(st->keys + pos, st->keys + pos + 1, (st->cur - pos - 1) * sizeof(S_KEY));
#			ifdef S_MAP
			memmove(st->values + pos, st->values + pos + 1, (st->cur - pos - 1) * sizeof(S_VALUE));
#			endif
			st->cur--;
			return true;
		}
	return false;
}

#ifdef S_MAP
static S_VALUE S_NAME(find)( S_TYPE st, S_KEY k ) {
	for(int i=0;i<st.cur;i++)
		if( st.keys[i] == k )
			return st.values[i];
	return (S_VALUE)0;
}
#else
static S_VALUE *S_NAME(reserve_impl)( hl_alloc *alloc, S_TYPE *st, int count ) {
	if( st->cur + count > st->max ) {
		int n = st->max ? (st->max << 1) : STRUCT_DEF_SIZE;
		while( n < st->cur + count ) n <<= 1;
		S_KEY *keys = (S_KEY*)hl_malloc(alloc,sizeof(S_KEY) * n);
		memcpy(keys,st->keys,sizeof(S_KEY) * st->cur);
		st->keys = keys;
	}
	S_VALUE *ptr = st->keys + st->cur;
	st->cur += count;
	return ptr;
}
#endif


#else

INLINE static bool S_NAME(add_impl)( hl_alloc *alloc, S_TYPE *st, S_ARGS ) {
	int min = 0;
	int max = st->cur;
	int pos;
	while( min < max ) {
		int mid = (min + max) >> 1;
		S_KEY k2 = st->keys[mid];
		if( S_CMP(k,k2) ) min = mid + 1; else if( S_CMP(k2,k) ) max = mid; else return false;
	}
	S_NAME(check_size)(alloc,st);
	pos = (min + max) >> 1;
	memmove(st->keys + pos + 1, st->keys + pos, (st->cur - pos) * sizeof(S_KEY));
#	ifdef S_MAP
	memmove(st->values + pos + 1, st->values + pos, (st->cur - pos) * sizeof(S_VALUE));
#	endif
	st->keys[pos] = k;
#	ifdef S_MAP
	st->values[pos] = v;
#	endif
	st->cur++;
	return true;
}

#ifdef S_MAP
INLINE static void S_NAME(replace_impl)( hl_alloc *alloc, S_TYPE *st, S_ARGS ) {
	int min = 0;
	int max = st->cur;
	int pos;
	while( min < max ) {
		int mid = (min + max) >> 1;
		S_KEY k2 = st->keys[mid];
		if( k2 < k ) min = mid + 1; else if( k2 > k ) max = mid; else {
			st->values[mid] = v;
			return;
		}
	}
	S_NAME(check_size)(alloc,st);
	pos = (min + max) >> 1;
	memmove(st->keys + pos + 1, st->keys + pos, (st->cur - pos) * sizeof(S_KEY));
	memmove(st->values + pos + 1, st->values + pos, (st->cur - pos) * sizeof(S_VALUE));
	st->keys[pos] = k;
	st->values[pos] = v;
	st->cur++;
}
#endif

INLINE static bool S_NAME(exists)( S_TYPE st, S_KEY k ) {
	int min = 0;
	int max = st.cur;
	while( min < max ) {
		int mid = (min + max) >> 1;
		S_KEY k2 = st.keys[mid];
		if( S_CMP(k,k2) ) min = mid + 1; else if( S_CMP(k2,k) ) max = mid; else return true;
	}
	return false;
}

#ifdef S_MAP
INLINE static S_VALUE S_NAME(find)( S_TYPE st, S_KEY k ) {
	int min = 0;
	int max = st.cur;
	while( min < max ) {
		int mid = (min + max) >> 1;
		S_KEY k2 = st.keys[mid];
		if( k2 < k ) min = mid + 1; else if( k2 > k ) max = mid; else return st.values[mid];
	}
	return S_DEFVAL;
}
#endif

INLINE static bool S_NAME(remove)( S_TYPE *st, S_KEY k ) {
	int min = 0;
	int max = st->cur;
	while( min < max ) {
		int mid = (min + max) >> 1;
		S_KEY k2 = st->keys[mid];
		if( S_CMP(k,k2) ) min = mid + 1; else if( S_CMP(k2,k) ) max = mid; else {
			int pos = mid;
			memmove(st->keys + pos, st->keys + pos + 1, (st->cur - pos - 1) * sizeof(S_KEY));
#			ifdef S_MAP
			memmove(st->values + pos, st->values + pos + 1, (st->cur - pos - 1) * sizeof(S_VALUE));
#			endif
			st->cur--;
			return true;
		}
	}
	return false;
}

#endif

INLINE static void S_NAME(reset)( S_TYPE *st ) {
	st->cur = 0;
}

INLINE static S_VALUE *S_NAME(free)( S_TYPE *st ) {
	st->cur = 0;
	st->max = 0;
	S_VALUE *vals = st->values;
#	ifdef S_MAP
	st->keys = NULL;
#	endif
	st->values = NULL;
	return vals;
}

INLINE static int S_NAME(count)( S_TYPE st ) {
	return st.cur;
}

INLINE static S_VALUE S_NAME(get)( S_TYPE st, int idx ) {
	return st.values[idx];
}

INLINE static S_VALUE S_NAME(first)( S_TYPE st ) {
	return st.cur == 0 ? S_DEFVAL : st.values[0];
}

INLINE static bool S_NAME(iter_next)( S_TYPE st, S_VALUE *val, int idx ) {
	if( idx < st.cur ) *val = st.values[idx];
	return idx < st.cur;
}

INLINE static bool S_NAME(iter_prev)( S_TYPE st, S_VALUE *val, int idx ) {
	if( idx >= 0 ) *val = st.values[idx];
	return idx >= 0;
}

#undef S_NAME
#undef S_TYPE
#undef S_VALUE
#undef S_KEY
#undef S_ARGS
#undef STRUCT_NAME
#undef S_CMP
#undef S_DEFVAL
#undef keys

#endif

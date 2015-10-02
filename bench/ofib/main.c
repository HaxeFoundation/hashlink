#include <stdio.h>
#include <stdlib.h>

typedef struct vobj vobj;

typedef struct {
	int *UNUSED;
	int (*fib)( vobj *o, int n );
} proto;

typedef struct {
	int (*fib)( vobj *o, int n );
	vobj *o;
} vclosure;

struct vobj {
	proto *p;
	int one;
	int two;
	vclosure *c;
};

int ofib( vobj *o, int n ) {
	if( n <= 2 )
		return 1;
	return o->p->fib(o, n - o->one) + o->c->fib(o->c->o,n - o->two);
}

int main() {
	proto *p = (proto*)malloc(sizeof(proto));
	vclosure *c = (vclosure*)malloc(sizeof(vclosure));
	vobj *o = (vobj*)malloc(sizeof(vobj));
	o->p = p;
	o->one = 1;
	o->two = 2;
	o->c = c;
	c->fib = ofib;
	c->o = o;
	p->fib = ofib;
	printf("%di\n",o->p->fib(o,40));
}
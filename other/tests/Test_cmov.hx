// Targets the float CMOV path (PHI_COND -> float conditional select) that the
// jit_x86_64.c change rewrites from a flag-preserving branchless blend into a
// branch-around-a-move. Every case below produces a float phi on a conditional
// edge, in register and (under pressure) spilled forms, plus a following use of
// the SAME compare so the shared-flags invariant is exercised.
class Test_cmov {

	// simple float ternary: out := cond ? a : b  (both in regs)
	static function selReg( c : Bool, a : Float, b : Float ) : Float {
		return c ? a : b;
	}

	// float32 variant (M_F32 path in the encoder)
	static function selF32( c : Bool, a : Single, b : Single ) : Single {
		return c ? a : b;
	}

	// the compare result is consumed TWICE: once by the float select, once by a
	// following branch. This is the exact shape that forced pushfq/popfq before.
	static function sharedCompare( x : Float, a : Float, b : Float ) : Float {
		var pick = (x > 0.5) ? a : b;   // float select on (x>0.5)
		if( x > 0.5 )                    // same comparison drives a real branch
			pick += 1000.0;
		return pick;
	}

	// heavy register pressure so the select operand is a SPILLED (memory) float,
	// exercising the !IS_REG(e->a) branch (the old "ANDNPD requires aligned addr").
	static function selSpilled( c : Bool, seed : Float ) : Float {
		var f0=seed+0.0, f1=seed+1.0, f2=seed+2.0, f3=seed+3.0, f4=seed+4.0;
		var f5=seed+5.0, f6=seed+6.0, f7=seed+7.0, f8=seed+8.0, f9=seed+9.0;
		var f10=seed+10.,f11=seed+11.,f12=seed+12.,f13=seed+13.,f14=seed+14.;
		var f15=seed+15.,f16=seed+16.,f17=seed+17.,f18=seed+18.,f19=seed+19.;
		// a conditional pick of two cold spilled floats
		var pick = c ? f17 : f3;
		return f0+f1+f2+f3+f4+f5+f6+f7+f8+f9+f10+f11+f12+f13+f14
			+f15+f16+f17+f18+f19 + pick*1000.0;
	}

	// selects inside a loop, alternating condition, to hit both taken/not-taken
	static function loopSel( n : Int ) : Float {
		var acc = 0.0;
		for( i in 0...n ) {
			var c = (i & 1) == 0;
			var v = c ? (i * 1.5) : (i * -2.25);
			acc += v;
		}
		return acc;
	}

	static function fmt( f : Float ) return Math.round(f*10000)/10000;

	static function main() {
		trace("=== start ===");
		for( c in [true,false] )
			for( a in [1.5, -3.25] )
				for( b in [10.0, -0.5] )
					trace("selReg", c, a, b, fmt(selReg(c,a,b)));
		for( c in [true,false] )
			trace("selF32", c, fmt(selF32(c, 2.5, -7.5)));
		for( x in [0.0, 0.5, 0.6, 1.0, -1.0] )
			trace("sharedCompare", x, fmt(sharedCompare(x, 3.0, 4.0)));
		for( c in [true,false] )
			trace("selSpilled", c, fmt(selSpilled(c, 0.5)));
		for( n in [0,1,2,5,10] )
			trace("loopSel", n, fmt(loopSel(n)));
		trace("=== done ===");
	}
}

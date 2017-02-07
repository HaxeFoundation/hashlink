import format.hl.Data;
using format.hl.Tools;
import Block;

class Stats {
	var mem : Memory;
	var byT = new Map();
	var allT = [];

	public function new(mem) {
		this.mem = mem;
	}

	public function add( t : TType, mem : Int ) {
		return addPath([t == null ? 0 : t.tid], mem);
	}

	public function addPath( tl : Array<Int>, mem : Int ) {
		var key = tl.join(" ");
		var inf = byT.get(key);
		if( inf == null ) {
			inf = { tl : tl, count : 0, mem : 0 };
			byT.set(key, inf);
			allT.push(inf);
		}
		inf.count++;
		inf.mem += mem;
	}

	public function print() {
		allT.sort(function(i1, i2) return i1.mem - i2.mem);
		for( i in allT )
			mem.log(i.count + " count, " + Memory.MB(i.mem) + " " + [for( tid in i.tl ) mem.types[tid].toString()].join(" > "));
	}

}

class Memory {

	static inline var PAGE_BITS = 16;

	public var memoryDump : sys.io.FileInput;

	public var is64 : Bool;
	public var bool32 : Bool;
	public var ptrBits : Int;

	public var types : Array<TType>;

	var code : format.hl.Data;
	var pages : Array<Page>;
	var roots : Array<Pointer>;
	var stacks : Array<Stack>;
	var typesPointers : Array<Pointer>;
	var closuresPointers : Array<Pointer>;
	var blocks : Array<Block>;
	var baseTypes : Array<{ t : HLType, p : Pointer }>;
	var all : Block;

	var toProcess : Array<Block>;
	var dynObjCandidates : Array<Block>;
	var tdynObj : TType;
	var pointerBlock : Map<Pointer, Block>;
	var pointerType : Map<Pointer, TType>;

	function new() {
	}

	public function typeSize( t : HLType ) {
		return switch( t ) {
		case HVoid: 0;
		case HUi8: 1;
		case HUi16: 2;
		case HI32, HF32: 4;
		case HF64: 8;
		case HBool:
			return bool32 ? 4 : 1;
		default:
			return is64 ? 8 : 4;
		}
	}

	public function getType( t : HLType, isNull = false ) : TType {
		// this is quite slow, but we can't use a Map, maybe try a more per-type specific approach ?
		for( t2 in types )
			if( t == t2.t )
				return t2;
		if( isNull )
			return null;
		throw "Type not found " + t.toString();
		return null;
	}

	function loadBytecode( arg : String ) {
		if( code != null ) throw "Duplicate code";
		code = new format.hl.Reader(false).read(new haxe.io.BytesInput(sys.io.File.getBytes(arg)));
		log(arg + " code loaded");
	}

	inline function readInt() {
		return memoryDump.readInt32();
	}

	inline function readPointer() : Pointer {
		return cast memoryDump.readInt32();
	}

	public static function MB( v : Float ) {
		if( v < 1000 )
			return Std.int(v) + "B";
		if( v < 1024 * 1000 )
			return (Math.round(v * 10 / 1024) / 10)+"KB";
		return (Math.round(v * 10 / (1024 * 1024)) / 10)+"MB";
	}

	function loadMemory( arg : String ) {
		memoryDump = sys.io.File.read(arg);
		if( memoryDump.read(3).toString() != "HMD" )
			throw "Invalid memory dump file";

		var version = memoryDump.readByte() - "0".code;
		var flags = readInt();
		is64 = (flags & 1) != 0;
		bool32 = (flags & 2) != 0;

		ptrBits = is64 ? 3 : 2;
		if( is64 ) throw "64 bit not supported";

		// load pages
		var count = readInt();
		pages = [];
		for( i in 0...count ) {
			while( true ) {
				var addr = readPointer();
				if( addr.isNull() ) break;
				var p = new Page(this);
				p.addr = addr;
				p.kind = cast readInt();
				p.size = readInt();
				p.blockSize = readInt();
				p.firstBlock = readInt();
				p.maxBlocks = readInt();
				p.nextBlock = readInt();

				p.dataPosition = memoryDump.tell();
				memoryDump.seek(p.size, SeekCur);

				var flags = readInt();
				if( flags & 1 != 0 )
					p.bmp = memoryDump.read((p.maxBlocks + 7) >> 3); // 1 bit per block
				if( flags & 2 != 0 )
					p.sizes = memoryDump.read(p.maxBlocks); // 1 byte per block
				pages.push(p);
			}
		}

		// load roots
		roots = [for( i in 0...readInt() ) readPointer()];

		// load stacks
		stacks = [];
		for( i in 0...readInt() ) {
			var s = new Stack();
			s.base = readPointer();
			s.contents = [for( i in 0...readInt() ) readPointer()];
			stacks.push(s);
		}

		// load types

		baseTypes = [];
		while( true ) {
			var tid = readInt();
			if( tid < 0 ) break;
			var ptr = readPointer();
			baseTypes.push({ t : Type.createEnumIndex(HLType, tid), p : ptr });
		}

		typesPointers = [for( i in 0...readInt() ) readPointer()];
		closuresPointers = [for( i in 0...readInt() ) readPointer()];
	}

	function printStats() {
		var totalSize = 0, maxSize = 0;
		var used = 0, gc = 0;
		for( p in pages ) {
			maxSize += p.size;
			totalSize += p.size + p.bmp.length;
			gc += p.size - (p.maxBlocks - p.firstBlock) * p.blockSize;
			gc += p.bmp.length;
		}
		for( b in blocks )
			used += b.getMemSize();
		log(pages.length + " pages, " + MB(totalSize) + " memory");
		log(roots.length + " roots, "+ stacks.length + " stacks");
		log(code.types.length + " types, " + closuresPointers.length + " closures");
		log(blocks.length + " live blocks " + MB(used) + " used, " + MB(maxSize - used) + " free, "+MB(gc)+" gc");
	}

	function getTypeNull( t : TType ) {
		if( t.nullWrap != null )
			return t.nullWrap;
		for( t2 in types )
			switch( t2.t ) {
			case HNull(base) if( base == t.t ):
				t.nullWrap = t2;
				return t2;
			default:
			}
		var r = new TType(types.length, HNull(t.t));
		t.nullWrap = r;
		types.push(r);
		return r;
	}

	function goto( b : Block ) {
		b.page.goto(b.bid);
	}

	function check() {
		if( code == null ) throw "Missing .hl file";
		if( memoryDump == null ) throw "Missing .dump file";
		if( code.types.length != this.typesPointers.length ) throw "Types count mismatch";

		pointerType = new Map();
		var cid = 0;
		types = [for( i in 0...code.types.length ) new TType(i, code.types[i])];
		for( i in 0...typesPointers.length ) {
			pointerType.set(typesPointers[i], types[i]);
			switch( code.types[i] ) {
			case HFun(f):
				var tid = types.length;
				var args = f.args.copy();
				var ct = new TType(tid, HFun({ args : args, ret : f.ret }), args.shift());
				types.push(ct);
				pointerType.set(closuresPointers[cid++], ct);
			default:
			}
		}

		for( b in baseTypes ) {
			var t = getType(b.t, true);
			if( t == null ) {
				t = new TType(types.length, b.t);
				types.push(t);
			}
			pointerType.set(b.p, t);
		}

		blocks = [];
		var pageMem = new Map();
		var progress = 0;
		pointerBlock = new Map();

		for( p in pages ) {
			progress++;
			if( progress % 100 == 0 )
				Sys.print((Std.int(progress * 1000 / pages.length) / 10) + "%  \r");

			var bid = p.firstBlock;
			while( bid < p.maxBlocks ) {
				if( !p.isLiveBlock(bid) ) {
					bid++;
					continue;
				}
				var sz = p.getBlockSize(bid);

				var b = new Block();
				b.page = p;
				b.bid = bid;

				goto(b);
				b.type = pointerType.get(readPointer());
				if( b.type != null && !b.type.isDyn )
					b.type = getTypeNull(b.type);
				if( b.type != null )
					b.typeKind = KHeader;

				blocks.push(b);
				pointerBlock.set(b.getPointer(), b);

				bid += sz;
			}

			for( i in 0...(p.size >> PAGE_BITS) )
				pageMem.set(p.addr.offset(i << PAGE_BITS), p);

		}

		printStats();

		// look in roots (higher ownership priority)
		all = new Block();

		var broot = new Block();
		broot.type = new TType(types.length, HAbstract("roots"));
		types.push(broot.type);
		broot.addParent(all);
		var rid = 0;
		for( g in code.globals ) {
			if( !g.isPtr() ) continue;
			var r = roots[rid++];
			var b = pointerBlock.get(r);
			if( b == null ) continue;
			b.addParent(broot);
			if( b.type == null ) {
				b.type = getType(g);
				b.typeKind = KRoot;
			}
		}

		var tvoid = getType(HVoid);
		for( t in types )
			t.buildTypes(this, tvoid);

		tdynObj = getType(HDynObj);
		toProcess = blocks.copy();
		dynObjCandidates = [];
		while( toProcess.length > 0 ) {
			buildHierarchy();
			buildDynObjs();
		}

		// look in stacks (low priority of ownership)
		var tstacks = new TType(types.length, HAbstract("stack"));
		types.push(tstacks);
		for( s in stacks ) {
			var bstack = new Block();
			bstack.type = tstacks;
			bstack.addParent(all);
			for( r in s.contents ) {
				var b = pointerBlock.get(r);
				if( b != null )
					b.addParent(bstack);
			}
		}

		for( b in blocks )
			b.finalize();

		var unk = 0, unkMem = 0;
		for( b in blocks ) {
			if( b.owner == null ) {
				log("  "+b.getPointer().toString() + " is not referenced");
				continue;
			}

			if( b.type != null ) continue;

			var o = b.owner;
			while( o != null && o.type == null )
				o = o.owner;
			if( o != null )
				switch( o.type.t ) {
				case HAbstract(_):
					b.type = o.type; // data inside this
					b.typeKind = KAbstractData;
					continue;
				default:
				}

			unk++;
			unkMem += b.getMemSize();
		}

		var falseCount = 0;
		for( t in types )
			falseCount += t.falsePositive;

		log("Hierarchy built, "+unk+" unknown ("+MB(unkMem)+"), "+falseCount+" false positives");
	}

	function printFalsePositives( ?typeStr : String ) {
		var falses = [for( t in types ) if( t.falsePositive > 0 && (typeStr == null || t.toString().indexOf(typeStr) >= 0) ) t];
		falses.sort(function(t1, t2) return t1.falsePositive - t2.falsePositive);
		for( f in falses )
			log(f.falsePositive+" count " + f + " "+f.falsePositiveIndexes+"\n    "+f.fields);
	}

	function printUnknown() {
		var byT = new Map();
		for( b in blocks ) {
			if( b.type != null ) continue;

			var o = b;
			while( o != null && o.type == null )
				o = o.owner;

			var t = o == null ? null : o.type;
			var tid = t == null ? -1 : t.tid;
			var inf = byT.get(tid);
			if( inf == null ) {
				inf = { t : t, count : 0, mem : 0 };
				byT.set(tid, inf);
			}
			inf.count++;
			inf.mem += b.getMemSize();
		}
		var all = [for( k in byT ) k];
		all.sort(function(i1, i2) return i1.count - i2.count);
		for( a in all )
			log("Unknown "+a.count + " count, " + MB(a.mem)+" "+(a.t == null ? "" : a.t.toString()));
	}

	function buildHierarchy() {
		var progress = 0;
		var blocks = toProcess;
		toProcess = [];

		for( b in blocks )
			b.removeChildren();

		for( b in blocks ) {

			progress++;
			if( progress % 10000 == 0 )
				Sys.print((Std.int(progress * 1000.0 / blocks.length) / 10) + "%  \r");

			if( b.page.kind == PNoPtr )
				continue;

			if( b.type != null && !b.type.hasPtr )
				switch(b.type.t) {
				case HFun(_):
				default:
					log("  Scanning "+b.type+" "+b.getPointer().toString());
				}

			goto(b);
			var fields = null;
			var start = 0;
			if( b.type != null ) {
				fields = b.type.fields;
				// enum
				if( b.type.constructs != null ) {
					start++;
					fields = b.type.constructs[readInt()];
					if( is64 ) readInt(); // skip, not a pointer anyway
				}
			}

			for( i in start...(b.getMemSize() >> ptrBits) ) {
				var r = readPointer();
				var bs = pointerBlock.get(r);
				if( bs == null ) continue;
				var ft = fields != null ? fields[i] : null;
				bs.addParent(b);
				if( ft != null && !ft.t.isPtr() ) {
					b.type.falsePositive++;
					b.type.falsePositiveIndexes[i]++;
					continue;
				}
				if( bs.type == null && ft != null ) {
					if( ft.t == HDyn ) {
						if( bs.typeKind == null ) {
							dynObjCandidates.push(bs);
							bs.typeKind = KDynObj(b.type, b.typeKind);
						}
						continue;
					}
					if( ft.t.isDynamic() ) {
						trace(b.typeKind, b.getPointer().toString(), b.type.toString(), ft.toString());
						continue;
					}
					bs.type = ft;
					bs.typeKind = KInferred(b.type, b.typeKind);
					if( bs.subs != null )
						toProcess.push(bs);
				}
			}
		}
	}

	function buildDynObjs() {
		var blocks = dynObjCandidates;
		dynObjCandidates = [];
		for( b in blocks ) {

			if( b.type != null ) {
				//log("  Skip dynobj " + b.typeKind);
				continue;
			}

			// most likely a DynObj (which have a dynamicaly allocated hl_type*)
			// let's check
			goto(b);
			var btptr = readPointer();
			var bdptr = readPointer();
			var nfields = readInt();
			var dataSize = readInt();

			var bt = pointerBlock.get(btptr);
			if( bt != null ) {
				goto(bt);
				var tid = readInt();
				if( tid != 15 ) bt = null;
			}

			if( bt == null ) {
				log("  Not a dynobj " + b.getPointer().toString()+" "+b.owner);
				continue;
			}

			b.type = bt.type = tdynObj;
			bt.typeKind = KDynObjData(b.typeKind);

			var bd = pointerBlock.get(bdptr);
			if( bd == null )
				continue;

			if( bd.type != null )
				log("  Overwriting dynobj data " + bd.getPointer().toString()+" "+bd.type);

			bd.type = b.type; // dynobj data

			// read remaining dynobj
			if( is64 ) readInt();
			readPointer();
			readPointer();

			var fields = [for( i in 0...nfields ) {
				t : pointerType.get(readPointer()),
				name : readInt(),
				index : readInt(),
			}];

			goto(bd);

			// assign types to dynobj fields
			var pos = 0;
			for( i in 0...(bd.getMemSize() >> ptrBits) ) {
				var bv = pointerBlock.get(readPointer());
				if( bv != null )
					for( f in fields )
						if( f.index == pos && bv.type == null ) {
							bv.type = f.t;
							if( bv.subs != null ) toProcess.push(bv);
							break;
						}
				pos += 1 << ptrBits;
			}
		}
	}


	function printByType() {
		var ctx = new Stats(this);
		for( b in blocks )
			ctx.add(b.type, b.getMemSize());
		ctx.print();
	}

	function locate( tstr : String, up = 0 ) {
		var lt = null;
		for( t in types )
			if( t.t.toString() == tstr ) {
				lt = t;
				break;
			}
		if( lt == null ) {
			log("Type not found");
			return;
		}

		var ctx = new Stats(this);
		for( b in blocks )
			if( b.type == lt ) {
				var tl = [];
				var owner = b.owner;
				if( owner != null ) {
					tl.push(owner.type == null ? 0 : owner.type.tid);
					var k : Int = up;
					while( owner.owner != null && k-- > 0 ) {
						owner = owner.owner;
						tl.unshift(owner.type == null ? 0 : owner.type.tid);
					}
				}
				ctx.addPath(tl, b.getMemSize());
			}
		ctx.print();
	}

	function printPartition() {
		var part = sys.io.File.write("part.txt",false);
		for( p in pages ) {
			var bid = p.firstBlock;
			part.writeString(p.addr.toString()+ " [" + p.blockSize+"]");
			while( bid < p.maxBlocks ) {
				if( !p.isLiveBlock(bid) ) {
					part.writeByte(".".code);
					bid++;
					continue;
				}
				var sz = p.getBlockSize(bid);
				for( i in 0...sz ) part.writeByte("#".code);
				bid += sz;
			}
			part.writeByte("\n".code);
		}
		part.close();
		log("Partition saved in part.txt");
	}

	public function log(msg:String) {
		Sys.println(msg);
	}

	static function main() {
		var m = new Memory();

		//hl.Gc.dumpMemory(); Sys.command("cp memory.hl test.hl");

		var args = Sys.args();
		while( args.length > 0 ) {
			var arg = args.shift();
			if( StringTools.endsWith(arg, ".hl") ) {
				m.loadBytecode(arg);
				continue;
			}
			m.loadMemory(arg);
		}
		m.check();

		var stdin = Sys.stdin();
		while( true ) {
			Sys.print("> ");
			var args = ~/ +/g.split(StringTools.trim(stdin.readLine()));
			var cmd = args.shift();
			switch( cmd ) {
			case "exit", "quit", "q":
				break;
			case "types":
				m.printByType();
			case "stats":
				m.printStats();
			case "false":
				m.printFalsePositives(args.shift());
			case "unknown":
				m.printUnknown();
			case "locate":
				m.locate(args.shift(), Std.parseInt(args.shift()));
			case "part":
				m.printPartition();
			default:
				Sys.println("Unknown command " + cmd);
			}
		}
	}

}
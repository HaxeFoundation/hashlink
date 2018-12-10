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

	public function print( withSum = false ) {
		allT.sort(function(i1, i2) return i1.mem - i2.mem);
		var totCount = 0;
		var totMem = 0;
		for( i in allT ) {
			totCount += i.count;
			totMem += i.mem;
			mem.log(i.count + " count, " + Memory.MB(i.mem) + " " + [for( tid in i.tl ) mem.types[tid].toString()].join(" > "));
		}
		if( withSum )
			mem.log("Total: "+totCount+" count, "+Memory.MB(totMem));
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
	var tdynObj : TType;
	var tdynObjData : TType;
	var pointerBlock : PointerMap<Block>;
	var pointerType : PointerMap<TType>;
	var falseCandidates : Array<{ b : Block, f : Block, idx : Int }>;

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
		var low = memoryDump.readInt32();
		var high = is64 ? memoryDump.readInt32() : 0;
		return cast haxe.Int64.make(high,low);
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

		pointerType = new PointerMap();
		var cid = 0;
		types = [for( i in 0...code.types.length ) new TType(i, code.types[i])];
		for( i in 0...typesPointers.length ) {
			pointerType.set(typesPointers[i], types[i]);
			switch( code.types[i] ) {
			case HFun(f):
				var tid = types.length;
				var args = f.args.copy();
				var clparam = args.shift();
				if( clparam == null ) {
					cid++;
					continue;
				}
				switch( clparam ) {
				case HEnum(p) if( p.name == "" ):
					p.name = '<closure$i context>';
				default:
				}
				var ct = new TType(tid, HFun({ args : args, ret : f.ret }), clparam);
				types.push(ct);
				var pt = closuresPointers[cid++];
				if( pt != null )
					pointerType.set(pt, ct);
			case HObj(o):
				if( o.tsuper != null )
					for( j in 0...types.length )
						if( types[j].t == o.tsuper ) {
							types[i].parentClass = types[j];
							break;
						}
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
		var pageMem = new PointerMap<Page>();
		var progress = 0;
		pointerBlock = new PointerMap();

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

				// NoPtr page can also have a type ptr
				goto(b);
				b.type = pointerType.get(readPointer());
				if( b.type != null && b.type.hasPtr && p.kind == PNoPtr )
					b.type = null; // false positive
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
		broot.depth = 0;
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
		tdynObjData = new TType(types.length, HAbstract("dynobjdata"));
		types.push(tdynObjData);

		toProcess = blocks.copy();
		falseCandidates = [];
		while( toProcess.length > 0 )
			buildHierarchy();

		// look in stacks (low priority of ownership)
		var tstacks = new TType(types.length, HAbstract("stack"));
		var bstacks = [];
		types.push(tstacks);
		for( s in stacks ) {
			var bstack = new Block();
			bstack.depth = 10000;
			bstack.type = tstacks;
			bstack.addParent(all);
			bstacks.push(bstack);
			for( r in s.contents ) {
				var b = pointerBlock.get(r);
				if( b != null )
					b.addParent(bstack);
			}
		}

		for( f in falseCandidates )
			if( f.f.owner == null ) {
				f.f.addParent(f.b);
				f.b.type.falsePositive++;
				f.b.type.falsePositiveIndexes[f.idx]++;
			}

		// assign depths

		Sys.println("Computing depths...");
		broot.markDepth();
		for( b in bstacks ) b.markDepth();

		var changed = -1;
		while( changed != 0 ) {
			changed = 0;
			for( b in blocks ) {
				var minD = -1;
				if( b.parents == null ) {
					if( b.owner != null && b.owner.depth >= 0 )
						minD = b.owner.depth;
				} else {
					for( p in b.parents )
						if( p.depth >= 0 && (minD < 0 || p.depth < minD) )
							minD = p.depth;
				}
				if( minD >= 0 ) {
					minD++;
					if( b.depth < 0 || b.depth > minD ) {
						b.depth = minD;
						changed++;
					}
				}
			}
		}

		for( b in blocks )
			b.finalize();

		var unk = 0, unkMem = 0, unRef = 0;
		for( b in blocks ) {
			if( b.owner == null ) {
				unRef++;
				if( unRef < 100 )
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

		log("Hierarchy built, "+unk+" unknown ("+MB(unkMem)+"), "+falseCount+" false positives, "+unRef+" unreferenced");
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
			var ptrTags = null;
			if( b.type != null ) {
				fields = b.type.fields;
				ptrTags = b.type.ptrTags;
				// enum
				if( b.type.constructs != null ) {
					start++;
					fields = b.type.constructs[readInt()];
					if( is64 ) readInt(); // skip, not a pointer anyway
				}
			}

			for( i in start...(b.getMemSize() >> ptrBits) ) {
				var r = readPointer();

				//if( ptrTags != null && ((ptrTags.get(i >> 5) >>> (i & 31)) & 1) == 0 ) continue;

				var bs = pointerBlock.get(r);
				if( bs == null ) continue;
				var ft = fields != null ? fields[i] : null;

				if( b.type == tdynObj && (i == 1 || i == 2 || i == 3) ) {
					if( bs.typeKind != KHeader && (bs.typeKind != null || bs.type != null) )
						trace(bs.typeKind, bs.type);
					else {
						bs.type = tdynObjData;
						bs.typeKind = KDynObjData;
					}
				}

				if( ft != null && !ft.t.isPtr() ) {
					falseCandidates.push({ b : b, f:bs, idx : i });
					continue;
				}
				bs.addParent(b);

				if( bs.type == null && ft != null ) {
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

	function printByType() {
		var ctx = new Stats(this);
		for( b in blocks )
			ctx.add(b.type, b.getMemSize());
		ctx.print();
	}

	function resolveType( str ) {
		for( t in types )
			if( t.t.toString() == str )
				return t;
		log("Type not found '"+str+"'");
		return null;
	}

	function locate( tstr : String, up = 0 ) {
		var lt = resolveType(tstr);
		if( lt == null ) return;

		var ctx = new Stats(this);
		for( b in blocks )
			if( b.type != null && b.type.match(lt) ) {
				var tl = [];
				var owner = b.owner;
				if( owner != null ) {
					var ol = [owner];
					tl.push(owner.type == null ? 0 : owner.type.tid);
					var k : Int = up;
					while( owner.owner != null && k-- > 0 && ol.indexOf(owner.owner) < 0 && owner.owner != all ) {
						var prev = owner;
						owner = owner.owner;
						ol.push(owner);
						tl.unshift(owner.type == null ? 0 : owner.type.tid);
					}
				}
				ctx.addPath(tl, b.getMemSize());
			}
		ctx.print();
	}

	function count( tstr : String, excludes : Array<String> ) {
		var t = resolveType(tstr);
		if( t == null ) return;
		var texclude = [];
		for( e in excludes ) {
			var t = resolveType(e);
			if( t == null ) return;
			texclude.push(t);
		}
		var ctx = new Stats(this);
		Block.MARK_UID++;
		var mark = [];
		for( b in blocks )
			if( b.type == t )
				visitRec(b,ctx,[],mark);
		while( mark.length > 0 ) {
			var b = mark.pop();
			for( s in b.subs )
				visitRec(s,ctx,texclude,mark);
		}
		ctx.print(true);
	}

	function visitRec( b : Block, ctx : Stats, exclude : Array<TType>, mark : Array<Block> ) {
		if( b.mark == Block.MARK_UID ) return;
		b.mark = Block.MARK_UID;
		if( b.type != null ) for( t in exclude ) if( b.type.match(t) ) return;
		ctx.addPath(b.type == null ? [] : [b.type.tid],b.getMemSize());
		if( b.subs != null )
			mark.push(b);
	}

	function parents( tstr : String, up = 0 ) {
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
			if( b.type == lt )
				for( b in b.getParents() )
					ctx.addPath([if( b.type == null ) 0 else b.type.tid], 0);
		ctx.print();
	}

	function subs( tstr : String, down = 0 ) {
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
		var mark = new Map();
		for( b in blocks )
			if( b.type == lt ) {
				function addRec(tl:Array<Int>,b:Block, k:Int) {
					if( k < 0 ) return;
					if( mark.exists(b) )
						return;
					mark.set(b, true);
					tl.push(b.type == null ? 0 : b.type.tid);
					ctx.addPath(tl, b.getMemSize());
					if( b.subs != null ) {
						k--;
						for( s in b.subs )
							addRec(tl.copy(),s, k);
					}
				}
				addRec([], b, down);
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

	static inline var BMP_BITS = 5;

	function makeBitmap() {

		// VMMap.exe tool can also be used to show address space fragmentation in realtime
		// although it will not be able to show GC fragmentation

		var totalMem = 2 * 1024. * 1024. * 1024.; // GB of memory
		var sizeReal = Math.ceil(Math.sqrt(totalMem / (1 << BMP_BITS)));
		var size = 1, bits = 1;
		while( size < sizeReal ) {
			size <<= 1;
			bits++;
		}
		var bytes = haxe.io.Bytes.alloc(size * size * 4);
		var bmp : hl.BytesAccess<Int> = (bytes : hl.Bytes);
		for( i in 0...size * size )
			bmp[i] = 0xFF000000;
		for( p in pages ) {
			var index = p.addr.shift(BMP_BITS).low;
			// reserved
			for( i in 0...(p.size >> BMP_BITS) )
				bmp[index+i] = 0xFF808080;
			for( i in 0...((p.firstBlock * p.blockSize) >> BMP_BITS) )
				bmp[index + i] = 0xFFFFFF00; // YELLOW = GC MEMORY
		}
		for( b in blocks ) {
			var index = b.getPointer().shift(BMP_BITS).low;
			var color = b.page.memHasPtr() ? 0xFFFF0000 : 0xFF00FF00; // GREEN = data / RED = objects
			for( i in 0...b.getMemSize() >> BMP_BITS )
				bmp[index + i] = color;
		}
		var f = sys.io.File.write("bitmap.png");
		new format.png.Writer(f).write(format.png.Tools.build32BGRA(size, size, bytes));
		f.close();
	}

	function printPages() {
		var perBlockSize = new Map();
		for( p in pages ) {
			var inf = perBlockSize.get(p.blockSize);
			if( inf == null ) {
				inf = {
					blockSize : p.blockSize,
					count : 0,
					mem : 0.,
					kinds : [],
					blocks : [],
				};
				perBlockSize.set(p.blockSize, inf);
			}
			inf.count++;
			inf.mem += p.size;
			inf.kinds[cast p.kind]++;
		}
		for( b in blocks ) {
			var inf = perBlockSize.get(b.page.blockSize);
			inf.blocks[cast b.page.kind]++;
		}
		var all = Lambda.array(perBlockSize);
		all.sort(function(i1, i2) return i1.blockSize - i2.blockSize);
		log("Kinds = [Dyn,Raw,NoPtr,Finalizer]");
		for( i in all )
			log('BlockSize ${i.blockSize} : ${i.count} pages, ${Std.int(i.mem/1024)} KB, kinds = ${i.kinds}, blocks = ${i.blocks}');
	}

	static function main() {
		var m = new Memory();

		//hl.Gc.dumpMemory(); Sys.command("cp memory.hl test.hl");

		var code = null, memory = null;
		var args = Sys.args();
		while( args.length > 0 ) {
			var arg = args.shift();
			if( StringTools.endsWith(arg, ".hl") ) {
				code = arg;
				m.loadBytecode(arg);
				continue;
			}
			memory = arg;
			m.loadMemory(arg);
		}
		if( code != null && memory == null ) {
			memory = new haxe.io.Path(code).dir+"/hlmemory.dump";
			if( sys.FileSystem.exists(memory) ) m.loadMemory(memory);
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
			case "count":
				m.count(args.shift(), args);
			case "parents":
				m.parents(args.shift());
			case "subs":
				m.subs(args.shift(), Std.parseInt(args.shift()));
			case "part":
				m.printPartition();
			case "bitmap":
				m.makeBitmap();
			case "pages":
				m.printPages();
			default:
				Sys.println("Unknown command " + cmd);
			}
		}
	}

}
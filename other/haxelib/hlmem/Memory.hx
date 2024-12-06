package hlmem;

import hlmem.Block;
import format.hl.Data;
using format.hl.Tools;

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

	public inline function makeID( t : TType, field : Int ) {
		return t.tid | (field << 24);
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
		sort( @:privateAccess mem.sortByCount );
		var totCount = 0;
		var totMem = 0;
		var max = @:privateAccess mem.maxLines;
		if( max > 0 && allT.length > max ) {
			mem.log("<ignore "+(allT.length - max)+" lines>");
			allT = allT.slice(allT.length - max);
		}
		for( i in allT ) {
			totCount += i.count;
			totMem += i.mem;
			var tpath = getPathStrings(mem, i.tl);
			mem.log(Memory.withColor(i.count + " count, " + Memory.MB(i.mem) + " ", 33) + tpath.join(Memory.withColor(' > ', 36)));
		}
		if( withSum )
			mem.log("Total: "+totCount+" count, "+Memory.MB(totMem));
	}

	public static function getTypeString(mem : Memory, id : Int){
		var tid = id & 0xFFFFFF;
		var t = mem.types[tid];
		var tstr = t.toString();
		tstr += Memory.withColor("#" + tid, 35);
		var fid = id >>> 24;
		if( fid > 0 ) {
			var f = t.memFieldsNames[fid-1];
			if( f != null ) tstr += "." + f;
		}
		return tstr;
	}

	public static function getPathStrings(mem : Memory, i : Array<Int>){
		var tpath = [];
		for( tid in i )
			tpath.push(getTypeString(mem, tid));

		return tpath;
	}

	public function sort( byCount = true , asc = true) {
		if( byCount )
			allT.sort(function(i1, i2) return asc ? i1.count - i2.count : i2.count - i1.count);
		else
			allT.sort(function(i1, i2) return asc ? i1.mem - i2.mem : i2.mem - i1.mem);
	}
}

enum FieldsMode {
	Full;
	Parents;
	None;
}

enum FilterMode {
	None;
	Intersect;	// Will display only blocks present in all memories
	Unique;		// Will display only blocks not present in other memories
}

class Memory {
	public var memoryDump : sys.io.FileInput;

	#if js
	var memBytes : haxe.io.Bytes;
	var memPos : Int;
	#end

	public var is64 : Bool;
	public var bool32 : Bool;
	public var ptrBits : Int;

	public var types : Array<TType>;

	var otherMems : Array<Memory>;
	var filterMode: FilterMode = None;

	var memFile : String;

	var privateData : Int;
	var markData : Int;

	var sortByCount : Bool;
	var displayFields : FieldsMode = Full;
	var displayProgress = true;

	var code : format.hl.Data;
	var pages : Array<Page>;
	var roots : Array<Pointer>;
	var stacks : Array<Stack>;
	var typesPointers : Array<Pointer>;
	var closuresPointers : Array<Pointer>;
	var blocks : Array<Block>;
	var filteredBlocks : Array<Block> = [];
	var baseTypes : Array<{ t : HLType, p : Pointer }>;
	var all : Block;

	var toProcess : Array<Block>;
	var tdynObj : TType;
	var tdynObjData : TType;
	var pointerBlock : PointerMap<Block>;
	var pointerType : PointerMap<TType>;
	var falseCandidates : Array<{ b : Block, f : Block, idx : Int }>;

	var currentTypeIndex = 0;
	var resolveCache : Map<String,TType> = new Map();
	var maxLines : Int = 100;

	function new() {
	}

	public function typeSize( t : HLType ) {
		return switch( t ) {
		case HVoid: 0;
		case HUi8: 1;
		case HUi16: 2;
		case HI32, HF32: 4;
		case HI64, HF64: 8;
		case HBool:
			return bool32 ? 4 : 1;
		case HPacked(_), HAt(_):
			throw "assert";
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
		#if js
		var ch1 = memBytes.get(memPos);
		var ch2 = memBytes.get(memPos + 1);
		var ch3 = memBytes.get(memPos + 2);
		var ch4 = memBytes.get(memPos + 3);
		memPos += 4;
		return memoryDump.bigEndian ? ch4 | (ch3 << 8) | (ch2 << 16) | (ch1 << 24) : ch1 | (ch2 << 8) | (ch3 << 16) | (ch4 << 24);
		#else
		return memoryDump.readInt32();
		#end
	}

	inline function readPointer() : Pointer {
		#if js
		var low = readInt();
		var high = is64 ? readInt() : 0;
		#else
		var low = memoryDump.readInt32();
		var high = is64 ? memoryDump.readInt32() : 0;
		#end

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
		memFile = arg;
		memoryDump = sys.io.File.read(arg);

		var version = 0;

		#if js
		memBytes = sys.io.File.getBytes(arg);
		memPos = 0;

		if( memBytes.getString(memPos, 3) != "HMD" )
			throw "Invalid memory dump file";

		memPos += 3;

		version = memBytes.get(memPos) - "0".code;
		memPos += 1;

		#else
		if( memoryDump.read(3).toString() != "HMD" )
			throw "Invalid memory dump file";

		version = memoryDump.readByte() - "0".code;
		#end

		if( version != 1 )
			throw "Unsupported format version "+version;

		var flags = readInt();
		is64 = (flags & 1) != 0;
		bool32 = (flags & 2) != 0;
		ptrBits = is64 ? 3 : 2;
		var ptrSize = 1 << ptrBits;

		privateData = readInt();
		markData = readInt();

		// load pages
		var count = readInt();
		pages = [];
		blocks = [];
		for( i in 0...count ) {
			var addr = readPointer();
			var p = new Page();
			p.addr = addr;
			p.kind = cast readInt();
			p.size = readInt();
			p.reserved = readInt();

			var readPtr = !p.memHasPtr();
			while( true ) {
				var ptr = readPointer();
				if( ptr.isNull() ) break;
				var size = readInt();
				var b = new Block();
				b.page = p;
				b.addr = ptr;
				b.size = size;
				b.typePtr = @:privateAccess new Pointer(0);
				b.owner = null;
				b.typeKind = null;
				b.subs = null;
				b.parents = null;

				if( readPtr && size >= ptrSize ) b.typePtr = readPointer();
				blocks.push(b);
			}

			if( p.memHasPtr() ) {
				#if js
				p.dataPosition = memPos;
				memPos += p.size;
				#else
				p.dataPosition = memoryDump.tell();
				memoryDump.seek(p.size, SeekCur);
				#end
			}
			pages.push(p);
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

	public function getStats() {
		var pagesSize = 0, reserved = 0;
		var used = 0, gc = 0;
		var fUsed = 0;
		for( p in pages ) {
			pagesSize += p.size;
			reserved += p.reserved;
		}
		for( b in blocks )
			used += b.size;
		for (b in filteredBlocks)
			fUsed += b.size;

		var objs = [];
		var obj = {
			memFile : memFile,
			free: pagesSize - used - reserved,
			used: used,
			filterUsed: fUsed,
			totalAllocated: pagesSize - reserved,
			gc : privateData + markData,
			pagesCount : pages.length,
			pagesSize : pagesSize,
			rootsCount : roots.length,
			stackCount : stacks.length,
			typesCount : code.types.length,
			closuresCount : closuresPointers.length,
			blockCount : blocks.length
		};

		objs.push(obj);
		for (m in otherMems??[]) objs = objs.concat(m.getStats());

		return objs;
	}

	function printStats() {
		var objs = getStats();

		for (obj in objs) {
			log(withColor("--- " + obj.memFile + " ---", 36));
			log(obj.pagesCount + " pages, " + MB(obj.pagesSize) + " memory");
			log(obj.rootsCount + " roots, "+ obj.stackCount + " stacks");
			log(obj.typesCount + " types, " + obj.closuresCount + " closures");
			log(obj.blockCount + " live blocks " + MB(obj.used) + " used, " + MB(obj.free) + " free, "+MB(obj.gc)+" gc");
			if (filterMode != None)
				log(filteredBlocks.length + " blocks in filter " + MB(obj.filterUsed) + " used");
		}
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
		var p = b.page.dataPosition;
		if( p < 0 ) throw "assert";

		#if js
		memPos = p + b.addr.sub(b.page.addr);
		#else
		memoryDump.seek(p + b.addr.sub(b.page.addr), SeekBegin);
		#end
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
				if( !pt.isNull() )
					pointerType.set(pt, ct);
			case HObj(o), HStruct(o):
				if( o.tsuper != null ) {
					var found = false;
					for( j in 0...types.length )
						if( types[j].t == o.tsuper ) {
							types[i].parentClass = types[j];
							found = true;
							break;
						}
					if( !found ) throw "Missing parent class";
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

		var progress = 0;
		pointerBlock = new PointerMap();

		var missingTypes = 0;
		for( b in blocks ) {
			progress++;
			if( displayProgress && progress % 1000 == 0 )
				Sys.print((Std.int((progress / blocks.length) * 1000.0) / 10) + "%  \r");
			if( b.page.kind == PDynamic ) {
				goto(b);
				b.typePtr = readPointer();
			}
			b.type = pointerType.get(b.typePtr);
			if( b.page.kind == PDynamic && b.type == null && b.typePtr != Pointer.NULL )
				missingTypes++; // types that we don't have in our dump
			b.typePtr = Pointer.NULL;
			if( b.type != null ) {
				switch( b.page.kind ) {
				case PDynamic:
				case PNoPtr:
					if( b.type.hasPtr ) {
						if( b.type.t.match(HEnum(_)) ) {
							// most likely one of the constructor without pointer parameter
						} else
							b.type = null; // false positive
					}
				case PRaw, PFinalizer:
					if( b.type.isDyn )
						b.type = null; // false positive
				}
			}
			if( b.type != null && !b.type.isDyn )
				b.type = getTypeNull(b.type);
			if( b.type != null )
				b.typeKind = KHeader;
			pointerBlock.set(b.addr, b);
		}

		//Sys.println(missingTypes+" blocks with unresolved type");

		printStats();

		// look in roots (higher ownership priority)
		all = new Block();

		var broot = new Block();
		broot.type = new TType(types.length, HAbstract("roots"));
		types.push(broot.type);
		broot.depth = 0;
		broot.addParent(all);

		for( r in roots ) {
			var b = pointerBlock.get(r);
			if( b == null ) continue;
			b.addParent(broot);
			if( b.type == null )
				b.typeKind = KRoot;
		}

		var tinvalid = new TType(types.length, HAbstract("invalid"));
		types.push(tinvalid);
		for( t in types )
			t.buildTypes(this, tinvalid);

		var tunknown = new TType(types.length, HAbstract("unknown"));
		types.push(tunknown);

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

		// precompute Arrays (no NativeArray intermediate)
		function shortCircuit( native, haxe ) {
			var tnat = resolveType(native, false);
			var tarr = resolveType(haxe, false);
			if( tnat == null || tarr == null ) return;
			for( b in blocks ) {
				if( b.type == tnat && b.owner != null && b.owner.type == tarr && b.subs != null ) {
					for( s in b.subs )
						s.b.addParent(b.owner);
				}
			}
		}
		shortCircuit("hl.NativeArray","Array<T>");
		// disable for now, this generates unknowns and "Void" links
		//shortCircuit("hl_bytes_map","Map<String,Dynamic>");
		//shortCircuit("hl_int_map","Map<Int,Dynamic>");
		//shortCircuit("hl_obj_map","Map<{},Dynamic>");

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
					log("  "+b.addr.toString()+"["+b.size+"] is not referenced");
				continue;
			}

			if( b.type != null )
				continue;

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

			b.type = tunknown;
		}

		var falseCount = 0;
		for( t in types )
			falseCount += t.falsePositive;

		log("Hierarchy built, "+falseCount+" false positives, "+unRef+" unreferenced");
	}

	function printFalsePositives( ?typeStr : String ) {
		var falses = [for( t in types ) if( t.falsePositive > 0 && (typeStr == null || t.toString().indexOf(typeStr) >= 0) ) t];
		falses.sort(function(t1, t2) return t1.falsePositive - t2.falsePositive);
		for( f in falses )
			log(f.falsePositive+" count " + f + " "+f.falsePositiveIndexes+"\n    "+[for( f in f.memFields ) f.t.toString()]);
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
			inf.mem += b.size;
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
			if( displayProgress && progress % 10000 == 0 )
				Sys.print((Std.int(progress * 1000.0 / blocks.length) / 10) + "%  \r");

			if( !b.page.memHasPtr() )
				continue;

			if( b.type != null && !b.type.hasPtr )
				log("  Scanning "+b.type+" "+b.addr.toString());

			goto(b);
			var fields = null;
			var start = 0;
			var ptrTags = null;
			var hasFieldNames = false;
			if( b.type != null ) {
				hasFieldNames = b.type.memFieldsNames != null;
				fields = b.type.memFields;
				ptrTags = b.type.ptrTags;
				// enum
				if( b.type.constructs != null ) {
					readPointer(); // type
					var index = readInt();
					fields = b.type.constructs[index];
					if( is64 ) readInt(); // skip, not a pointer anyway
					start += 2;
				}
			}

			for( i in start...(b.size >> ptrBits) ) {
				var r = readPointer();

				var bs = pointerBlock.get(r);
				if( bs == null ) continue;
				var ft = fields != null ? fields[i] : null;

				if( ptrTags != null && ((ptrTags.get(i >> 3) >>> (i & 7)) & 1) == 0 && !b.type.t.match(HVirtual(_)) )
					continue;

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
				bs.addParent(b,hasFieldNames ? (i+1) : 0);

				if( bs.type == null && ft != null ) {
					if( ft.t.match(HDyn | HObj(_)) ) {
						// we can't infer with a polymorph type
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
		for( b in filteredBlocks )
			ctx.add(b.type, b.size);
		ctx.print();
	}

	function resolveType( str, showError = true ) {
		var t = resolveCache.get(str);
		if( t != null )
			return t;
		for( i in currentTypeIndex...types.length ) {
			var t = types[i];
			var tstr = t.toString();
			if (tstr != null) {
				resolveCache.set(tstr, t);
				currentTypeIndex = i + 1;
				if( tstr == str )
					return t;
			}
		}
		if( showError )
			log("Type not found '"+str+"'");
		return null;
	}

	function locate( tstr : String, up = 0 ) {
		var ctx = new Stats(this);

		var lt = null;
		if (StringTools.startsWith(tstr, "#")) {
			var id = Std.parseInt(tstr.substr(1, tstr.length));
			if (id != null && id >= 0) {
				lt = types[id];
			}
		} else {
			lt = resolveType(tstr);
		}

		if( lt == null ) return ctx;

		inline function isVirtualField(t) { t >>>= 24; return t == 1 || t == 2; }


		for( b in filteredBlocks )
			if( b.type != null && b.type.match(lt) ) {
				var tl = [];
				var owner = b.owner;
				// skip first virtual field
				if( lt.t != HDynObj && owner != null && owner.type != null && owner.type.t.match(HVirtual(_)) && isVirtualField(owner.makeTID(b,true)) )
					owner = owner.owner;

				if( owner != null ) {
					tl.push(owner.makeTID(b,displayFields == Full));
					var k : Int = up;
					while( owner.owner != null && k-- > 0 && owner.owner != all ) {
						var tag = owner.owner.makeTID(owner,displayFields != None);
						owner = owner.owner;
						// remove recursive sequence
						for( i => tag2 in tl )
							if( tag2 == tag ) {
								var seq = true;
								for( n in 0...i ) {
									if( tl[n] != tl[i+1+n] )
										seq = false;
								}
								if( seq ) {
									for( k in 0...i ) tl.shift();
									tag = -1;
									k += i + 1;
								}
								break;
							}
						// don't display virtual wrappers
						if( displayFields != None && owner.type != null && isVirtualField(tag) && owner.type.t.match(HVirtual(_)) ) {
							tag = -1;
							k++;
						}
						if( tag != -1 )
							tl.unshift(tag);
					}
				}
				ctx.addPath(tl, b.size);
			}

		ctx.print();
		return ctx;
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
		for( b in filteredBlocks )
			if( b.type == t )
				visitRec(b,ctx,[],mark);
		while( mark.length > 0 ) {
			var b = mark.pop();
			for( s in b.subs )
				visitRec(s.b,ctx,texclude,mark);
		}
		ctx.print(true);
	}

	function visitRec( b : Block, ctx : Stats, exclude : Array<TType>, mark : Array<Block> ) {
		if( b.mark == Block.MARK_UID ) return;
		b.mark = Block.MARK_UID;
		if( b.type != null ) for( t in exclude ) if( b.type.match(t) ) return;
		ctx.addPath(b.type == null ? [] : [b.type.tid],b.size);
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
		for( b in filteredBlocks )
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
		for( b in filteredBlocks )
			if( b.type == lt ) {
				function addRec(tl:Array<Int>,b:Block, k:Int) {
					if( k < 0 ) return;
					if( mark.exists(b) )
						return;
					mark.set(b, true);
					tl.push(b.type == null ? 0 : b.type.tid);
					ctx.addPath(tl, b.size);
					if( b.subs != null ) {
							k--;
						for( s in b.subs )
							addRec(tl.copy(),s.b, k);
					}
				}
				addRec([], b, down);
			}
		ctx.print();
	}

	public function setFilterMode(m: FilterMode) {
		filterMode = m;
		switch( m ) {
		case None:
			filteredBlocks = blocks.copy();
		default:
			filteredBlocks = [];
			var progress = 0;
			for( b in blocks ) {
				progress++;
				if( displayProgress && progress % 1000 == 0 )
					Sys.print((Std.int((progress / blocks.length) * 1000.0) / 10) + "%  \r");
				if( !isBlockIgnored(b, m) )
					filteredBlocks.push(b);
			}
			if( displayProgress )
				Sys.print("       \r");
		}
	}
	public function isBlockIgnored(b: Block, m: FilterMode) {
		switch( m ) {
		case None:
			return false;
		case Intersect:
			for( m in otherMems ) {
				var b2 = m.pointerBlock.get(b.addr);
				if( b2 == null || b2.typePtr != b.typePtr || b2.size != b.size)
					return true;
			}
		case Unique:
			for( m in otherMems ) {
				var b2 = m.pointerBlock.get(b.addr);
				if( b2 != null && b2.typePtr == b.typePtr && b2.size == b.size )
					return true;
			}
		}
		return false;
	}

	public function log(msg:String) {
		Sys.println(msg);
	}

	static function parseArgs(str: String) {
		str = StringTools.trim(str);
		var i = 0;
		var tok = "";
		var args = [];
		var escape = false;
		while(i != str.length) {
			var c = str.charAt(i++);
			if(c == '"') {
				escape = !escape;
			}
			else {
				if(c == " " && !escape) {
					if(tok.length > 0) args.push(tok);
					tok = "";
				}
				else
					tok += c;
			}
		}
		if(tok.length > 0) args.push(tok);
		return args;
	}

	static var useColor = true;
	static function main() {
		var m = new Memory();
		var others: Array<Memory> = [];
		var filterMode: FilterMode = None;

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
			if( arg == "-c" || arg == "--color" )
				continue;
			if( arg == "--no-color" ) {
				useColor = false;
				continue;
			}
			if( arg == "--args" ) {
				m.displayProgress = false;
				break;
			}
			if (memory == null) {
				memory = arg;
				m.loadMemory(arg);
			} else {
				var m2 = new Memory();
				m2.loadMemory(arg);
				others.push(m2);
			}
		}
		if( code != null && memory == null ) {
			var dir = new haxe.io.Path(code).dir;
			if( dir == null ) dir = ".";
			memory = dir+"/hlmemory.dump";
			if( sys.FileSystem.exists(memory) ) m.loadMemory(memory);
		}

		m.check();
		for (m2 in others) {
			m2.code = m.code;
			m2.check();
		}
		m.otherMems = [for (i in others) i];
		m.setFilterMode(filterMode);

		var stdin = Sys.stdin();
		while( true ) {
			Sys.print(withColor("> ", 31));
			var args = parseArgs(args.length > 0 ? args.shift() : stdin.readLine());
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
			case "sort":
				switch( args.shift() ) {
				case "mem":
					m.sortByCount = false;
				case "count":
					m.sortByCount = true;
				case mode:
					Sys.println("Unknown sort mode " + mode);
				}
			case "fields":
				switch( args.shift() ) {
				case "full":
					m.displayFields = Full;
				case "none":
					m.displayFields = None;
				case "parents":
					m.displayFields = Parents;
				case mode:
					Sys.println("Unknown fields mode " + mode);
				}
			case "filter":
				switch( args.shift() ) {
				case "none":
					filterMode = None;
				case "intersect":
					filterMode = Intersect;
				case "unique":
					filterMode = Unique;
				case mode:
					Sys.println("Unknown filter mode " + mode);
				}
				m.setFilterMode(filterMode);
			case "nextDump":
				others.push(m);
				m = others.shift();
				m.otherMems = [for (i in others) i];
				m.setFilterMode(filterMode);
				var ostr = others.length > 0 ? (" (others are " + others.map(m -> m.memFile) + ")") : "";
				Sys.println("Using dump " + m.memFile + ostr);
			case "lines":
				var v = args.shift();
				if( v != null )
					m.maxLines = Std.parseInt(v);
				Sys.println(m.maxLines == 0 ? "Lines limit disabled" : m.maxLines + " maximum lines displayed");
			case null:
				Sys.println("");
			default:
				Sys.println("Unknown command " + cmd);
			}
		}
	}

	// A list of ansi colors is available at
	// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797#8-16-colors
	public static function withColor(str: String, ansiCol: Int) {
		if (!useColor)
			return str;
		return "\x1B[" + ansiCol + "m" + str + "\x1B[0m";
	}
}
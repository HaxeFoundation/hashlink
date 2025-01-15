package hlmem;

import hlmem.Block;
import hlmem.Result;
import format.hl.Data;
using format.hl.Tools;

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
	var analyzer : Analyzer;
	var code : format.hl.Data;
	var buildDone : Bool = false;

	// Load from dump
	public var memFile : String;
	var memFileReader : FileReader;
	public var is64 : Bool;
	public var bool32 : Bool;
	public var ptrBits : Int;
	var privateData : Int;
	var markData : Int;
	var pages : Array<Page>;
	var blocks : Array<Block>;
	var roots : Array<Pointer>;
	var stacks : Array<Stack>;
	var baseTypes : Array<{ t : HLType, p : Pointer }>;
	var typesPointers : Array<Pointer>;
	var closuresPointers : Array<Pointer>;

	// Build using dump + code
	var types : Array<TType>;
	var pointerType : PointerMap<TType>;
	var pointerBlock : PointerMap<Block>;
	var all : Block;
	var tdynObj : TType;
	var tdynObjData : TType;
	var falseCandidates : Array<{ b : Block, f : Block, idx : Int }>;

	// Cache
	var memStats : MemStats = null;
	var resolveLastIndex : Int = 0;
	var resolveCache : Map<String,TType> = new Map();

	// Filter
	public var filterMode : FilterMode = None;
	public var otherMems : Array<Memory> = [];
	var filteredBlocks : Array<Block> = [];

	// ----- Init -----

	public function new( analyzer : Analyzer ) {
		this.analyzer = analyzer;
	}

	// ----- Type -----

	public function typeSize( t : HLType ) : Int {
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

	public function getTypeById( tid : Int ) : TType {
		var tid = tid & 0xFFFFFF;
		return types[tid];
	}

	public function getTypeString( id : Int, withTstr : Bool, withId : Bool, withField : Bool ) : String {
		var tid = id & 0xFFFFFF;
		var t = types[tid];
		var tstr = withTstr ? "" + t.toString() : "";
		if( withId )
			tstr += Analyzer.withColor("#" + tid, Magenta);
		var fid = id >>> 24;
		if( withField && fid > 0 ) {
			var f = t.memFieldsNames[fid-1];
			if( f != null ) tstr += "." + f;
		}
		return tstr;
	}

	// ----- Load & Build -----

	inline function readInt() : Int {
		return memFileReader.readInt32();
	}

	inline function readPointer() : Pointer {
		return cast memFileReader.readPointer(is64);
	}

	function goto( b : Block ) {
		var p = b.page.dataPosition;
		if( p < 0 ) throw "assert";
		memFileReader.seek(p + b.addr.sub(b.page.addr), SeekBegin);
	}

	public function load( file : String ) {
		memFile = file;
		memFileReader = new FileReader(file);

		var version = 0;
		if( memFileReader.readString(3) != "HMD" )
			throw "Invalid memory dump file";

		version = memFileReader.readByte() - "0".code;

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
				b.typePtr = Pointer.NULL;
				b.owner = null;
				b.typeKind = null;
				b.subs = null;
				b.parents = null;

				if( readPtr && size >= ptrSize ) b.typePtr = readPointer();
				blocks.push(b);
			}

			if( p.memHasPtr() ) {
				p.dataPosition = memFileReader.tell();
				memFileReader.seek(p.size, SeekCur);
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

		Analyzer.log(file + " dump loaded");
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

	public function build() {
		if( buildDone ) return;
		if( memFile == null ) throw "Missing .dump file";
		if( memFileReader == null ) memFileReader = new FileReader(memFile);

		buildBlockTypes(false);

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

		var toProcess = blocks.copy();
		falseCandidates = [];
		while( toProcess.length > 0 )
			toProcess = buildHierarchy(toProcess);

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

		Analyzer.log("Computing depths...");
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
					Analyzer.log("  "+b.addr.toString()+"["+b.size+"] is not referenced");
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

		buildDone = true;
		memFileReader.close();
		memFileReader = null;
		Analyzer.log("Hierarchy built, "+falseCount+" false positives, "+unRef+" unreferenced");
	}

	/**
	 * Can be used instead of build() to only initialize blocks for filter. Must be called after load().
	 */
	 public function buildBlockTypes( closeFile : Bool = true ) {
		if( types != null ) return; // Already built types
		if( code == null ) code = analyzer.code;
		if( code == null ) throw "Missing .hl file";
		if( code.types.length != typesPointers.length ) throw "Types count mismatch";

		var cid = 0;
		types = [for( i in 0...code.types.length ) new TType(i, code.types[i])];
		pointerType = new PointerMap();
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
			Analyzer.logProgress(progress, blocks.length);
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

		if( closeFile ) {
			memFileReader.close();
			memFileReader = null;
		}

		getMemStats().print();
		Analyzer.log("Block types built, " + missingTypes + " blocks with unresolved type");
	}

	function buildHierarchy( toProcess : Array<Block> ) : Array<Block> {
		var progress = 0;
		var blocks = toProcess;
		toProcess = [];

		for( b in blocks )
			b.removeChildren();

		for( b in blocks ) {
			progress++;
			Analyzer.logProgress(progress, blocks.length);

			if( !b.page.memHasPtr() )
				continue;

			if( b.type != null && !b.type.hasPtr )
				Analyzer.log("  Scanning "+b.type+" "+b.addr.toString());

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
		return toProcess;
	}

	// ----- Stats -----

	public function getMemStats() : MemStats {
		if( memStats != null )
			return memStats;
		var pagesSize = 0, reserved = 0;
		var used = 0;
		var fUsed = 0;
		for( p in pages ) {
			pagesSize += p.size;
			reserved += p.reserved;
		}
		for( b in blocks )
			used += b.size;
		for (b in filteredBlocks)
			fUsed += b.size;

		memStats = {
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
			blockCount : blocks.length,
			filterMode: filterMode,
			filteredBlockCount : filteredBlocks.length,
		};
		return memStats;
	}

	public function getFalsePositives( ?typeStr : String ) : FalsePositiveStats {
		var ctx = new FalsePositiveStats();
		for( t in types )
			if( t.falsePositive > 0 && (typeStr == null || ("" + t.toString()).indexOf(typeStr) >= 0) ) {
				ctx.add(t);
			}
		ctx.sort();
		return ctx;
	}

	public function getUnknown() : BlockStats {
		var ctx = new BlockStats(this);
		for( b in blocks ) {
			if( b.type != null ) continue;
			var o = b;
			while( o != null && o.type == null )
				o = o.owner;
			var t = o == null ? null : o.type;
			ctx.add(t, b.size);
		}
		return ctx;
	}

	public function getBlockStatsByType() : BlockStats {
		var ctx = new BlockStats(this);
		for( b in filteredBlocks )
			ctx.add(b.type, b.size);
		return ctx;
	}

	public function resolveType( str : String, showError = true ) : TType {
		if( StringTools.startsWith(str, "#") ) {
			var id = Std.parseInt(str.substr(1, str.length));
			if (id != null) {
				return getTypeById(id);
			}
		} else {
			var t = resolveCache.get(str);
			if( t != null )
				return t;
			for( i in resolveLastIndex...types.length ) {
				var t = types[i];
				var tstr = t.toString();
				if (tstr != null) {
					resolveCache.set(tstr, t);
					resolveLastIndex = i + 1;
					if( tstr == str )
						return t;
				}
			}
		}

		if( showError )
			Analyzer.log("Type not found '" + str + "'");
		return null;
	}

	public function locate( lt : TType, up = 0 ) : BlockStats {
		var ctx = new BlockStats(this);

		inline function isVirtualField(t) { t >>>= 24; return t == 1 || t == 2; }

		for( b in filteredBlocks )
			if( b.type != null && b.type.match(lt) ) {
				var tl = [];
				var owner = b.owner;
				// skip first virtual field
				if( lt.t != HDynObj && owner != null && owner.type != null && owner.type.t.match(HVirtual(_)) && isVirtualField(owner.makeTID(b,true)) )
					owner = owner.owner;

				if( owner != null ) {
					tl.push(owner.makeTID(b, Analyzer.displayFields == Full));
					var k : Int = up;
					while( k-- > 0 && owner.owner != all ) {
						if( owner.owner == null ) {
							tl.unshift(0);
							break;
						}
						var tag = owner.owner.makeTID(owner, Analyzer.displayFields != None);
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
									// do not add i+1 here to prevent infinit loop
									k += i;
								}
								break;
							}
						// don't display virtual wrappers
						if( Analyzer.displayFields != None && owner.type != null && isVirtualField(tag) && owner.type.t.match(HVirtual(_)) ) {
							tag = -1;
							k++;
						}
						if( tag != -1 )
							tl.unshift(tag);
					}
				}
				ctx.addPath(tl, b.size);
			}

		return ctx;
	}

	public function count( lt : TType, excludes : Array<String> ) : BlockStats {
		var ctx = new BlockStats(this);
		var texclude = [];
		for( e in excludes ) {
			var t = resolveType(e);
			if( t == null ) return ctx;
			texclude.push(t);
		}
		Block.MARK_UID++;
		var mark = [];
		for( b in filteredBlocks )
			if( b.type == lt )
				visitRec(b,ctx,[],mark);
		while( mark.length > 0 ) {
			var b = mark.pop();
			for( s in b.subs )
				visitRec(s.b,ctx,texclude,mark);
		}
		return ctx;
	}

	function visitRec( b : Block, ctx : BlockStats, exclude : Array<TType>, mark : Array<Block> ) {
		if( b.mark == Block.MARK_UID ) return;
		b.mark = Block.MARK_UID;
		if( b.type != null ) for( t in exclude ) if( b.type.match(t) ) return;
		ctx.addPath(b.type == null ? [] : [b.type.tid],b.size);
		if( b.subs != null )
			mark.push(b);
	}

	public function parents( lt : TType, up = 0 ) : BlockStats {
		var ctx = new BlockStats(this);
		for( b in filteredBlocks )
			if( b.type == lt )
				for( p in b.getParents() ) {
					ctx.addPath([p.makeTID(b, Analyzer.displayFields != None)], p.size);
				}
		return ctx;
	}

	public function subs( lt : TType, down = 0 ) {
		var ctx = new BlockStats(this);
		var mark = new Map();
		for( b in filteredBlocks )
			if( b.type == lt ) {
				function addRec(tl:Array<Int>, b:Block, k:Int) {
					if( k < 0 ) return;
					if( mark.exists(b) )
						return;
					mark.set(b, true);
					tl.push(b.type == null ? 0 : b.type.tid);
					ctx.addPath(tl, b.size);
					if( b.subs != null ) {
							k--;
						for( s in b.subs )
							addRec(tl.copy(), s.b, k);
					}
				}
				if( b.subs != null ) {
					for( s in b.subs )
						addRec([], s.b, down);
				}
			}
		return ctx;
	}

	// ----- Tools with side-effect -----

	public function removeParent( lt : TType, pt : TType ) {
		for( b in filteredBlocks )
			if( b.type == lt )
				for( p in b.getParents() )
					if( p.type == pt )
						b.removeParent(p);
	}

	// ----- Filter -----

	public function buildFilteredBlocks() {
		memStats = null;
		switch( filterMode ) {
		case None:
			filteredBlocks = blocks.copy();
		case _ if (otherMems.length == 0):
			filteredBlocks = blocks.copy();
		default:
			filteredBlocks = [];
			var progress = 0;
			for( b in blocks ) {
				progress++;
				Analyzer.logProgress(progress, blocks.length);
				if( !isBlockIgnored(b) )
					filteredBlocks.push(b);
			}
		}
	}

	function isBlockIgnored( b : Block ) {
		switch( filterMode ) {
		case None:
			return false;
		case Intersect:
			for( m2 in otherMems ) {
				var b2 = m2.pointerBlock.get(b.addr);
				if( b2 == null || b2.typePtr != b.typePtr || b2.size != b.size )
					return true;
			}
		case Unique:
			for( m2 in otherMems ) {
				var b2 = m2.pointerBlock.get(b.addr);
				if( b2 != null && b2.typePtr == b.typePtr && b2.size == b.size )
					return true;
			}
		}
		return false;
	}

}

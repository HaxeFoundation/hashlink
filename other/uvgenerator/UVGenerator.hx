import haxe.Exception;
import sys.io.File;
import eval.luv.File.FileSync;
import haxe.io.Path;
import haxe.PosInfos;
import eval.luv.Dir.DirSync;

using StringTools;

typedef TypeAndName = {
	type:String,
	name:String
}

typedef FunctionSignature = {
	var name:String;
	var returnType:String;
	var arguments:Array<TypeAndName>;
}

typedef StructSignature = {
	var type:String;
	var name:String;
	var fields:Array<TypeAndName>;
}

class Skip extends Exception {}

class UVGenerator {
	static inline var DOCS = 'include/libuv/docs/src';
	static inline var UV_GENERATED_C = 'libs/uv/uv_generated.c';
	static inline var UV_HX = 'other/uvgenerator/UV.hx';
	static inline var UV_HX_HEADER = 'other/uvgenerator/UV.hx.header';

	static final skipDocs = ['api', 'dll', 'guide', 'index', 'migration_010_100',
		'poll', 'threading', 'threadpool', 'upgrading'];
	static final skipFunctions = ['uv_replace_allocator', 'uv_get_osfhandle',
		'uv_fileno', 'uv_open_osfhandle', 'uv_print_all_handles', 'uv_print_active_handles',
		'uv_os_environ', 'uv_os_free_environ', 'uv_setup_args', 'uv_get_process_title',
		'uv_set_process_title', 'uv_tcp_open', 'uv_udp_open', 'uv_socketpair', 'uv_buf_init',
		'uv_inet_ntop', 'uv_inet_pton', 'uv_loadavg', 'uv_interface_addressses',
		'uv_loop_configure']; // TODO: don't skip uv_loop_configure
	static final skipStructs = ['uv_process_options_t', 'uv_utsname_t', 'uv_env_item_t', 'uv_dir_t',
		'uv_stdio_container_t'];
	static final skipStructFields = ['f_spare[4]', 'phys_addr[6]'];
	static final allowNoCallback = ['uv_fs_cb', 'uv_random_cb'];

	static final predefinedHxTypes = new Map<String,String>();
	static final hxTypesToGenerate = new Map<String,String>();

	static function main() {
		var root = rootDir();
		var docsDir = Path.join([root, DOCS]);
		var scan = DirSync.scan(docsDir).resolve();
		var cFile = File.write(Path.join([root, UV_GENERATED_C]));
		var hxFile = File.write(Path.join([root, UV_HX]));

		var hxHeader = File.getContent(Path.join([root, UV_HX_HEADER]));
		collectPredefinedHxTypesToMap(hxHeader, predefinedHxTypes);
		hxFile.writeString(hxHeader);

		var entry = null;
		while(null != (entry = scan.next())) {
			if(entry.kind != FILE || '.rst' != entry.name.sub(entry.name.length - 4))
				continue;

			if(skipDocs.contains(entry.name.sub(0, entry.name.length - 4).toString()))
				continue;

			var path = Path.join([docsDir, entry.name.toString()]);
			Sys.println('Generating ' + entry.name.sub(0, entry.name.length - 4).toString() + '...');

			var lines = File.getContent(path).split('\n');
			var totalLines = lines.length;
			while(lines.length > 0) {
				var line = lines.shift().trim();
				var lineNumber = totalLines - lines.length;
				try {
					if(line.startsWith('.. c:function:: ')) {
						var sig = parseSignature(line.substr('.. c:function:: '.length).trim());
						var cStr = needsCbWrapper(sig) ? cFnWrapperWithCb(sig) : cFnBinding(sig);
						cFile.writeString(cStr);
						hxFile.writeString(hxBinding(sig));
					} else if(line.startsWith('typedef struct ') && line.endsWith('{')) {
						for(sig in parseStruct(line, lines)) {
							if(sig.name == null || skipStructs.contains(sig.name))
								continue;
							cFile.writeString(cStructBinding(sig));
							hxFile.writeString(hxStructBinding(sig));
						}
					}
				} catch(e:Skip) {
					continue;
				} catch(e) {
					Sys.stderr().writeString('Error on symbol at line: $lineNumber\n${e.details()}\n');
					Sys.exit(1);
				}
			}
		}
		hxFile.writeString('}\n\n');

		hxFile.writeString(generateHXTypes(hxTypesToGenerate));

		cFile.close();
		hxFile.close();
		scan.end();
	}

	static function collectPredefinedHxTypesToMap(hxCode:String, map:Map<String,String>) {
		var re = ~/(abstract|typedef|class)\s+([^ (<]+)/;
		var pos = 0;
		while(re.matchSub(hxCode, pos)) {
			map.set(re.matched(2), re.matched(1));
			var p = re.matchedPos();
			pos = p.pos + p.len;
		}
	}

	static function generateHXTypes(types:Map<String,String>):String {
		var lines = [];
		for(hx => c in types) {
			lines.push('abstract $hx(Abstract<"$c">) {}');
		}
		return lines.join('\n');
	}

	static function needsCbWrapper(sig:FunctionSignature):Bool {
		for(a in sig.arguments)
			if(a.type.endsWith('_cb')) {
				return true;
			}
		return false;
	}

	static function rootDir(?p:PosInfos):String {
		var generatorPath = FileSync.realPath(p.fileName).resolve().toString();
		return new Path(new Path(new Path(generatorPath).dir).dir).dir;
	}

	static function parseStructType(firstLine:String):Null<String> {
		var start = firstLine.indexOf('struct ');
		if(start < 0)
			return null;
		start += 'struct '.length;
		var end = firstLine.indexOf(' ', start);
		if(end < 0)
			return null;
		var name = firstLine.substring(start, end);
		if(name.endsWith('_s'))
			name = name.substr(0, name.length - 2) + '_t';
		return name;
	}

	static function parseStruct(firstLine:String, lines:Array<String>):Array<StructSignature> {
		var result = [];
		var fields = [];
		var type = parseStructType(firstLine);
		var name = null;
		while(lines.length > 0) {
			var line = lines.shift();
			//strip comments
			var commentPos = line.indexOf('//');
			if(commentPos >= 0)
				line = line.substr(0, commentPos);
			commentPos = line.indexOf('/*');
			if(commentPos >= 0)
				line = line.substr(0, commentPos) + line.substr(line.indexOf('*/') + 2);
			line = line.trim();
			//leave unions for manual handling
			if(line.startsWith('union {')) {
				parseStruct(line, lines);
				continue;
			}
			if(line.startsWith('struct ') && line.endsWith('{')) {
				var sub = parseStruct(line, lines);
				if(sub.length > 0) {
					result = result.concat(sub);
					fields.push({name:sub[0].name, type:sub[0].type});
					sub[0].name = sub[0].type;
				}
			} else {
				var splitPos = line.lastIndexOf(' ');
				if(line.endsWith(';'))
					line = line.substr(0, line.length - 1);
				if(line.startsWith('}')) {
					name = line.substr(splitPos + 1);
					break;
				} else {
					fields.push(parseTypeAndName(line));
				}
			}
		}
		result.unshift({type:type, name:name, fields:fields});
		return result;
	}

	static function parseSignature(str:String):FunctionSignature {
		str = str.substr(0, str.length - (str.endsWith(';') ? 2 : 1));
		var parts = str.split('(');
		var returnAndName = parseTypeAndName(parts[0]);
		var args = parts[1].split(',').map(StringTools.trim);
		if(skipFunctions.contains(returnAndName.name))
			throw new Skip('${returnAndName.name} is in the skipFunctions list');
		return {
			name: returnAndName.name,
			returnType: returnAndName.type,
			arguments: args.map(parseTypeAndName)
		}
	}

	static function parseTypeAndName(str:String):TypeAndName {
		var namePos = str.lastIndexOf(' ') + 1;
		if(namePos <= 0)
			return {
				type: str,
				name: ''
			}
		else
			return {
				type: str.substring(0, namePos - 1),
				name: str.substr(namePos)
			}
	}

	static function mapHLType(type:String):String {
		return switch type {
			case 'int': '_I32';
			case 'int32_t': '_I32';
			case 'int32_t*': '_REF(_I32)';
			case 'int64_t': '_I64';
			case 'uint64_t': '_U64';
			case 'long': '_I64';
			case 'long*': '_REF(_I64)';
			case 'long long': '_I64';
			case 'char*': '_BYTES';
			case 'void*': '_POINTER';
			case 'void': '_VOID';
			case 'unsigned int': '_U32';
			case 'ssize_t': '_I64';
			case 'size_t*': '_REF(_U64)';
			case 'size_t': '_U64';
			case 'int*': '_REF(_I32)';
			case 'double*': '_REF(_F64)';
			case 'double': '_F64';
			case 'FILE*': '_FILE';
			case 'uv_pid_t': '_I32';
			case 'uv_buf_t': '_BUF';
			case 'uv_dirent_type_t': '_I32';
			case 'cpu_times*': '_CPU_TIMES';
			case _ if(type.endsWith('**')):
				'_REF(' + mapHLType(type.substr(0, type.length - 1)) + ')';
			case _ if(type.startsWith('uv_')):
				type = type.substr(3);
				if(type.endsWith('_t*'))
					type = type.substring(0, type.length - 3);
				'_' + type.toUpperCase();
			case _ if(type.startsWith('struct ') && type.endsWith("*")):
				'_' + type.substring('struct '.length, type.length - 1).toUpperCase();
			case _ if(type.startsWith('const ')):
				mapHLType(type.substr('const '.length));
			case _:
				throw 'Unknown type: "$type"';
		}
	}

	static function isUvBuf(cType:String):Bool {
		return cType == 'uv_buf_t' || cType == 'const uv_buf_t';
	}

	static function mapHLArg(a:TypeAndName):String {
		var type = mapHLType(a.type);
		if(a.name.endsWith(']')) {
			type = isUvBuf(a.type) ? '${type}_ARRAY' : '_REF($type)';
		}
		return type;
	}

	static final reCapitalize = ~/_[a-z]/g;

	static function snakeToPascalCase(str:String):String {
		return str.charAt(0).toUpperCase() + reCapitalize.map(str.substr(1), r -> r.matched(0).replace('_', '').toUpperCase());
	}

	static function mapHXType(type:String):String {
		if(type.startsWith('const '))
			type = type.substr('const '.length);

		var isRef = type.endsWith('*');
		if(isRef)
			type = type.substr(0, type.length - 1);

		inline function handleRef(t:String)
			return isRef ? 'Ref<$t>' : t;

		return switch type {
			case 'void': isRef ? 'Pointer' : 'Void';
			case 'char': isRef ? 'Bytes' : 'Int';
			case 'int': handleRef('Int');
			case 'long': handleRef('I64');
			case 'long long': handleRef('I64');
			case 'double': handleRef('Float');
			case 'int32_t': handleRef('Int');
			case 'int64_t': handleRef('I64');
			case 'uint64_t': handleRef('U64');
			case 'size_t': handleRef('U64');
			case 'ssize_t': handleRef('I64');
			case 'uv_dirent_type_t': 'Int';
			case _ if(type.startsWith('unsigned ')):
				handleRef('U' + mapHXType(type.substr('unsigned '.length)));
			case _ if(type.endsWith('*')):
				handleRef(mapHXType(type));
			case _:
				if(type.startsWith('struct '))
					type = type.substr('struct '.length);
				var hxType = snakeToPascalCase(type);
				var finalName = (type.startsWith('uv_') ? hxType : 'C$hxType') + (isRef ? 'Star' : '');
				if(!predefinedHxTypes.exists(finalName))
					hxTypesToGenerate.set(finalName, type + (isRef ? '_star' : ''));
				finalName;
		}
	}

	static function functionName(name:String):String {
		return if(name.startsWith('uv_'))
			name.substr('uv_'.length)
		else
			throw 'Function name is expected to start with "uv_": "$name"';
	}

	static function functionNameWithCb(name:String):String {
		return '${functionName(name)}_with_cb';
	}

	static function callbackName(type:String):String {
		return 'on_${type}';
	}

	static function hxBinding(sig:FunctionSignature):String {
		function compose(name:String, args:Array<String>) {
			var args = args.join(', ');
			var ret = mapHXType(sig.returnType);
			return '\tstatic public function $name($args):$ret;\n';
		}
		function mapArg(a:TypeAndName):String {
			if(a.name.endsWith(']')) {
				var openPos = a.name.lastIndexOf('[');
				var hxType = mapHXType(a.type);
				hxType = isUvBuf(a.type) ? hxType + 'Arr' : 'Ref<$hxType>';
				return '${a.name.substring(0, openPos)}:$hxType';
			} else {
				var type = mapHXType(a.type);
				var name = a.name == '' ? type.toLowerCase() : a.name;
				return '${name}:${type}';
			}
		}
		if(needsCbWrapper(sig)) {
			var fnName = functionNameWithCb(sig.name);
			var args = sig.arguments
				.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
				.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? 'use_${a.type}:Bool' : mapArg(a));
			return compose(fnName, args);
		} else {
			var fnName = functionName(sig.name);
			var args = sig.arguments.filter(a -> a.type != 'void').map(mapArg);
			return compose(fnName, args);
		}
	}

	static function cFnBinding(sig:FunctionSignature):String {
		var args = switch sig.arguments {
			case [{type:'void'}]: '_NO_ARG';
			case _: sig.arguments.map(mapHLArg).join(' ');
		}
		return 'DEFINE_PRIM(${mapHLType(sig.returnType)}, ${functionName(sig.name)}, $args);\n';
	}

	static function cFnWrapperWithCb(sig:FunctionSignature):String {
		var fnName = functionNameWithCb(sig.name);

		var args = sig.arguments
			.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
			.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? 'bool use_${a.type}' : '${a.type} ${a.name}')
			.join(', ');
		var lines = ['HL_PRIM ${sig.returnType} HL_NAME($fnName)( $args ) {'];

		var args = sig.arguments
			.map(a -> {
				var cbName = callbackName(a.type);
				if(a.type.endsWith('_cb'))
					allowNoCallback.contains(a.type) ? 'use_${a.type}?$cbName:NULL' : cbName
				else
					if(a.name.endsWith(']')) {
						var openPos = a.name.lastIndexOf('[');
						a.name.substring(0, openPos);
					} else
						a.name;
			})
			.join(', ');
		var ret = sig.returnType == 'void' ? '' : 'return ';
		lines.push('	$ret${sig.name}($args);');
		lines.push('}');

		var args = sig.arguments
			.filter(a -> !a.type.endsWith('_cb') || allowNoCallback.contains(a.type))
			.map(a -> a.type.endsWith('_cb') && allowNoCallback.contains(a.type) ? '_BOOL' : mapHLArg(a))
			.join(' ');
		lines.push('DEFINE_PRIM(${mapHLType(sig.returnType)}, $fnName, $args);');

		return lines.join('\n') + '\n';
	}

	static function detectStruct(name:String):{name:String, definePrim:String} {
		if(name.startsWith('uv_') && name.endsWith('_t')) {
			return {
				name: name.substring('uv_'.length, name.length - '_t'.length),
				definePrim: 'DEFINE_PRIM_UV_FIELD'
			}
		} else {
			return {
				name: name,
				definePrim: 'DEFINE_PRIM_C_FIELD'
			}
		}
	}

	static function cStructBinding(sig:StructSignature):String {
		var s = detectStruct(sig.name);
		var cStruct = sig.name + '*';
		var hlStruct = mapHLType(cStruct);
		var result = sig.fields
			.filter(f -> !skipStructFields.contains(f.name))
			.map(f -> {
				if(structFieldNeedsRef(f.type)) {
					var hlType = mapHLType(f.type + '*');
					'${s.definePrim}_REF($hlType, ${f.type} *, $hlStruct, ${s.name}, ${f.name});';
				} else {
					'${s.definePrim}(${mapHLType(f.type)}, ${f.type}, $hlStruct, ${s.name}, ${f.name});';
				}
			});
		result.push('DEFINE_PRIM_ALLOC($hlStruct, ${s.name});');
		return result.join('\n') + '\n';
	}

	static function hxStructBinding(sig:StructSignature):String {
		var name = detectStruct(sig.name).name;
		var cStruct = sig.name + '*';
		var hxStruct = mapHXType(cStruct);
		var result = sig.fields
			.filter(f -> !skipStructFields.contains(f.name))
			.map(f -> {
				var cType = f.type;
				if(structFieldNeedsRef(f.type))
					cType += '*';
				'\tstatic public function ${name}_${f.name}($name:$hxStruct):${mapHXType(cType)};';
			});
		result.push('\tstatic public function alloc_$name():$hxStruct;');
		return result.join('\n') + '\n';
	}

	static function structFieldNeedsRef(fieldType:String):Bool {
		return !fieldType.endsWith('*') && !fieldType.startsWith('unsigned') && switch fieldType {
			case 'int' | 'bool' | 'double' | 'int64_t' | 'uint64_t' | 'size_t'
				| 'ssize_t' | 'long' | 'long long' | 'int32_t' | 'uv_dirent_type_t' : false;
			case _: true;
		}
	}
}


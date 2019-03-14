package mesa;

abstract Uniform(Null<Int>) {
}

abstract Program(Null<Int>) {
}

abstract Shader(Null<Int>) {
}

abstract Texture(Null<Int>) {
}

abstract Buffer(Null<Int>) {
}

abstract Framebuffer(Null<Int>) {
}

abstract Renderbuffer(Null<Int>) {
}

abstract Query(Null<Int>) {
}

abstract VertexArray(Null<Int>) {
}

@:hlNative("mesa","gl_")
class GL {

	public static function init() : Bool {
		return false;
	}

	// non standard
	public static function getConfigParameter( v : Int ) : Int {
		return 0;
	}

	public static function isContextLost() : Bool {
		return false;
	}

	public static function clear( bits : Int ) {
	}

	public static function getError() : Int {
		return 0;
	}

	public static function scissor( x : Int, y : Int, width : Int, height : Int ) {
	}

	public static function clearColor( r : Float, g : Float, b : Float, a : Float) {
	}

	public static function clearDepth( value : Float ) {
	}

	public static function clearStencil( value : Int ) {
	}

	public static function viewport( x : Int, y : Int, width : Int, height : Int ) {
	}

	public static function finish() {
	}

	public static function pixelStorei( key : Int, value : Int ) {
	}

	public static function getParameter( name : Int ) : Dynamic {
		switch( name ){
			case VENDOR, VERSION, RENDERER, SHADING_LANGUAGE_VERSION:
				return @:privateAccess String.fromUTF8(getString(name));
			case _:
				throw "Not implemented";
				return null;
		}
	}

	static function getString( name : Int ) : hl.Bytes {
		return null;
	}

	// state changes

	public static function polygonMode( face : Int, mode : Int ) {
	}

	public static function enable( feature : Int ) {
	}

	public static function disable( feature : Int ) {
	}

	public static function cullFace( face : Int ) {
	}

	public static function blendFunc( src : Int, dst : Int ) {
	}

	public static function blendFuncSeparate( src : Int, dst : Int, alphaSrc : Int, alphaDst : Int ) {
	}

	public static function blendEquation( op : Int ) {
	}

	public static function blendEquationSeparate( op : Int, alphaOp : Int ) {
	}

	public static function depthMask( mask : Bool ) {
	}

	public static function depthFunc( f : Int ) {
	}

	public static function colorMask( r : Bool, g : Bool, b : Bool, a : Bool ) {
	}

	public static function stencilMaskSeparate( face : Int, mask : Int ){
	}

	public static function stencilFuncSeparate( face : Int, func : Int, ref : Int, mask : Int ){
	}

	public static function stencilOpSeparate( face : Int, sfail : Int, dpfail : Int, dppas : Int ){
	}

	// program

	public static function createProgram() : Program {
		return null;
	}

	public static function bindFragDataLocation( p : Program, colorNumber : Int, name : String ) : Void {
	}

	public static function attachShader( p : Program, s : Shader ) {
	}

	public static function linkProgram( p : Program ) {
	}

	public static function getProgramParameter( p : Program, param : Int ) : Dynamic {
		return null;
	}

	public static function getProgramInfoLog( p : Program ) : String {
		return @:privateAccess String.fromUTF8(getProgramInfoBytes(p));
	}

	static function getProgramInfoBytes( p : Program ) : hl.Bytes {
		return null;
	}

	public static function getUniformLocation( p : Program, name : String ) : Uniform {
		return null;
	}

	public static function getAttribLocation( p : Program, name : String ) : Int {
		return -1;
	}

	public static function useProgram( p : Program ) {
	}


	// shader

	public static function createShader( type : Int ) : Shader {
		return null;
	}

	public static function shaderSource( s : Shader, src : String ) {
	}

	public static function compileShader( s : Shader ) {
	}

	public static function getShaderInfoLog( s : Shader ) : String {
		return @:privateAccess String.fromUTF8(getShaderInfoBytes(s));
	}

	static function getShaderInfoBytes( s : Shader ) : hl.Bytes {
		return null;
	}

	public static function getShaderParameter( s : Shader, param : Int ) : Dynamic {
		return null;
	}

	public static function deleteShader( s : Shader ) {
	}

	// texture

	public static function createTexture() : Texture {
		return null;
	}

	public static function activeTexture( t : Int ) {
	}

	public static function bindTexture( t : Int, texture : Texture ) {
	}

	public static function texParameteri( t : Int, key : Int, value : Int ) {
	}

	public static function texParameterf( t : Int, key : Int, value : hl.F32 ) {
	}

	@:hlNative("mesa","gl_tex_image2d")
	public static function texImage2D( target : Int, level : Int, internalFormat : Int, width : Int, height : Int, border : Int, format : Int, type : Int, image : hl.Bytes ) {
	}

	@:hlNative("mesa","gl_tex_image3d")
	public static function texImage3D( target : Int, level : Int, internalFormat : Int, width : Int, height : Int, depth : Int, border : Int, format : Int, type : Int, image : hl.Bytes ) {
	}

	@:hlNative("mesa","gl_tex_image2d_multisample")
	public static function texImage2DMultisample( target : Int, internalFormat : Int, samples : Int, width : Int, height : Int, fixedsamplelocations : Bool ) {
	}

	@:hlNative("mesa","gl_compressed_tex_image2d")
	public static function compressedTexImage2D( target : Int, level : Int, internalFormat : Int, width : Int, height : Int, border : Int, imageSize : Int, image : hl.Bytes ) {
	}

	@:hlNative("mesa","gl_compressed_tex_image3d")
	public static function compressedTexImage3D( target : Int, level : Int, internalFormat : Int, width : Int, height : Int, depth : Int, border : Int, imageSize : Int, image : hl.Bytes ) {
	}

	public static function generateMipmap( t : Int ) {
	}

	public static function deleteTexture( t : Texture ) {
	}

	// framebuffer

	public static function blitFramebuffer( src_x0 : Int, src_y0 : Int, src_x1 : Int, src_y1 : Int, dst_x0 : Int, dst_y0 : Int, dst_x1 : Int, dst_y1 : Int, mask : Int, filter : Int ) {
	}

	public static function createFramebuffer() : Framebuffer {
		return null;
	}

	public static function bindFramebuffer( target : Int, f : Framebuffer ) {
	}

	@:hlNative("mesa","gl_framebuffer_texture2d")
	public static function framebufferTexture2D( target : Int, attach : Int, texTarget : Int, t : Texture, level : Int ) {
	}

	public static function framebufferTextureLayer( target : Int, attach : Int, t : Texture, level : Int, layer : Int ) {
	}

	public static function framebufferTexture( target : Int, attach : Int, t : Texture, level : Int ) {
	}

	public static function deleteFramebuffer( f : Framebuffer ) {
	}

	public static function readBuffer( mode : Int ) {
	}

	public static function readPixels( x : Int, y : Int, width : Int, height : Int, format : Int, type : Int, data : hl.Bytes ) {
	}

	public static function drawBuffers( n : Int, buffers : hl.Bytes ) {
	}

	// renderbuffer

	public static function createRenderbuffer() : Renderbuffer {
		return null;
	}

	public static function bindRenderbuffer( target : Int, r : Renderbuffer ) {
	}

	public static function renderbufferStorage( target : Int, format : Int, width : Int, height : Int ) {
	}

	public static function renderbufferStorageMultisample( target : Int, samples : Int, format : Int, width : Int, height : Int ) {
	}

	public static function framebufferRenderbuffer( frameTarget : Int, attach : Int, renderTarget : Int, b : Renderbuffer ) {
	}

	public static function deleteRenderbuffer( b : Renderbuffer ) {
	}

	// buffer

	public static function createBuffer() : Buffer {
		return null;
	}

	public static function bindBufferBase( target : Int, index : Int, buffer : Buffer ) {
	}

	public static function bindBuffer( target : Int, b : Buffer ) {
	}

	public static function bufferDataSize( target : Int, size : Int, param : Int ) {
	}

	public static function bufferData( target : Int, size : Int, data : hl.Bytes, param : Int ) {
	}

	public static function bufferSubData( target : Int, offset : Int, data : hl.Bytes, srcOffset : Int, srcLength : Int ) {
	}

	public static function enableVertexAttribArray( attrib : Int ) {
	}

	public static function disableVertexAttribArray( attrib : Int ) {
	}

	public static function vertexAttribPointer( index : Int, size : Int, type : Int, normalized : Bool, stride : Int, position : Int ) {
	}

	public static function vertexAttribIPointer( index : Int, size : Int, type : Int, stride : Int, position : Int ) {
	}

	public static function vertexAttribDivisor( index : Int, divisor : Int ) {
	}

	public static function deleteBuffer( b : Buffer ) {
	}

	// uniforms

	public static function uniform1i( u : Uniform, i : Int ) {
	}

	public static function uniform4fv( u : Uniform, buffer : hl.Bytes, bufPos : Int, count : Int ) {
	}

	public static function uniformMatrix4fv( u : Uniform, transpose : Bool, buffer : hl.Bytes, bufPos : Int, count : Int ) {
	}

	// compute

	public static function dispatchCompute( num_groups_x : Int, num_groups_y : Int, num_groups_z : Int ) {
	}

	public static function memoryBarrier( barrier : Int ) {
	}

	// draw

	public static function drawElements( mode : Int, count : Int, type : Int, start : Int ) {
	}

	public static function drawElementsInstanced( mode : Int, count : Int, type : Int, start : Int, primcount : Int ) {
	}

	public static function drawArrays( mode : Int, start : Int, count : Int ) {
	}

	public static function drawArraysInstanced( mode : Int, start : Int, count : Int, primcount : Int ) {
	}

	public static function multiDrawElementsIndirect( mode : Int, type : Int, data : hl.Bytes, count : Int, stride : Int ) {
	}

	// queries

	public static function createQuery() : Query {
		return null;
	}

	public static function deleteQuery( q : Query ) {
	}

	public static function beginQuery( target : Int, q : Query ) {
	}

	public static function endQuery( target : Int ) {
	}

	public static function queryResultAvailable( q : Query ) {
		return false;
	}

	public static function queryResult( q : Query ) : Float {
		return 0.;
	}

	public static function queryCounter( q : Query, target : Int ) {
	}

	// vertexarray
	public static function createVertexArray() : VertexArray {
		return null;
	}

	public static function bindVertexArray( a : VertexArray ) : Void {
	}

	public static function deleteVertexArray( a : VertexArray ) : Void {
	}

	// uniform buffer

	public static function getUniformBlockIndex( p : Program, name : String ) : Int {
		return 0;
	}

	public static function uniformBlockBinding( p : Program, blockIndex : Int, blockBinding : Int ) : Void {
	}

	// ----- CONSTANTS -----

	/* ClearBufferMask */
	public static inline var DEPTH_BUFFER_BIT               = 0x00000100;
	public static inline var STENCIL_BUFFER_BIT             = 0x00000400;
	public static inline var COLOR_BUFFER_BIT               = 0x00004000;

	/* BeginMode */
	public static inline var POINTS                         = 0x0000;
	public static inline var LINES                          = 0x0001;
	public static inline var LINE_LOOP                      = 0x0002;
	public static inline var LINE_STRIP                     = 0x0003;
	public static inline var TRIANGLES                      = 0x0004;
	public static inline var TRIANGLE_STRIP                 = 0x0005;
	public static inline var TRIANGLE_FAN                   = 0x0006;

	/* AlphaFunction(not supported in ES20) */
	/*      NEVER */
	/*      LESS */
	/*      EQUAL */
	/*      LEQUAL */
	/*      GREATER */
	/*      NOTEQUAL */
	/*      GEQUAL */
	/*      ALWAYS */
	/* BlendingFactorDest */
	public static inline var ZERO                           = 0;
	public static inline var ONE                            = 1;
	public static inline var SRC_COLOR                      = 0x0300;
	public static inline var ONE_MINUS_SRC_COLOR            = 0x0301;
	public static inline var SRC_ALPHA                      = 0x0302;
	public static inline var ONE_MINUS_SRC_ALPHA            = 0x0303;
	public static inline var DST_ALPHA                      = 0x0304;
	public static inline var ONE_MINUS_DST_ALPHA            = 0x0305;

	/* BlendingFactorSrc */
	/*      ZERO */
	/*      ONE */
	public static inline var DST_COLOR                      = 0x0306;
	public static inline var ONE_MINUS_DST_COLOR            = 0x0307;
	public static inline var SRC_ALPHA_SATURATE             = 0x0308;
	/*      SRC_ALPHA */
	/*      ONE_MINUS_SRC_ALPHA */
	/*      DST_ALPHA */
	/*      ONE_MINUS_DST_ALPHA */
	/* BlendEquationSeparate */
	public static inline var FUNC_ADD                       = 0x8006;
	public static inline var FUNC_MIN                       = 0x8007;
    public static inline var FUNC_MAX                       = 0x8008;
	public static inline var BLEND_EQUATION                 = 0x8009;
	public static inline var BLEND_EQUATION_RGB             = 0x8009;   /* same as BLEND_EQUATION */
	public static inline var BLEND_EQUATION_ALPHA           = 0x883D;

	/* BlendSubtract */
	public static inline var FUNC_SUBTRACT                  = 0x800A;
	public static inline var FUNC_REVERSE_SUBTRACT          = 0x800B;

	/* Separate Blend Functions */
	public static inline var BLEND_DST_RGB                  = 0x80C8;
	public static inline var BLEND_SRC_RGB                  = 0x80C9;
	public static inline var BLEND_DST_ALPHA                = 0x80CA;
	public static inline var BLEND_SRC_ALPHA                = 0x80CB;
	public static inline var CONSTANT_COLOR                 = 0x8001;
	public static inline var ONE_MINUS_CONSTANT_COLOR       = 0x8002;
	public static inline var CONSTANT_ALPHA                 = 0x8003;
	public static inline var ONE_MINUS_CONSTANT_ALPHA       = 0x8004;
	public static inline var BLEND_COLOR                    = 0x8005;

	/* GLBuffer Objects */
	public static inline var ARRAY_BUFFER                   = 0x8892;
	public static inline var ELEMENT_ARRAY_BUFFER           = 0x8893;
	public static inline var ARRAY_BUFFER_BINDING           = 0x8894;
	public static inline var ELEMENT_ARRAY_BUFFER_BINDING   = 0x8895;
	public static inline var SHADER_STORAGE_BUFFER          = 0x90D2;
	public static inline var UNIFORM_BUFFER                 = 0x8A11;
	public static inline var QUERY_BUFFER                   = 0x9192;

	public static inline var STREAM_DRAW                    = 0x88E0;
	public static inline var STATIC_DRAW                    = 0x88E4;
	public static inline var DYNAMIC_DRAW                   = 0x88E8;

	public static inline var BUFFER_SIZE                    = 0x8764;
	public static inline var BUFFER_USAGE                   = 0x8765;

	public static inline var CURRENT_VERTEX_ATTRIB          = 0x8626;

	/* CullFaceMode */
	public static inline var FRONT                          = 0x0404;
	public static inline var BACK                           = 0x0405;
	public static inline var FRONT_AND_BACK                 = 0x0408;

	/* PolygonMode */
	public static inline var POINT                          = 0x1B00;
	public static inline var LINE                           = 0x1B01;
	public static inline var FILL                           = 0x1B02;

	/* DepthFunction */
	/*      NEVER */
	/*      LESS */
	/*      EQUAL */
	/*      LEQUAL */
	/*      GREATER */
	/*      NOTEQUAL */
	/*      GEQUAL */
	/*      ALWAYS */
	/* EnableCap */
	/* TEXTURE_2D */
	public static inline var CULL_FACE                      = 0x0B44;
	public static inline var BLEND                          = 0x0BE2;
	public static inline var DITHER                         = 0x0BD0;
	public static inline var STENCIL_TEST                   = 0x0B90;
	public static inline var DEPTH_TEST                     = 0x0B71;
	public static inline var SCISSOR_TEST                   = 0x0C11;
	public static inline var POLYGON_OFFSET_FILL            = 0x8037;
	public static inline var SAMPLE_ALPHA_TO_COVERAGE       = 0x809E;
	public static inline var SAMPLE_COVERAGE                = 0x80A0;
	public static inline var MULTISAMPLE                    = 0x809D;
	public static inline var DEPTH_CLAMP                    = 0x864F;

	/* ErrorCode */
	public static inline var NO_ERROR                       = 0;
	public static inline var INVALID_ENUM                   = 0x0500;
	public static inline var INVALID_VALUE                  = 0x0501;
	public static inline var INVALID_OPERATION              = 0x0502;
	public static inline var OUT_OF_MEMORY                  = 0x0505;

	/* FrontFaceDirection */
	public static inline var CW                             = 0x0900;
	public static inline var CCW                            = 0x0901;

	/* GetPName */
	public static inline var LINE_WIDTH                     = 0x0B21;
	public static inline var ALIASED_POINT_SIZE_RANGE       = 0x846D;
	public static inline var ALIASED_LINE_WIDTH_RANGE       = 0x846E;
	public static inline var CULL_FACE_MODE                 = 0x0B45;
	public static inline var FRONT_FACE                     = 0x0B46;
	public static inline var DEPTH_RANGE                    = 0x0B70;
	public static inline var DEPTH_WRITEMASK                = 0x0B72;
	public static inline var DEPTH_CLEAR_VALUE              = 0x0B73;
	public static inline var DEPTH_FUNC                     = 0x0B74;
	public static inline var STENCIL_CLEAR_VALUE            = 0x0B91;
	public static inline var STENCIL_FUNC                   = 0x0B92;
	public static inline var STENCIL_FAIL                   = 0x0B94;
	public static inline var STENCIL_PASS_DEPTH_FAIL        = 0x0B95;
	public static inline var STENCIL_PASS_DEPTH_PASS        = 0x0B96;
	public static inline var STENCIL_REF                    = 0x0B97;
	public static inline var STENCIL_VALUE_MASK             = 0x0B93;
	public static inline var STENCIL_WRITEMASK              = 0x0B98;
	public static inline var STENCIL_BACK_FUNC              = 0x8800;
	public static inline var STENCIL_BACK_FAIL              = 0x8801;
	public static inline var STENCIL_BACK_PASS_DEPTH_FAIL   = 0x8802;
	public static inline var STENCIL_BACK_PASS_DEPTH_PASS   = 0x8803;
	public static inline var STENCIL_BACK_REF               = 0x8CA3;
	public static inline var STENCIL_BACK_VALUE_MASK        = 0x8CA4;
	public static inline var STENCIL_BACK_WRITEMASK         = 0x8CA5;
	public static inline var VIEWPORT                       = 0x0BA2;
	public static inline var SCISSOR_BOX                    = 0x0C10;
	/*      SCISSOR_TEST */
	public static inline var COLOR_CLEAR_VALUE              = 0x0C22;
	public static inline var COLOR_WRITEMASK                = 0x0C23;
	public static inline var UNPACK_ALIGNMENT               = 0x0CF5;
	public static inline var PACK_ALIGNMENT                 = 0x0D05;
	public static inline var MAX_TEXTURE_SIZE               = 0x0D33;
	public static inline var MAX_VIEWPORT_DIMS              = 0x0D3A;
	public static inline var SUBPIXEL_BITS                  = 0x0D50;
	public static inline var RED_BITS                       = 0x0D52;
	public static inline var GREEN_BITS                     = 0x0D53;
	public static inline var BLUE_BITS                      = 0x0D54;
	public static inline var ALPHA_BITS                     = 0x0D55;
	public static inline var DEPTH_BITS                     = 0x0D56;
	public static inline var STENCIL_BITS                   = 0x0D57;
	public static inline var POLYGON_OFFSET_UNITS           = 0x2A00;
	/*      POLYGON_OFFSET_FILL */
	public static inline var POLYGON_OFFSET_FACTOR          = 0x8038;
	public static inline var TEXTURE_BINDING_2D             = 0x8069;
	public static inline var SAMPLE_BUFFERS                 = 0x80A8;
	public static inline var SAMPLES                        = 0x80A9;
	public static inline var SAMPLE_COVERAGE_VALUE          = 0x80AA;
	public static inline var SAMPLE_COVERAGE_INVERT         = 0x80AB;

	/* GetTextureParameter */
	/*      TEXTURE_MAG_FILTER */
	/*      TEXTURE_MIN_FILTER */
	/*      TEXTURE_WRAP_S */
	/*      TEXTURE_WRAP_T */
	public static inline var COMPRESSED_TEXTURE_FORMATS     = 0x86A3;

	/* HintMode */
	public static inline var DONT_CARE                      = 0x1100;
	public static inline var FASTEST                        = 0x1101;
	public static inline var NICEST                         = 0x1102;

	/* HintTarget */
	public static inline var GENERATE_MIPMAP_HINT            = 0x8192;

	/* DataType */
	public static inline var BYTE                           = 0x1400;
	public static inline var UNSIGNED_BYTE                  = 0x1401;
	public static inline var SHORT                          = 0x1402;
	public static inline var UNSIGNED_SHORT                 = 0x1403;
	public static inline var INT                            = 0x1404;
	public static inline var UNSIGNED_INT                   = 0x1405;
	public static inline var FLOAT                          = 0x1406;

	public static inline var HALF_FLOAT                     = 0x140B;

	/* PixelFormat */
	public static inline var DEPTH_COMPONENT                = 0x1902;
	public static inline var RED                            = 0x1903;
	public static inline var GREEN                          = 0x1904;
	public static inline var BLUE                           = 0x1905;
	public static inline var ALPHA                          = 0x1906;
	public static inline var RG                             = 0x8227;
	public static inline var RGB                            = 0x1907;
	public static inline var RGBA                           = 0x1908;
	public static inline var LUMINANCE                      = 0x1909;
	public static inline var LUMINANCE_ALPHA                = 0x190A;

	public static inline var BGRA                           = 0x80E1;
	public static inline var RGBA8                          = 0x8058;
	public static inline var RGB10_A2                       = 0x8059;

	public static inline var RG16                           = 0x822C;
	public static inline var RG16UI                         = 0x823A;
	public static inline var RG16F                          = 0x822F;
	public static inline var RG32F                          = 0x8230;
	public static inline var R8								= 0x8229;
	public static inline var RG8							= 0x822B;
	public static inline var R16F							= 0x822D;
	public static inline var R32F							= 0x822E;
	public static inline var UNSIGNED_INT_2_10_10_10_REV	= 0x8368;
	public static inline var UNSIGNED_INT_10F_11F_11F_REV	= 0x8C3B;

	/* PixelType */
	/*      UNSIGNED_BYTE */
	public static inline var UNSIGNED_SHORT_4_4_4_4         = 0x8033;
	public static inline var UNSIGNED_SHORT_5_5_5_1         = 0x8034;
	public static inline var UNSIGNED_SHORT_5_6_5           = 0x8363;

	public static inline var SRGB                           = 0x8C40;
	public static inline var SRGB8                          = 0x8C41;
	public static inline var SRGB_ALPHA                     = 0x8C42;
	public static inline var SRGB8_ALPHA                    = 0x8C43;
	public static inline var FRAMEBUFFER_SRGB               = 0x8DB9;

	public static inline var RGBA32F                        = 0x8814;
	public static inline var RGB32F                         = 0x8815;
	public static inline var RGBA16F                        = 0x881A;
	public static inline var RGB16F                         = 0x881B;
	public static inline var R11F_G11F_B10F                 = 0x8C3A;
	public static inline var ALPHA16F                       = 0x881C;
	public static inline var ALPHA32F                       = 0x8816;

	/* Shaders */
	public static inline var FRAGMENT_SHADER                  = 0x8B30;
	public static inline var VERTEX_SHADER                    = 0x8B31;
	public static inline var GEOMETRY_SHADER                  = 0x8DD9;
	public static inline var COMPUTE_SHADER                   = 0x91B9;
	public static inline var MAX_VERTEX_ATTRIBS               = 0x8869;
	public static inline var MAX_VERTEX_UNIFORM_VECTORS       = 0x8DFB;
	public static inline var MAX_VARYING_VECTORS              = 0x8DFC;
	public static inline var MAX_COMBINED_TEXTURE_IMAGE_UNITS = 0x8B4D;
	public static inline var MAX_VERTEX_TEXTURE_IMAGE_UNITS   = 0x8B4C;
	public static inline var MAX_TEXTURE_IMAGE_UNITS          = 0x8872;
	public static inline var MAX_FRAGMENT_UNIFORM_VECTORS     = 0x8DFD;
	public static inline var SHADER_TYPE                      = 0x8B4F;
	public static inline var DELETE_STATUS                    = 0x8B80;
	public static inline var LINK_STATUS                      = 0x8B82;
	public static inline var VALIDATE_STATUS                  = 0x8B83;
	public static inline var ATTACHED_SHADERS                 = 0x8B85;
	public static inline var ACTIVE_UNIFORMS                  = 0x8B86;
	public static inline var ACTIVE_ATTRIBUTES                = 0x8B89;
	public static inline var SHADING_LANGUAGE_VERSION         = 0x8B8C;
	public static inline var CURRENT_PROGRAM                  = 0x8B8D;

	/* StencilFunction */
	public static inline var NEVER                          = 0x0200;
	public static inline var LESS                           = 0x0201;
	public static inline var EQUAL                          = 0x0202;
	public static inline var LEQUAL                         = 0x0203;
	public static inline var GREATER                        = 0x0204;
	public static inline var NOTEQUAL                       = 0x0205;
	public static inline var GEQUAL                         = 0x0206;
	public static inline var ALWAYS                         = 0x0207;

	/* StencilOp */
	/*      ZERO */
	public static inline var KEEP                           = 0x1E00;
	public static inline var REPLACE                        = 0x1E01;
	public static inline var INCR                           = 0x1E02;
	public static inline var DECR                           = 0x1E03;
	public static inline var INVERT                         = 0x150A;
	public static inline var INCR_WRAP                      = 0x8507;
	public static inline var DECR_WRAP                      = 0x8508;

	/* StringName */
	public static inline var VENDOR                         = 0x1F00;
	public static inline var RENDERER                       = 0x1F01;
	public static inline var VERSION                        = 0x1F02;

	/* TextureMagFilter */
	public static inline var NEAREST                        = 0x2600;
	public static inline var LINEAR                         = 0x2601;

	/* TextureMinFilter */
	/*      NEAREST */
	/*      LINEAR */
	public static inline var NEAREST_MIPMAP_NEAREST         = 0x2700;
	public static inline var LINEAR_MIPMAP_NEAREST          = 0x2701;
	public static inline var NEAREST_MIPMAP_LINEAR          = 0x2702;
	public static inline var LINEAR_MIPMAP_LINEAR           = 0x2703;

	/* TextureParameterName */
	public static inline var TEXTURE_MAG_FILTER             = 0x2800;
	public static inline var TEXTURE_MIN_FILTER             = 0x2801;
	public static inline var TEXTURE_WRAP_R                 = 0x8072;
	public static inline var TEXTURE_WRAP_S                 = 0x2802;
	public static inline var TEXTURE_WRAP_T                 = 0x2803;
	public static inline var TEXTURE_LOD_BIAS               = 0x8501;
	public static inline var TEXTURE_MAX_ANISOTROPY         = 0x84FE;
	public static inline var TEXTURE_COMPARE_MODE           = 0x884C;
	public static inline var TEXTURE_COMPARE_FUNC           = 0x884D;
	public static inline var COMPARE_REF_TO_TEXTURE         = 0x884E;

	/* TextureTarget */
	public static inline var TEXTURE_2D                     = 0x0DE1;
	public static inline var TEXTURE_2D_MULTISAMPLE         = 0x9100;
	public static inline var TEXTURE_3D                     = 0x806F;
	public static inline var TEXTURE                        = 0x1702;
	public static inline var TEXTURE_2D_ARRAY				= 0x8C1A;

	public static inline var TEXTURE_CUBE_MAP_SEAMLESS      = 0x884F;
	public static inline var TEXTURE_CUBE_MAP               = 0x8513;
	public static inline var TEXTURE_BINDING_CUBE_MAP       = 0x8514;
	public static inline var TEXTURE_CUBE_MAP_POSITIVE_X    = 0x8515;
	public static inline var TEXTURE_CUBE_MAP_NEGATIVE_X    = 0x8516;
	public static inline var TEXTURE_CUBE_MAP_POSITIVE_Y    = 0x8517;
	public static inline var TEXTURE_CUBE_MAP_NEGATIVE_Y    = 0x8518;
	public static inline var TEXTURE_CUBE_MAP_POSITIVE_Z    = 0x8519;
	public static inline var TEXTURE_CUBE_MAP_NEGATIVE_Z    = 0x851A;
	public static inline var MAX_CUBE_MAP_TEXTURE_SIZE      = 0x851C;

	/* TextureUnit */
	public static inline var TEXTURE0                       = 0x84C0;
	public static inline var TEXTURE1                       = 0x84C1;
	public static inline var TEXTURE2                       = 0x84C2;
	public static inline var TEXTURE3                       = 0x84C3;
	public static inline var TEXTURE4                       = 0x84C4;
	public static inline var TEXTURE5                       = 0x84C5;
	public static inline var TEXTURE6                       = 0x84C6;
	public static inline var TEXTURE7                       = 0x84C7;
	public static inline var TEXTURE8                       = 0x84C8;
	public static inline var TEXTURE9                       = 0x84C9;
	public static inline var TEXTURE10                      = 0x84CA;
	public static inline var TEXTURE11                      = 0x84CB;
	public static inline var TEXTURE12                      = 0x84CC;
	public static inline var TEXTURE13                      = 0x84CD;
	public static inline var TEXTURE14                      = 0x84CE;
	public static inline var TEXTURE15                      = 0x84CF;
	public static inline var TEXTURE16                      = 0x84D0;
	public static inline var TEXTURE17                      = 0x84D1;
	public static inline var TEXTURE18                      = 0x84D2;
	public static inline var TEXTURE19                      = 0x84D3;
	public static inline var TEXTURE20                      = 0x84D4;
	public static inline var TEXTURE21                      = 0x84D5;
	public static inline var TEXTURE22                      = 0x84D6;
	public static inline var TEXTURE23                      = 0x84D7;
	public static inline var TEXTURE24                      = 0x84D8;
	public static inline var TEXTURE25                      = 0x84D9;
	public static inline var TEXTURE26                      = 0x84DA;
	public static inline var TEXTURE27                      = 0x84DB;
	public static inline var TEXTURE28                      = 0x84DC;
	public static inline var TEXTURE29                      = 0x84DD;
	public static inline var TEXTURE30                      = 0x84DE;
	public static inline var TEXTURE31                      = 0x84DF;
	public static inline var ACTIVE_TEXTURE                 = 0x84E0;

	/* TextureWrapMode */
	public static inline var REPEAT                         = 0x2901;
	public static inline var CLAMP_TO_EDGE                  = 0x812F;
	public static inline var MIRRORED_REPEAT                = 0x8370;

	/* Uniform Types */
	public static inline var FLOAT_VEC2                     = 0x8B50;
	public static inline var FLOAT_VEC3                     = 0x8B51;
	public static inline var FLOAT_VEC4                     = 0x8B52;
	public static inline var INT_VEC2                       = 0x8B53;
	public static inline var INT_VEC3                       = 0x8B54;
	public static inline var INT_VEC4                       = 0x8B55;
	public static inline var BOOL                           = 0x8B56;
	public static inline var BOOL_VEC2                      = 0x8B57;
	public static inline var BOOL_VEC3                      = 0x8B58;
	public static inline var BOOL_VEC4                      = 0x8B59;
	public static inline var FLOAT_MAT2                     = 0x8B5A;
	public static inline var FLOAT_MAT3                     = 0x8B5B;
	public static inline var FLOAT_MAT4                     = 0x8B5C;
	public static inline var SAMPLER_2D                     = 0x8B5E;
	public static inline var SAMPLER_CUBE                   = 0x8B60;

	/* Vertex Arrays */
	public static inline var VERTEX_ATTRIB_ARRAY_ENABLED        = 0x8622;
	public static inline var VERTEX_ATTRIB_ARRAY_SIZE           = 0x8623;
	public static inline var VERTEX_ATTRIB_ARRAY_STRIDE         = 0x8624;
	public static inline var VERTEX_ATTRIB_ARRAY_TYPE           = 0x8625;
	public static inline var VERTEX_ATTRIB_ARRAY_NORMALIZED     = 0x886A;
	public static inline var VERTEX_ATTRIB_ARRAY_POINTER        = 0x8645;
	public static inline var VERTEX_ATTRIB_ARRAY_BUFFER_BINDING = 0x889F;

	/* Point Size */
	public static inline var VERTEX_PROGRAM_POINT_SIZE       = 0x8642;
	public static inline var POINT_SPRITE                    = 0x8861;

	/* GLShader Source */
	public static inline var COMPILE_STATUS                 = 0x8B81;

	/* GLShader Precision-Specified Types */
	public static inline var LOW_FLOAT                      = 0x8DF0;
	public static inline var MEDIUM_FLOAT                   = 0x8DF1;
	public static inline var HIGH_FLOAT                     = 0x8DF2;
	public static inline var LOW_INT                        = 0x8DF3;
	public static inline var MEDIUM_INT                     = 0x8DF4;
	public static inline var HIGH_INT                       = 0x8DF5;

	/* GLFramebuffer Object. */
	public static inline var FRAMEBUFFER                    = 0x8D40;
	public static inline var RENDERBUFFER                   = 0x8D41;
	public static inline var READ_FRAMEBUFFER               = 0x8CA8;
	public static inline var DRAW_FRAMEBUFFER               = 0x8CA9;
	public static inline var DRAW_INDIRECT_BUFFER			= 0x8F3F;

	public static inline var RGBA4                          = 0x8056;
	public static inline var RGB5_A1                        = 0x8057;
	public static inline var RGB565                         = 0x8D62;
	public static inline var DEPTH_COMPONENT16              = 0x81A5;
	public static inline var DEPTH_COMPONENT24              = 0x81A6;
	public static inline var STENCIL_INDEX                  = 0x1901;
	public static inline var STENCIL_INDEX8                 = 0x8D48;
	public static inline var DEPTH_STENCIL                  = 0x84F9;

	public static inline var RENDERBUFFER_WIDTH             = 0x8D42;
	public static inline var RENDERBUFFER_HEIGHT            = 0x8D43;
	public static inline var RENDERBUFFER_INTERNAL_FORMAT   = 0x8D44;
	public static inline var RENDERBUFFER_RED_SIZE          = 0x8D50;
	public static inline var RENDERBUFFER_GREEN_SIZE        = 0x8D51;
	public static inline var RENDERBUFFER_BLUE_SIZE         = 0x8D52;
	public static inline var RENDERBUFFER_ALPHA_SIZE        = 0x8D53;
	public static inline var RENDERBUFFER_DEPTH_SIZE        = 0x8D54;
	public static inline var RENDERBUFFER_STENCIL_SIZE      = 0x8D55;

	public static inline var FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE           = 0x8CD0;
	public static inline var FRAMEBUFFER_ATTACHMENT_OBJECT_NAME           = 0x8CD1;
	public static inline var FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL         = 0x8CD2;
	public static inline var FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE = 0x8CD3;

	public static inline var COLOR_ATTACHMENT0              = 0x8CE0;
	public static inline var DEPTH_ATTACHMENT               = 0x8D00;
	public static inline var STENCIL_ATTACHMENT             = 0x8D20;
	public static inline var DEPTH_STENCIL_ATTACHMENT       = 0x821A;

	public static inline var NONE                           = 0;

	public static inline var FRAMEBUFFER_COMPLETE                      = 0x8CD5;
	public static inline var FRAMEBUFFER_INCOMPLETE_ATTACHMENT         = 0x8CD6;
	public static inline var FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT = 0x8CD7;
	public static inline var FRAMEBUFFER_INCOMPLETE_DIMENSIONS         = 0x8CD9;
	public static inline var FRAMEBUFFER_UNSUPPORTED                   = 0x8CDD;

	public static inline var FRAMEBUFFER_BINDING            = 0x8CA6;
	public static inline var RENDERBUFFER_BINDING           = 0x8CA7;
	public static inline var MAX_RENDERBUFFER_SIZE          = 0x84E8;

	public static inline var INVALID_FRAMEBUFFER_OPERATION  = 0x0506;

	/* WebGL-specific enums */
	public static inline var UNPACK_FLIP_Y_WEBGL            = 0x9240;
	public static inline var UNPACK_PREMULTIPLY_ALPHA_WEBGL = 0x9241;
	public static inline var CONTEXT_LOST_WEBGL             = 0x9242;
	public static inline var UNPACK_COLORSPACE_CONVERSION_WEBGL = 0x9243;
	public static inline var BROWSER_DEFAULT_WEBGL          = 0x9244;

	/* Queries */
	public static inline var SAMPLES_PASSED                 = 0x8914;
	public static inline var TIMESTAMP                      = 0x8E28;

	/* Barriers */
	public static inline var VERTEX_ATTRIB_ARRAY_BARRIER_BIT = 0x00000001;
	public static inline var ELEMENT_ARRAY_BARRIER_BIT       = 0x00000002;
	public static inline var UNIFORM_BARRIER_BIT             = 0x00000004;
	public static inline var TEXTURE_FETCH_BARRIER_BIT       = 0x00000008;
	public static inline var SHADER_IMAGE_ACCESS_BARRIER_BIT = 0x00000020;
	public static inline var COMMAND_BARRIER_BIT             = 0x00000040;
	public static inline var PIXEL_BUFFER_BARRIER_BIT        = 0x00000080;
	public static inline var TEXTURE_UPDATE_BARRIER_BIT      = 0x00000100;
	public static inline var BUFFER_UPDATE_BARRIER_BIT       = 0x00000200;
	public static inline var FRAMEBUFFER_BARRIER_BIT         = 0x00000400;
	public static inline var TRANSFORM_FEEDBACK_BARRIER_BIT  = 0x00000800;
	public static inline var ATOMIC_COUNTER_BARRIER_BIT      = 0x00001000;
	public static inline var SHADER_STORAGE_BARRIER_BIT      = 0x00002000;
	public static inline var QUERY_BUFFER_BARRIER_BIT        = 0x00008000;
	public static inline var ALL_BARRIER_BITS                = 0xFFFFFFFF;

}
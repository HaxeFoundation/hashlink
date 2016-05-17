package sdl;

/*
@:cppFileCode('
static void OnSDLAudio( void * userdata, Uint8 * stream, int len ) {
	int base = 0;
	hx::SetTopOfStack(&base,true);
	((sdl::SoundChannel_obj*)userdata)->onSample(FLOATPTR(stream), len>>2);
	hx::SetTopOfStack((int*)0,true);
}
')
*/
class SoundChannel {

	public var bufferSamples(default, null) : Int;
//	@cpp var root : PTR<Object>;
	var device : Int;

	public function new( bufferSamples : Int ) {
		this.bufferSamples = bufferSamples;
/*		var error : String = null;
		untyped __cpp__('
			SDL_AudioSpec want, have;
			SDL_zero(want);
			root = this;
			hx::GCAddRoot(&root);
			want.freq = 44100;
			want.format = AUDIO_F32SYS;
			want.channels = 2;
			want.samples = bufferSamples;
			want.callback = OnSDLAudio;
			want.userdata = this;
			device = SDL_OpenAudioDevice(NULL, 0, & want, NULL, 0);
			if( device != 0 )
				SDL_PauseAudioDevice(device, 0);
			else
				hx::GCRemoveRoot(&root);
		');
*/	}

	public function stop() {
		if( device != 0 ) {
/*			untyped __cpp__('
				SDL_CloseAudioDevice(device);
				hx::GCRemoveRoot(&root);
			');
*/			device = 0;
		}
	}

}
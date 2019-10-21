namespace Mixer {

static float t = 0;
void audio_callback(void* userdata, u8* stream, int len) {
    f32 *s = (f32*) stream;
    for (int i = 0; i < len / sizeof(f32); i++) {
        t += (sin(t / 1000) * 100 + 100) * PI * 2.0 / 48000.0;
        s[i] = cos(t);
    }
}

bool init() {
    audio_mixer.arena = Util::request_arena();

    SDL_AudioSpec want = {};
    want.freq = audio_mixer.freq;
    want.format = AUDIO_F32;
    want.samples = 2048;
    want.channels = 2;
    want.callback = audio_callback;

    // Let SDL handle the translation for us.
    SDL_AudioSpec have;
    SDL_AudioDeviceID dev;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev <= 0) {
        ERR("%s", SDL_GetError());
        return false;
    }
    
    SDL_PauseAudioDevice(dev, 0);
    return true;
}

};  // namespace Mixer

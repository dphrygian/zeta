#include "core.h"
#include "audio.h"
#include "soloudaudiosystem.h"

IAudioSystem* CreateSoLoudAudioSystem()
{
	return new SoLoudAudioSystem;
}

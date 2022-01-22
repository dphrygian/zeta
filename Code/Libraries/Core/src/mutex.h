#ifndef MUTEX_H
#define MUTEX_H

#if BUILD_WINDOWS_NO_SDL
#include <Windows.h>
#endif

#if BUILD_SDL
#include "SDL2/SDL.h"
#endif

class Mutex
{
public:
	Mutex();
	~Mutex();

	void Wait();
	void Release();

private:
#if BUILD_WINDOWS_NO_SDL
	HANDLE		m_pMutex;
#endif
#if BUILD_SDL
	SDL_mutex*	m_pMutex;
#endif
};

#endif // MUTEX_H

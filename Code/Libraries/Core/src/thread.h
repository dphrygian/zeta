#ifndef THREAD_H
#define THREAD_H

#if BUILD_WINDOWS_NO_SDL
#include <Windows.h>
typedef DWORD ( WINAPI *ThreadFunc )( void* );
#endif

#if BUILD_SDL
#include "SDL2/SDL.h"
typedef int ( *ThreadFunc )( void* );
#endif

class Thread
{
public:
	Thread( ThreadFunc Function, void* Parameter );
	~Thread();

	void	Wait();

private:
#if BUILD_WINDOWS_NO_SDL
	HANDLE		m_pThread;
#endif
#if BUILD_SDL
	SDL_Thread*	m_pThread;
#endif
};

#endif

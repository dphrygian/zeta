#include "core.h"
#include "mutex.h"

Mutex::Mutex()
:	m_pMutex( NULL )
{
#if BUILD_WINDOWS_NO_SDL
	m_pMutex = CreateMutex(
		NULL,	// Attributes
		FALSE,	// Initial owner
		NULL	// Name
		);
#endif
#if BUILD_SDL
	m_pMutex = SDL_CreateMutex();
#endif
}

Mutex::~Mutex()
{
#if BUILD_WINDOWS_NO_SDL
	CloseHandle( m_pMutex );
#endif
#if BUILD_SDL
	SDL_DestroyMutex( m_pMutex );
#endif
}

void Mutex::Wait()
{
#if BUILD_WINDOWS_NO_SDL
	WaitForSingleObject( m_pMutex, INFINITE );
#endif
#if BUILD_SDL
	SDL_LockMutex( m_pMutex );
#endif
}

void Mutex::Release()
{
#if BUILD_WINDOWS_NO_SDL
	ReleaseMutex( m_pMutex );
#endif
#if BUILD_SDL
	SDL_UnlockMutex( m_pMutex );
#endif
}

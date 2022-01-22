#include "core.h"
#include "thread.h"

Thread::Thread( ThreadFunc Function, void* Parameter )
:	m_pThread( NULL )
{
#if BUILD_WINDOWS_NO_SDL
	m_pThread = CreateThread(
		NULL,		// Security attributes--left NULL for default
		0,			// Stack size--left 0 for default
		Function,	// Function
		Parameter,	// Parameter to function
		0,			// Flags--could use CREATE_SUSPENDED and run with ResumeThread
		NULL		// ThreadID (out DWORD, not sure where it's used)
		);
#endif
#if BUILD_SDL
	m_pThread = SDL_CreateThread(
		Function,	// Function
		"Thread",	// Name
		Parameter	// Data
		);
#endif
}

Thread::~Thread()
{
	// No need to ExitThread, it happens automatically when the thread function returns
}

void Thread::Wait()
{
#if BUILD_WINDOWS_NO_SDL
	WaitForSingleObject( m_pThread, INFINITE );
#endif
#if BUILD_SDL
	SDL_WaitThread( m_pThread, NULL );
#endif
}

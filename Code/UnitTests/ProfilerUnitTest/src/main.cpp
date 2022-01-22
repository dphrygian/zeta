#include "core.h"
#include "filestream.h"
#include "printmanager.h"
#include "allocator.h"

#include <Windows.h>
#include <crtdbg.h>
#include <stdio.h>

int Fibonnacci( int i );
int Factorial( int i );
void AllocateAndFree1( int i );
void AllocateAndFree2( int i );

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	Unused( hInstance );
	Unused( hPrevInstance );
	Unused( lpCmdLine );
	Unused( nCmdShow );

	Allocator::GetDefault().Initialize( 65536 );

	Fibonnacci( 5 );
	Factorial( 5 );
	AllocateAndFree1( 500 );
	AllocateAndFree2( 500 );

#if DO_PROFILING
	Profiler::DeleteInstance();
#endif

	PrintManager::DeleteInstance();

	Allocator::GetDefault().Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Write ) );
	Allocator::GetDefault().ShutDown();

	DEBUGASSERT( _CrtCheckMemory() );
	DEBUGASSERT( !_CrtDumpMemoryLeaks() );

	return 0;
}

int Fibonnacci( int i )
{
	PROFILE_FUNCTION;

	if( i < 0 )
	{
		return -1;
	}
	if( i == 0 || i == 1 )
	{
		return i;
	}
	return Fibonnacci( i - 1 ) + Fibonnacci( i - 2 );
}

int Factorial( int i )
{
	PROFILE_FUNCTION;

	if( i <= 1 )
	{
		return 1;
	}
	return i * Factorial( i - 1 );
}

void AllocateAndFree1( int i )
{
	byte* pMem = NULL;

	PROFILE_BEGIN( AllocateAndFree );
	PROFILE_BEGIN( Allocate );
	pMem = new byte[i];
	PROFILE_END;
	PROFILE_BEGIN( Free );
	delete[] pMem;
	PROFILE_END;
	PROFILE_END;
}

void AllocateAndFree2( int i )
{
	byte* pMem = NULL;

	PROFILE_SCOPE( AllocateAndFree );

	{
		PROFILE_SCOPE( Allocate );
		pMem = new byte[i];
	}

	{
		PROFILE_SCOPE( Free );
		delete[] pMem;
	}
}
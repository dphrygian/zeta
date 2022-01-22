#include "core.h"
#include "filestream.h"
#include "fileutil.h"

#include <stdarg.h>

/*static*/ Array<SimpleString> FileStream::sm_ResPaths;

/*static*/ void	FileStream::StaticAddResPath( const SimpleString& ResPath )
{
	sm_ResPaths.PushBackUnique( FileUtil::NormalizeDirectoryPath( ResPath.CStr() ) );
}

/*static*/ void FileStream::StaticShutDown()
{
	sm_ResPaths.Clear();
}

// NOTE: If any ResPath is a child of the working directory or another ResPath,
// files will be listed multiple times, relative to each path. Clients should
// handle this by treating them like separate, redundant files.
/*static*/ void	FileStream::StaticGetFilesInFolder( const SimpleString& Path, const bool Recursive, const SimpleString& Extension, Array<SimpleString>& OutFiles )
{
	FileUtil::GetFilesInFolder( Path, Recursive, Extension, OutFiles );

	FOR_EACH_ARRAY( ResPathIter, sm_ResPaths, SimpleString )
	{
		const SimpleString NewPath = ResPathIter.GetValue() + Path;
		FileUtil::GetFilesInFolder( NewPath, Recursive, Extension, OutFiles );
	}
}

FileStream::FileStream()
:	m_TheFile( NULL )
,	m_FileMode( EFM_None )
,	m_QualifiedFilename()
{
}

FileStream::FileStream( const char* FileName, EFileMode FileMode )
:	m_TheFile( NULL )
,	m_FileMode( FileMode )
,	m_QualifiedFilename()
{
	const char* Mode = NULL;
	if(			FileMode == EFM_Read )		{ Mode = "rb"; }
	else if(	FileMode == EFM_Write )		{ Mode = "wb"; }
	else if(	FileMode == EFM_Append )	{ Mode = "ab"; }

	DEVASSERT( Mode != NULL );
	m_QualifiedFilename = FileName;
	FOPEN( m_TheFile, m_QualifiedFilename.CStr(), Mode );

	// Maintain a list of known mod folders or other resource paths.
	// If the file couldn't be opened from ./, try opening from those locations.
	// *Don't* change the working directory, just append the mod folder to FileName.
	if( !m_TheFile )
	{
		FOR_EACH_ARRAY( ResPathIter, sm_ResPaths, SimpleString )
		{
			m_QualifiedFilename = ResPathIter.GetValue() + FileName;
			FOPEN( m_TheFile, m_QualifiedFilename.CStr(), Mode );
			if( m_TheFile )
			{
				break;
			}
		}
	}

#if BUILD_DEV
	if( !m_TheFile )
	{
		PrintManager* const pPrintManager = PrintManager::GetInstance_NoAlloc();
		if( pPrintManager && FileName == pPrintManager->GetLogFilename() )
		{
			// Don't print and don't assert! That would cause endless recursion trying to write to a log file that can't be opened.
			return;
		}

		PRINTF( "Couldn't open file: %s\n", FileName );
		WARN;
	}
#endif
}

FileStream::~FileStream()
{
	fclose( m_TheFile );
}

int FileStream::Read( int NumBytes, void* Buffer ) const
{
	ASSERT( m_FileMode == EFM_Read );

	return ( int )fread( Buffer, NumBytes, 1, m_TheFile );
}

int FileStream::Write( int NumBytes, const void* Buffer ) const
{
	ASSERT( m_FileMode == EFM_Write || m_FileMode == EFM_Append );

	return ( int )fwrite( Buffer, NumBytes, 1, m_TheFile );
}

int FileStream::PrintF( const char* Str, ... ) const
{
	ASSERT( m_FileMode == EFM_Write || m_FileMode == EFM_Append );

	va_list	Args;
	int		Length	= 0;
	char*	Buffer	= NULL;
	int		RetVal	= 0;

	va_start( Args, Str );
	Length = VSPRINTF_COUNT( Str, Args ) + 1;
	va_end( Args );
	Buffer = new char[ Length ];	// TODO: Pool this instead of dynamically allocating
	va_start( Args, Str );
	VSPRINTF( Buffer, Length, Str, Args );
	va_end( Args );

	RetVal = Write( Length - 1, Buffer );

	SafeDelete( Buffer );
	return RetVal;
}

int FileStream::SetPos( int Position ) const
{
	return ( int )fseek( m_TheFile, Position, SEEK_SET );
}

int FileStream::GetPos() const
{
	return ftell( m_TheFile );
}

int FileStream::EOS() const
{
	return feof( m_TheFile );
}

int FileStream::Size() const
{
	int Pos = GetPos();
	fseek( m_TheFile, 0, SEEK_END );
	int RetVal = GetPos();
	SetPos( Pos );
	return RetVal;
}

#include "core.h"
#include "fileutil.h"

#include <stdio.h>
#include <string.h>

#if BUILD_WINDOWS
#include <direct.h>	// For directory functions
#include <shlobj.h>
#endif

#if BUILD_LINUX || BUILD_MAC
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif

// This unit is a mess, a mix of standard and Windows-only functions, many with special-case
// purposes and some that are unsafe to use in-game and are only ever meant to run in tools.

// Cross-platform verified.
// Match upper and lower case letters and slashes
bool Match( char c1, char c2 )
{
	XTRACE_FUNCTION;

	if( c1 == c2 )
	{
		return true;
	}
	if( ( c1 == '\\' && c2 == '/' ) ||
		( c1 == '/' && c2 == '\\' ) )
	{
		return true;
	}
	if( ( c1 >= 'A' && c1 <= 'Z' ) && ( c1 == ( c2 - 32 ) ) )
	{
		return true;
	}
	if( ( c1 >= 'a' && c1 <= 'z' ) && ( c1 == ( c2 + 32 ) ) )
	{
		return true;
	}
	return false;
}

// Cross-platform verified.
bool FileUtil::Exists( const char* Filename )
{
	XTRACE_FUNCTION;

	FILE* pFile = NULL;
	FOPEN( pFile, Filename, "rb" );
	if( pFile )
	{
		fclose( pFile );
		return true;
	}
	return false;
}

// Cross-platform verified.
bool FileUtil::PathExists( const char* Path )
{
	XTRACE_FUNCTION;

	SimpleString CurrentDir = GetWorkingDirectory();
#if BUILD_WINDOWS
	if( _chdir( Path ) )
#else
	if( chdir( Path ) )
#endif
	{
		return false;
	}
	else
	{
#if BUILD_WINDOWS
		_chdir( CurrentDir.CStr() );
#else
		chdir( CurrentDir.CStr() );
#endif
		return true;
	}
}

// Cross-platform verified.
uint FileUtil::Size( const char* Filename )
{
	XTRACE_FUNCTION;

	FILE* pFile = NULL;
	FOPEN( pFile, Filename, "rb" );
	if( pFile )
	{
		fseek( pFile, 0, SEEK_END );
		uint RetVal = ftell( pFile );
		fclose( pFile );
		return RetVal;
	}
	return 0;
}

// Cross-platform verified.
// Compare filenames, case-insensitive, / and \ equivalent
bool FileUtil::Compare( const char* Filename1, const char* Filename2 )
{
	XTRACE_FUNCTION;

	const char* Iter1 = Filename1;
	const char* Iter2 = Filename2;
	ASSERT( Iter1 );
	ASSERT( Iter2 );
	while( *Iter1 && *Iter2 )
	{
		if( !Match( *Iter1, *Iter2 ) )
		{
			return false;
		}
		++Iter1;
		++Iter2;
	}
	return true;
}

// Cross-platform verified.
SimpleString FileUtil::Normalize( const char* Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );

	SimpleString RetVal = Path;
	for( char* c = RetVal.MutableCStr(); *c; ++c )
	{
		if( *c == '\\' )
		{
			*c = '/';
		}
	}

	return RetVal;
}

SimpleString FileUtil::NormalizeDirectoryPath( const char* Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );

	SimpleString RetVal = Normalize( Path );

	if( RetVal != "" )	// Ignore empty strings
	{
		const bool EndsWithSlash = RetVal.GetChar( RetVal.Length() - 1 ) == '/';
		if( !EndsWithSlash )
		{
			RetVal += "/";
		}
	}

	return RetVal;
}

// Cross-platform verified.
void FileUtil::MakePath( const char* Path )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	_mkdir( Path );
#else
	mkdir( Path, 0777 );
#endif
}

// Cross-platform verified.
void FileUtil::RecursiveMakePath( const char* Path )
{
	XTRACE_FUNCTION;

	SimpleString Filepath = Path;
	SimpleString SplitPath;
	SimpleString Remainder;

	int DescendDepth = 0;
	while( FileUtil::SplitLeadingFolder( Filepath.CStr(), SplitPath, Remainder ) )
	{
		if( !FileUtil::PathExists( SplitPath.CStr() ) )
		{
			FileUtil::MakePath( SplitPath.CStr() );
		}
		FileUtil::ChangeWorkingDirectory( SplitPath.CStr() );
		Filepath = Remainder;
		DescendDepth++;
	}

	while( DescendDepth > 0 )
	{
		FileUtil::ChangeWorkingDirectory( ".." );
		--DescendDepth;
	}
}

// Cross-platform verified.
void FileUtil::RemovePath( const char* Path )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	_rmdir( Path );
#else
	rmdir( Path );
#endif
}

// Cross-platform verified.
SimpleString FileUtil::GetWorkingDirectory()
{
	XTRACE_FUNCTION;

	char WorkingDir[ 256 ];
#if BUILD_WINDOWS
	_getcwd( WorkingDir, 256 );
#else
	getcwd( WorkingDir, 256 );
#endif
	return SimpleString( WorkingDir );
}

// Cross-platform verified.
void FileUtil::ChangeWorkingDirectory( const char* Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );
#if BUILD_WINDOWS
	_chdir( Path );
#else
	chdir( Path );
#endif
}

// Mixing in some Windows calls with all the POSIX stuff in the
// rest of this. Dur. Based on http://www.codeguru.com/forum/showthread.php?t=239271
void FileUtil::RecursiveRemoveDirectory( const char* Path )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	HANDLE          FileHandle;
	SimpleString	FilePath;
	SimpleString	FilePattern = SimpleString( Path ) + "\\*.*";
	WIN32_FIND_DATA FileInformation;

	FileHandle = FindFirstFile( FilePattern.CStr(), &FileInformation );
	if( FileHandle != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( FileInformation.cFileName[0] != '.' )	// Don't recurse up tree
			{
				FilePath = SimpleString( Path ) + "\\" + FileInformation.cFileName;

				if( FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					// Delete subdirectory
					RecursiveRemoveDirectory( FilePath.CStr() );
				}
				else
				{
					// Set file attributes so we can delete
					if( SetFileAttributes( FilePath.CStr(), FILE_ATTRIBUTE_NORMAL ) )
					{
						DeleteFile( FilePath.CStr() );
					}
				}
			}
		}
		while( TRUE == FindNextFile( FileHandle, &FileInformation ) );

		FindClose( FileHandle );

		// Set directory attributes so we can delete
		if( SetFileAttributes( Path, FILE_ATTRIBUTE_NORMAL ) )
		{
			RemoveDirectory( Path );
		}
	}
#else
	// TODO PORT LATER (not currently used)
	Unused( Path );
	WARN;
#endif
}

// Cross-platform verified.
bool FileUtil::RemoveFile( const char* Path )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	return FALSE != DeleteFile( Path );
#else
	return 0 == unlink( Path );
#endif
}

bool FileUtil::Copy( const char* OldPath, const char* NewPath, bool FailIfExists )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	return FALSE != CopyFile( OldPath, NewPath, FailIfExists );
#else
	// TODO PORT LATER (not currently used; and file copy is not fundamentally supported by the POSIX standard file API)
	Unused( OldPath );
	Unused( NewPath );
	Unused( FailIfExists );
	WARN;
	return false;
#endif
}

bool FileUtil::Move( const char* OldPath, const char* NewPath )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	const DWORD MoveFileExFlags = MOVEFILE_REPLACE_EXISTING;
	return FALSE != MoveFileEx( OldPath, NewPath, MoveFileExFlags );
#else
	// TODO PORT: Verify this
	return 0 == rename( OldPath, NewPath );
#endif
}

void GetFilesInFolderInternal( const SimpleString& Path, const SimpleString& Prefix, const bool Recursive, const SimpleString& Extension, Array<SimpleString>& OutFiles )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	HANDLE				FileHandle;
	WIN32_FIND_DATA		FileInformation;
	const SimpleString	FilePattern = SimpleString::PrintF( "%s/*", Path.CStr() );

	FileHandle = FindFirstFile( FilePattern.CStr(), &FileInformation );
	if( FileHandle != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( FileInformation.cFileName[0] == '.' )	// Don't recurse up tree
			{
			    continue;
			}

            const SimpleString Filename		= FileInformation.cFileName;

            if( FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                if( Recursive )
                {
                    const SimpleString PathFilename		= SimpleString::PrintF( "%s/%s", Path.CStr(), Filename.CStr() );
                    const SimpleString RecursivePrefix	= SimpleString::PrintF( "%s%s/", Prefix.CStr(), Filename.CStr() );
                    GetFilesInFolderInternal( PathFilename, RecursivePrefix, Recursive, Extension, OutFiles );
                }
                else
                {
                    // Ignore
                }
            }
            else
            {
                if( Extension == "" || Filename.EndsWith( Extension ) )
                {
                    const SimpleString PrefixFilename = SimpleString::PrintF( "%s%s", Prefix.CStr(), Filename.CStr() );
                    OutFiles.PushBack( PrefixFilename );
                }
            }
		}
		while( TRUE == FindNextFile( FileHandle, &FileInformation ) );

		FindClose( FileHandle );
	}
#elif BUILD_LINUX || BUILD_MAC
	DIR*	pDirectory	= opendir( Path.CStr() );
	if( pDirectory )
	{
		dirent* pDirEntry = NULL;
		while( ( pDirEntry = readdir( pDirectory ) ) )
		{
			// Ignore .. so we don't recurse up tree. (Also ignores hidden folders, I guess.)
			if( pDirEntry->d_name[0] == '.' )	// Don't recurse up tree
			{
			    continue;
			}

			const SimpleString Filename = pDirEntry->d_name;
			if( pDirEntry->d_type == DT_DIR )
			{
				if( Recursive )
				{
					const SimpleString PathFilename		= SimpleString::PrintF( "%s/%s", Path.CStr(), Filename.CStr() );
					const SimpleString RecursivePrefix	= SimpleString::PrintF( "%s%s/", Prefix.CStr(), Filename.CStr() );
					GetFilesInFolderInternal( PathFilename, RecursivePrefix, Recursive, Extension, OutFiles );
				}
				else
				{
					// Ignore
				}
			}
			else
			{
				if( Extension == "" || Filename.EndsWith( Extension ) )
				{
					const SimpleString PrefixFilename = SimpleString::PrintF( "%s%s", Prefix.CStr(), Filename.CStr() );
					OutFiles.PushBack( PrefixFilename );
				}
			}
		}

		closedir( pDirectory );
	}
#else
#error GetFilesInFolderInternal not implemented on this platform.
#endif
}

void GetFoldersInFolderInternal( const SimpleString& Path, const SimpleString& Prefix, const bool Recursive, Array<SimpleString>& OutFolders )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	HANDLE				FileHandle;
	WIN32_FIND_DATA		FileInformation;
	const SimpleString	FilePattern = SimpleString::PrintF( "%s/*", Path.CStr() );

	FileHandle = FindFirstFile( FilePattern.CStr(), &FileInformation );
	if( FileHandle != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( FileInformation.cFileName[0] == '.' )	// Don't recurse up tree
			{
			    continue;
			}

            const SimpleString Filename		= FileInformation.cFileName;

            if( FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
				const SimpleString PrefixFilename = SimpleString::PrintF( "%s%s", Prefix.CStr(), Filename.CStr() );
				OutFolders.PushBack( PrefixFilename );

                if( Recursive )
                {
                    const SimpleString PathFilename		= SimpleString::PrintF( "%s/%s", Path.CStr(), Filename.CStr() );
                    const SimpleString RecursivePrefix	= SimpleString::PrintF( "%s%s/", Prefix.CStr(), Filename.CStr() );
                    GetFoldersInFolderInternal( PathFilename, RecursivePrefix, Recursive, OutFolders );
                }
            }
		}
		while( TRUE == FindNextFile( FileHandle, &FileInformation ) );

		FindClose( FileHandle );
	}
#elif BUILD_LINUX || BUILD_MAC
	DIR*	pDirectory	= opendir( Path.CStr() );
	if( pDirectory )
	{
		dirent* pDirEntry = NULL;
		while( ( pDirEntry = readdir( pDirectory ) ) )
		{
			// Ignore .. so we don't recurse up tree. (Also ignores hidden folders, I guess.)
			if( pDirEntry->d_name[0] == '.' )	// Don't recurse up tree
			{
			    continue;
			}

			const SimpleString Filename = pDirEntry->d_name;
			if( pDirEntry->d_type == DT_DIR )
			{
				const SimpleString PrefixFilename = SimpleString::PrintF( "%s%s", Prefix.CStr(), Filename.CStr() );
				OutFolders.PushBack( PrefixFilename );

				if( Recursive )
				{
					const SimpleString PathFilename		= SimpleString::PrintF( "%s/%s", Path.CStr(), Filename.CStr() );
					const SimpleString RecursivePrefix	= SimpleString::PrintF( "%s%s/", Prefix.CStr(), Filename.CStr() );
					GetFoldersInFolderInternal( PathFilename, RecursivePrefix, Recursive, OutFolders );
				}
			}
		}

		closedir( pDirectory );
	}
#else
#error GetFoldersInFolderInternal not implemented on this platform.
#endif
}

void FileUtil::GetFilesInFolder( const SimpleString& Path, const bool Recursive, const SimpleString& Extension, Array<SimpleString>& OutFiles )
{
	XTRACE_FUNCTION;

	const SimpleString NormalizedPath = NormalizeDirectoryPath( Path.CStr() );
	GetFilesInFolderInternal( NormalizedPath, NormalizedPath, Recursive, Extension, OutFiles );
}

void FileUtil::GetFoldersInFolder( const SimpleString& Path, const bool Recursive, Array<SimpleString>& OutFolders )
{
	XTRACE_FUNCTION;

	const SimpleString NormalizedPath = NormalizeDirectoryPath( Path.CStr() );
	GetFoldersInFolderInternal( NormalizedPath, NormalizedPath, Recursive, OutFolders );
}

// Cross-platform verified.
const char* FileUtil::StripLeadingUpFolders( const char* Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );
	while( *Path )
	{
		if( Match( *Path, '/' ) && Match( *( Path + 1 ), '/' ) )
		{
			Path += 2;
		}
		else if( Match( *Path, '.' ) && Match( *( Path + 1 ), '/' ) )
		{
			Path += 2;
		}
		else if( Match( *Path, '.' ) && Match( *( Path + 1 ), '.' ) && Match( *( Path + 2 ), '/' ) )
		{
			Path += 3;
		}
		else
		{
			return Path;
		}
	}
	return Path;
}

// Cross-platform verified.
const char* FileUtil::StripLeadingFolder( const char* Path, const char* Folder )
{
	XTRACE_FUNCTION;

	ASSERT( Path );
	ASSERT( Folder );
	const char* OriginalPath = Path;
	while( *Path && *Folder )
	{
		if( Match( *Path, *Folder ) )
		{
			++Path;
			++Folder;
		}
		else
		{
			return OriginalPath;	// Folder wasn't matched, don't modify
		}
	}
	if( Match( *Path, '/' ) )
	{
		return Path + 1;
	}
	else
	{
		return OriginalPath;
	}
}

// Cross-platform verified.
const char* FileUtil::StripLeadingFolders( const char* const Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );

	const char* PathIter		= Path;
	const char* LastPathChar	= NULL;
	while( *PathIter )
	{
		if( Match( *PathIter, '/' ) )
		{
			LastPathChar = PathIter;
		}
		++PathIter;
	}

	// LastPathChar is now at the position of the last path character
	if( LastPathChar )
	{
		return LastPathChar + 1;
	}
	else
	{
		return Path;	// No path characters, just return the original string
	}
}

// Cross-platform verified.
SimpleString FileUtil::StripExtension( const char* Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );

	uint Length = (uint)strlen( Path ) + 1;
	char* PathCopy = new char[ Length ];
	strcpy_s( PathCopy, Length, Path );

	char* PathCopyIter = PathCopy;
	char* LastDot = NULL;
	while( *PathCopyIter )
	{
		if( Match( *PathCopyIter, '.' ) )
		{
			LastDot = PathCopyIter;
		}
		++PathCopyIter;
	}

	// LastDot is now at the position of the last dot
	if( LastDot )
	{
		*LastDot = '\0';
	}

	SimpleString RetVal( PathCopy );
	delete PathCopy;
	return RetVal;
}

// Cross-platform verified.
SimpleString FileUtil::StripExtensions( const char* Path )
{
	XTRACE_FUNCTION;

	ASSERT( Path );

	uint Length = (uint)strlen( Path ) + 1;
	char* PathCopy = new char[ Length ];
	strcpy_s( PathCopy, Length, Path );

	char* PathCopyIter = PathCopy;
	char* FirstDot = NULL;
	while( *PathCopyIter && !FirstDot )
	{
		if( Match( *PathCopyIter, '.' ) )
		{
			FirstDot = PathCopyIter;
		}
		++PathCopyIter;
	}

	// FirstDot is now at the position of the first dot
	if( FirstDot )
	{
		*FirstDot = '\0';
	}

	SimpleString RetVal( PathCopy );
	delete PathCopy;
	return RetVal;
}

// Cross-platform verified.
// This probably isn't super robust, but will work fine for a basic path.
bool FileUtil::SplitLeadingFolder( const char* Path, SimpleString& OutFolder, SimpleString& OutRemainder )
{
	XTRACE_FUNCTION;

	ASSERT( Path );

	SimpleString PathCopy( Path );
	char* FolderPath = PathCopy.MutableCStr();
	char* IterStr = PathCopy.MutableCStr();

	while( *IterStr )
	{
		if( Match( *IterStr, '/' ) )
		{
			// Split here
			*IterStr = '\0';
			OutFolder = FolderPath;
			OutRemainder = IterStr + 1;
			return true;
		}
		++IterStr;
	}

	return false;
}

// Not super robust, but good enough for my needs at the moment (in FontGenerator)
bool FileUtil::SplitLeadingPath( const char* Filename, SimpleString& OutPath, SimpleString& OutFilename )
{
	XTRACE_FUNCTION;

	ASSERT( Filename );

	SimpleString FilenameCopy( Filename );
	char* PathStart	= FilenameCopy.MutableCStr();
	char* IterStr	= FilenameCopy.MutableCStr() + FilenameCopy.Length();

	// Iterate backward to find the first slash and split there
	while( IterStr > PathStart )
	{
		if( Match( *IterStr, '/' ) )
		{
			// Split here
			*IterStr	= '\0';
			OutPath		= NormalizeDirectoryPath( PathStart );
			OutFilename	= IterStr + 1;
			return true;
		}
		--IterStr;
	}

	// No split found!
	OutPath		= "";
	OutFilename	= Filename;

	return false;
}

SimpleString FileUtil::GetUserPersonalPath()
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	char Path[ MAX_PATH ];
	if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, Path ) ) )
	{
		return SimpleString( Path );
	}
	else
	{
		return SimpleString( "./" );
	}
#else
	// TODO PORT LATER (not currently used)
	WARN;
	return SimpleString( "./" );
#endif
}

SimpleString FileUtil::GetUserLocalAppDataPath()
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	char Path[ MAX_PATH ];
	if( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, Path ) ) )
	{
		return NormalizeDirectoryPath( ( Path ) );
	}
	else
	{
		return SimpleString( "./" );
	}
#else
	// TODO PORT LATER (not currently used)
	WARN;
	return SimpleString( "./" );
#endif
}

// Cross-platform verified.
char CharToChar( char Char )	// PROBLEM, CODE REVIEWER? :D
{
	XTRACE_FUNCTION;

	Char = Char & 0xf;
	if( Char < 10 )
	{
		return '0' + Char;
	}
	else
	{
		return 'A' + ( Char - 10 );
	}
}

// Cross-platform verified.
SimpleString FileUtil::SanitizeFilename( const SimpleString& Filename )
{
	XTRACE_FUNCTION;

	Array<char> SanitizedFilename;
	Filename.FillArray( SanitizedFilename, true );
	for( uint CharIndex = 0; CharIndex < SanitizedFilename.Size(); ++CharIndex )
	{
		if( SanitizedFilename[ CharIndex ] == '\\' ||
			SanitizedFilename[ CharIndex ] == '/' ||
			SanitizedFilename[ CharIndex ] == ':' ||
			SanitizedFilename[ CharIndex ] == '*' ||
			SanitizedFilename[ CharIndex ] == '?' ||
			SanitizedFilename[ CharIndex ] == '"' ||
			SanitizedFilename[ CharIndex ] == '<' ||
			SanitizedFilename[ CharIndex ] == '>' ||
			SanitizedFilename[ CharIndex ] == '|' )
		{
			char Char = SanitizedFilename[ CharIndex ];

			SanitizedFilename.Remove( CharIndex );
			SanitizedFilename.Insert( CharToChar( Char & 0xf ), CharIndex );
			SanitizedFilename.Insert( CharToChar( Char >> 4 ), CharIndex );
			SanitizedFilename.Insert( '%', CharIndex );
		}
	}

	return SimpleString( SanitizedFilename );
}

bool FileUtil::TryShellExecute( const char* Filename, const char* Operation )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	HINSTANCE Result = ShellExecute(
		NULL,			// hWnd
		Operation,		// Operation (runas requests elevated access)
		Filename,		// File
		NULL,			// Parameters (or arguments to program)
		NULL,			// Directory
		SW_SHOWNORMAL	// Show flags (this makes sense in the context of opening a file in some editor)
		);

	const INT ExecuteResult = PtrToInt( Result );
	if( ExecuteResult > 32 )
	{
		return true;
	}
	else
	{
		PRINTF( "ShellExecute failed with code %d\n", ExecuteResult );
		return false;
	}
#else
	// TODO PORT LATER (not currently used)
	Unused( Filename );
	Unused( Operation );
	WARN;
	return false;
#endif
}

bool FileUtil::Launch( const char* Filename )
{
	XTRACE_FUNCTION;

#if BUILD_WINDOWS
	if( TryShellExecute( Filename, "runas" ) )
	{
		return true;
	}

	if( TryShellExecute( Filename, "open" ) )
	{
		return true;
	}

	return false;
#else
	// TODO PORT LATER (not currently used)
	Unused( Filename );
	WARN;
	return false;
#endif
}

#if BUILD_WINDOWS
// TODO PORT LATER (not currently used)
bool FileUtil::GetSaveFile( const HWND& hWnd, const SimpleString& Desc, const SimpleString& Ext, SimpleString& OutFileName )
{
	XTRACE_FUNCTION;

	// HACK, because string functions don't like dealing with \0 as part of a string.
	const SimpleString Filter = SimpleString::PrintF( "%s (*.%s)0*.%s0All Files (*.*)0*.*0", Desc.CStr(), Ext.CStr(), Ext.CStr() );
	Filter.Replace( '0', '\0' );

	OPENFILENAME OpenFileName;
	const uint MaxFilenameSize = 256;
	char FilenameBuffer[ MaxFilenameSize ];
	memset( &OpenFileName, 0, sizeof( OpenFileName ) );
	FilenameBuffer[0] = '\0';
	OpenFileName.lStructSize = sizeof( OpenFileName );
	OpenFileName.hwndOwner = hWnd;
	OpenFileName.lpstrFilter = Filter.CStr();
	OpenFileName.nFilterIndex = 1;
	OpenFileName.lpstrFile = FilenameBuffer;
	OpenFileName.nMaxFile = MaxFilenameSize;
	OpenFileName.lpstrInitialDir = ".";
	OpenFileName.Flags = OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;
	OpenFileName.lpstrDefExt = Ext.CStr();

	const SimpleString WorkingDirectory = FileUtil::GetWorkingDirectory();
	const BOOL Success = GetSaveFileName( &OpenFileName );
	FileUtil::ChangeWorkingDirectory( WorkingDirectory.CStr() );

	if( Success )
	{
		OutFileName = OpenFileName.lpstrFile;
		return true;
	}
	else
	{
		const DWORD Error = CommDlgExtendedError();
		PRINTF( "Error getting save file name: %d\n", Error );	// See cderr.h for meaning.
		return false;
	}
}
#endif

#if BUILD_WINDOWS
// TODO PORT LATER (not currently used)
bool FileUtil::GetLoadFile( const HWND& hWnd, const SimpleString& Desc, const SimpleString& Ext, SimpleString& OutFileName )
{
	XTRACE_FUNCTION;

	// HACK, because string functions don't like dealing with \0 as part of a string.
	const SimpleString Filter = SimpleString::PrintF( "%s (*.%s)0*.%s0All Files (*.*)0*.*0", Desc.CStr(), Ext.CStr(), Ext.CStr() );
	Filter.Replace( '0', '\0' );

	OPENFILENAME OpenFileName;
	const uint MaxFilenameSize = 256;
	char FilenameBuffer[ MaxFilenameSize ];
	memset( &OpenFileName, 0, sizeof( OpenFileName ) );
	FilenameBuffer[0] = '\0';
	OpenFileName.lStructSize = sizeof( OpenFileName );
	OpenFileName.hwndOwner = hWnd;
	OpenFileName.lpstrFilter = Filter.CStr();
	OpenFileName.nFilterIndex = 1;
	OpenFileName.lpstrFile = FilenameBuffer;
	OpenFileName.nMaxFile = MaxFilenameSize;
	OpenFileName.lpstrInitialDir = ".";
	OpenFileName.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST;
	OpenFileName.lpstrDefExt = Ext.CStr();

	const SimpleString WorkingDirectory = FileUtil::GetWorkingDirectory();
	const BOOL Success = GetOpenFileName( &OpenFileName );
	FileUtil::ChangeWorkingDirectory( WorkingDirectory.CStr() );

	if( Success )
	{
		OutFileName = OpenFileName.lpstrFile;
		return true;
	}
	else
	{
		const DWORD Error = CommDlgExtendedError();
		PRINTF( "Error getting load file name: %d\n", Error );	// See cderr.h for meaning.
		return false;
	}
}
#endif

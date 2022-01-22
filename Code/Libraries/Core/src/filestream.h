#ifndef FILESTREAM_H
#define FILESTREAM_H

#include <stdio.h>
#include "idatastream.h"
#include "array.h"
#include "simplestring.h"

class FileStream : public IDataStream
{
public:
	enum EFileMode
	{
		EFM_None,
		EFM_Read,
		EFM_Write,
		EFM_Append,
	};

	FileStream( const char* FileName, EFileMode FileMode );
	~FileStream();

	virtual int	Read( int NumBytes, void* Buffer ) const;
	virtual int	Write( int NumBytes, const void* Buffer ) const;
	virtual int PrintF( const char* Str, ... ) const;
	virtual int	SetPos( int Position ) const;
	virtual int	GetPos() const;
	virtual int EOS() const;
	virtual int	Size() const;

	const SimpleString&	GetQualifiedFilename() const { return m_QualifiedFilename; }

	// Clean up memory
	static void	StaticShutDown();

	// Add another resource folder where content can be found (besides the working directory)
	static void	StaticAddResPath( const SimpleString& ResPath );

	// This version also evokes FileUtil::GetFilesInFolder, enumerating every known file on disk or in package
	static void	StaticGetFilesInFolder( const SimpleString& Path, const bool Recursive, const SimpleString& Extension, Array<SimpleString>& OutFiles );

private:
	FileStream();

	FILE*			m_TheFile;
	EFileMode		m_FileMode;

	// The "qualified" filename includes whatever resource path prefix
	// may have been added. It is not necessarily an absolute path.
	SimpleString	m_QualifiedFilename;

	// List of other folders to look in (besides working directory)
	static Array<SimpleString>	sm_ResPaths;
};

#endif

#ifndef PACKSTREAM_H
#define PACKSTREAM_H

// PackStream replaces PackStream and supports multiple package files
// for superior patching, DLC, etc. Client code can manage the order in which
// package files are checked for matches so that newer packages may override
// content in older packages, e.g., for lightweight content patching.

#include "idatastream.h"
#include "array.h"
#include "simplestring.h"

#if BUILD_DEV
#include "set.h"
#endif

class FileStream;
class MemoryStream;

struct SPackageFileHeader
{
	SPackageFileHeader();

	uint	m_MagicID;
	uint	m_NumFiles;
	uint	m_OffsetToFiles;
	uint	m_FilesSize;
};

struct SPackageFileEntry
{
	SPackageFileEntry();

	SimpleString	m_Filename;
	uint			m_OffsetToFile;		// Offset from the start of the file block
	c_uint32		m_CompressedSize;
	c_uint32		m_DecompressedSize;
	c_uint32		m_Compressed;		// c_uint32 instead of bool to remind me that it would be padded out to this size anyway
};

struct SPackageFile
{
	SPackageFile();

	SimpleString*				m_Filename;
	IDataStream*				m_Stream;
	SPackageFileHeader			m_Header;
	Array<SPackageFileEntry*>	m_Entries;
	bool						m_InMemoryPackage;
};

class PackStream : public IDataStream
{
public:
	PackStream( const char* Filename, const bool UnpackFileIntoMemory = true );
	~PackStream();

	static void	StaticAddPackageFile( const char* PackageFilename, const bool PreemptExistingPackages = false );	// If PreemptExistingPackages, this package will be searched first for a match
	static void	StaticAddPackageInMemory( const char* PackageFilename, byte* const pPackageData, const uint PackageSize, const bool ReplaceIfOpen = false, const bool PreemptExistingPackages = false );
	static void StaticShutDown();
	static bool	StaticFileExists( const char* Filename );

	static void	StaticDestroyPackageFile( SPackageFile& PackageFile );

	// Corollary to FileUtil::GetFilesInFolder; only gets files contained within package files.
	// The Package argument filters by the contents of a particular package file; if it is "", all packages will be searched.
	// InMemoryPackagesOnly can be used to get only packages added by StaticAddPackageInMemory (e.g., from cloud downloads).
	static void	StaticGetFilesInFolder( const SimpleString& Path, const SimpleString& Package, const SimpleString& Extension, const bool InMemoryPackagesOnly, Array<SimpleString>& OutFiles );

	// This version also evokes FileStream::StaticGetFilesInFolder, enumerating every known file on disk or in package.
	// This includes loose files during dev, files in on-disk packages, and files in in-memory (cloud) packages. It gets EVERYTHING.
	static void	StaticGetFilesInFolder_Master( const SimpleString& Path, const bool Recursive, const SimpleString& Extension, Array<SimpleString>& OutFiles );

	virtual int	Read( int NumBytes, void* Buffer ) const;
	virtual int	Write( int NumBytes, const void* Buffer ) const;	// Not implemented for PackStream
	virtual int PrintF( const char* Str, ... ) const;				// Not implemented for PackStream
	virtual int SetPos( int Position ) const;
	virtual int	GetPos() const;
	virtual int EOS() const;
	virtual int	Size() const;

	// For file streaming from the package file; use with UnpackFileIntoMemory = false
	const SimpleString&	GetVirtualFilename() const { return m_Filename; }
	const char*			GetPhysicalFilename() const;
	uint				GetSubfileOffset() const;
	uint				GetSubfileLength() const;

#if BUILD_DEV
	static void	ReportUnusedPackageFiles();
#endif

private:
	static void	GetFilesInFolderInternal( const SimpleString& Path, const SimpleString& Package, const SimpleString& Extension, const bool InMemoryPackagesOnly, Array<SimpleString>& OutFiles );

	bool		FindFileEntry( const char* Filename, SPackageFile*& pOutPackageFile, SPackageFileEntry*& pOutFileEntry );

	// Members of the local stream
	SimpleString		m_Filename;				// The full name of the file that was requested, regardless of packed mode
	SPackageFile*		m_PackageFile;			// Which package file m_MemoryStream is reading from, if any
	SPackageFileEntry*	m_FileEntry;			// Which packed file m_MemoryStream represents
	FileStream*			m_FileStream;			// The non-packed file
	MemoryStream*		m_MemoryStream;			// The packed file decompressed to a buffer
	byte*				m_PackageFileBuffer;	// The actual buffer backing up m_MemoryStream
	bool				m_PackedMode;			// Is this stream reading from the decompressed memory stream or a loose file on disk

	// Static members for the global system
	static Array<SPackageFile>	sm_PackageFiles;

#if BUILD_DEV
	static Set<SimpleString>	sm_UsedPackageFiles;
#endif
};

#endif // PACKSTREAM_H

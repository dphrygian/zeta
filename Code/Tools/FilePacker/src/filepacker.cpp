#include "core.h"
#include "filepacker.h"
#include "filestream.h"
#include "fileutil.h"
#include "configmanager.h"

#include "zlib.h"

#include <memory.h>

FilePacker::FilePacker()
:	m_InFileSize( 0 )
,	m_InFileBuffer( NULL )
,	m_InFileCompressionSize( 0 )
,	m_InFileCompressionBuffer( NULL )
,	m_Header()
,	m_FileEntries()
,	m_ExistingPackFileSize( 0 )
,	m_ExistingPackFileBuffer( NULL ) {}

int FilePacker::PackFile( const char* InFilename, const char* PackFilename, bool Compress )
{
	m_FileEntries.Clear();
	m_InFileSize = FileUtil::Size( InFilename );

	ReadInFile( FileStream( InFilename, FileStream::EFM_Read ) );

	if( FileUtil::Exists( PackFilename ) )
	{
		ReadPackFile( FileStream( PackFilename, FileStream::EFM_Read ) );
	}
	else
	{
		m_Header.m_MagicID = 'KPCD';
		m_Header.m_NumFiles = 0;
		m_Header.m_OffsetToFiles = 0;	// Fix up just before writing
		m_Header.m_FilesSize = 0;
	}

	AddInFile( InFilename, Compress );

	WritePackFile( FileStream( PackFilename, FileStream::EFM_Write ) );

	return 0;
}

int FilePacker::UnpackFile( const char* PackFilename )
{
	if( FileUtil::Exists( PackFilename ) )
	{
		ReadPackFile( FileStream( PackFilename, FileStream::EFM_Read ) );
		for( uint i = 0; i < m_FileEntries.Size(); ++i )
		{
			SPackageFileEntry& FileEntry = m_FileEntries[i];
			FileUtil::RecursiveMakePath( FileEntry.m_Filename.CStr() );
			WriteUnpackedFile( FileStream( FileEntry.m_Filename.CStr(), FileStream::EFM_Write ), FileEntry );
		}
	}
	return 0;
}

int FilePacker::PackManifest( const SimpleString& PackFilename, const SimpleString& ManifestFilename )
{
	if( !FileUtil::Exists( ManifestFilename.CStr() ) )
	{
		PRINTF( "Manifest file %s not found.\n", ManifestFilename.CStr() );
		return 0;
	}

	if( FileUtil::Exists( PackFilename.CStr() ) )
	{
		FileUtil::RemoveFile( PackFilename.CStr() );
	}

	ConfigManager::Load( FileStream( ManifestFilename.CStr(), FileStream::EFM_Read ) );

	STATICHASH( NumFiles );
	const uint NumFiles = ConfigManager::GetInt( sNumFiles );
	for( uint FileIndex = 0; FileIndex < NumFiles; ++FileIndex )
	{
		const SimpleString	Filename	= ConfigManager::GetSequenceString( "File%d", FileIndex, "" );
		const bool			Compress	= ConfigManager::GetSequenceBool( "File%dCompress", FileIndex );

		PackFile( Filename.CStr(), PackFilename.CStr(), Compress );
	}

	return 0;
}

int FilePacker::Report( const char* PackFilename )
{
	if( FileUtil::Exists( PackFilename ) )
	{
		ReadPackFile( FileStream( PackFilename, FileStream::EFM_Read ) );

		PRINTF( "Package file contains %d files:\n", m_FileEntries.Size() );
		for( uint i = 0; i < m_FileEntries.Size(); ++i )
		{
			SPackageFileEntry& FileEntry = m_FileEntries[i];

			if( FileEntry.m_Compressed )
			{
				PRINTF( "\t%s (%d bytes compressed, %d bytes uncompressed)\n", FileEntry.m_Filename.CStr(), FileEntry.m_CompressedSize, FileEntry.m_DecompressedSize );
			}
			else
			{
				PRINTF( "\t%s (%d bytes)\n", FileEntry.m_Filename.CStr(), FileEntry.m_DecompressedSize );
			}
		}
	}

	return 0;
}

void FilePacker::WriteUnpackedFile( const IDataStream& Stream, SPackageFileEntry& FileEntry )
{
	byte* pFileData = m_ExistingPackFileBuffer + FileEntry.m_OffsetToFile;
	if( FileEntry.m_Compressed )
	{
		byte* pUncompressedData = new byte[ FileEntry.m_DecompressedSize ];
		uLong DecompressedSize = static_cast<uLong>( FileEntry.m_DecompressedSize );
		uncompress( pUncompressedData, &DecompressedSize, pFileData, FileEntry.m_CompressedSize );
		FileEntry.m_DecompressedSize = static_cast<c_uint32>( DecompressedSize );
		Stream.Write( FileEntry.m_DecompressedSize, pUncompressedData );
		SafeDeleteArray( pUncompressedData );
	}
	else
	{
		Stream.Write( FileEntry.m_DecompressedSize, pFileData );
	}

	PRINTF( "Unpacked %s (%d bytes)\n", FileEntry.m_Filename.CStr(), FileEntry.m_DecompressedSize );
}

void FilePacker::ReadInFile( const IDataStream& Stream )
{
	m_InFileBuffer = new byte[ m_InFileSize ];
	Stream.Read( m_InFileSize, m_InFileBuffer );
}

void FilePacker::ReadPackFile( const IDataStream& Stream )
{
	Stream.Read( sizeof( SPackageFileHeader ), &m_Header );
	ASSERT( m_Header.m_MagicID == 'KPCD' );

	for( uint i = 0; i < m_Header.m_NumFiles; ++i )
	{
		SPackageFileEntry FileEntry;

		FileEntry.m_Filename = Stream.ReadString();
		FileEntry.m_OffsetToFile = Stream.ReadUInt32();
		FileEntry.m_CompressedSize = Stream.ReadUInt32();
		FileEntry.m_DecompressedSize = Stream.ReadUInt32();
		FileEntry.m_Compressed = Stream.ReadUInt32();

		m_FileEntries.PushBack( FileEntry );
	}

	m_ExistingPackFileSize = m_Header.m_FilesSize;
	m_ExistingPackFileBuffer = new byte[ m_ExistingPackFileSize ];
	Stream.Read( m_ExistingPackFileSize, m_ExistingPackFileBuffer );
}

void FilePacker::AddInFile( const char* InFilename, bool Compress )
{
	m_InFileCompressionSize			= Compress ? compressBound( m_InFileSize ) : m_InFileSize;
	m_InFileCompressionBuffer		= new byte[ m_InFileCompressionSize ];

	SPackageFileEntry FileEntry;
	FileEntry.m_Filename			= FileUtil::Normalize( InFilename );
	FileEntry.m_OffsetToFile		= m_Header.m_FilesSize;
	FileEntry.m_CompressedSize		= m_InFileCompressionSize;
	FileEntry.m_DecompressedSize	= m_InFileSize;
	FileEntry.m_Compressed			= Compress ? 1 : 0;

	if( Compress )
	{
		uLong CompressedSize		= static_cast<uLong>( m_InFileCompressionSize );
		compress( m_InFileCompressionBuffer, &CompressedSize, m_InFileBuffer, m_InFileSize );

		// Update actual size after compression
		m_InFileCompressionSize		= static_cast<c_uint32>( CompressedSize );
		FileEntry.m_CompressedSize	= m_InFileCompressionSize;
	}
	else
	{
		memcpy_s( m_InFileCompressionBuffer, m_InFileCompressionSize, m_InFileBuffer, m_InFileSize );
	}

	m_Header.m_NumFiles++;
	m_FileEntries.PushBack( FileEntry );
}

void FilePacker::WritePackFile( const IDataStream& Stream )
{
	m_Header.m_OffsetToFiles = GetFilesOffset();
	m_Header.m_FilesSize = m_ExistingPackFileSize + m_InFileCompressionSize;

	Stream.Write( sizeof( SPackageFileHeader ), &m_Header );

	// Write file table of contents
	for( uint i = 0; i < m_FileEntries.Size(); ++i )
	{
		SPackageFileEntry& FileEntry = m_FileEntries[i];
		Stream.WriteString( FileEntry.m_Filename );
		Stream.WriteUInt32( FileEntry.m_OffsetToFile );
		Stream.WriteUInt32( FileEntry.m_CompressedSize );
		Stream.WriteUInt32( FileEntry.m_DecompressedSize );
		Stream.WriteUInt32( FileEntry.m_Compressed );
	}

	// Write existing blob of data
	if( m_ExistingPackFileBuffer )
	{
		Stream.Write( m_ExistingPackFileSize, m_ExistingPackFileBuffer );
	}

	// Write newly compressed data
	Stream.Write( m_InFileCompressionSize, m_InFileCompressionBuffer );
}

uint FilePacker::GetFilesOffset()
{
	// NOTE: This sizeof was causing a problem because the struct was word-aligned but
	// I wasn't writing the extra padding. No real good, general solution to this.
	uint SizeOfEntryMinusString = sizeof( SPackageFileEntry ) - sizeof( SimpleString );
	uint RetVal = 0;

	RetVal += sizeof( SPackageFileHeader );
	RetVal += SizeOfEntryMinusString * m_FileEntries.Size();
	for( uint i = 0; i < m_FileEntries.Size(); ++i )
	{
		RetVal += sizeof( c_uint32 );	// Because we write the length of the string first
		RetVal += ( m_FileEntries[i].m_Filename.Length() + 1 );
	}

	return RetVal;
}

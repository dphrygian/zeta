#include "core.h"
#include "checksum.h"
#include "idatastream.h"

static c_uint32 Adler32Mod = 65521;

// Simple, inefficient implementation
c_uint32 Checksum::Adler32( c_uint8* Stream, uint Length )
{
	c_uint32 A = 1;
	c_uint32 B = 0;

	for( uint Index = 0; Index < Length; ++Index )
	{
		A = ( A + Stream[ Index ] ) % Adler32Mod;
		B = ( B + A ) % Adler32Mod;
	}

	return ( B << 16 ) | A;
}

c_uint32 Checksum::Adler32( const IDataStream& Stream )
{
	c_uint32 A = 1;
	c_uint32 B = 0;

	int Length = Stream.Size();
	for( int Index = 0; Index < Length && !Stream.EOS(); ++Index )
	{
		A = ( A + Stream.ReadUInt8() ) % Adler32Mod;
		B = ( B + A ) % Adler32Mod;
	}

	return ( B << 16 ) | A;
}

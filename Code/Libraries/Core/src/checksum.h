#ifndef CHECKSUM_H
#define CHECKSUM_H

class IDataStream;

namespace Checksum
{
	c_uint32	Adler32( c_uint8* Stream, uint Length );
	c_uint32	Adler32( const IDataStream& Stream );
}

#endif // CHECKSUM_H

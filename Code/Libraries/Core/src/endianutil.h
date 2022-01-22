#ifndef ENDIAN_H
#define ENDIAN_H

namespace Endian
{
	inline c_uint32 Swap32( const c_uint32 i ) { return ( ( i & 0xff000000 ) >> 24 ) | ( ( i & 0x00ff0000 ) >> 8 ) | ( ( i & 0x0000ff00 ) << 8 ) | ( ( i & 0x000000ff ) << 24 ); }
	inline c_uint16 Swap16( const c_uint16 i ) { return ( ( i & 0xff00 ) >> 8 ) | ( ( i & 0x00ff ) << 8 ); }

	inline void SwapInPlace32( c_uint32& i ) { i = Endian::Swap32( i ); }
	inline void SwapInPlace16( c_uint16& i ) { i = Endian::Swap16( i ); }
}

#endif // ENDIAN_H

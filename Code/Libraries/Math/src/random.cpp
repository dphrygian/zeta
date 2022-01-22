#include "core.h"
#include "random.h"
#include <time.h>

Random::Random()
:	m_Seed( 0 )
,	m_MTI( RANDOM_N + 1 )
{
}

// Returns a number from 0 to Range (excluding Range)
uint Random::Get( uint Range )
{
	uint		y;
	static uint	mag01[2] = { 0, RANDOM_MATRIX_A };

	if( !Range )
	{
		return 0;
	}

	/* mag01[x] = x * MATRIX_A for x=0,1 */

	// Generate 624 words at a time (after we've exhausted the supply)
	if( m_MTI >= RANDOM_N )
	{
		int kk = 0;

		if( m_MTI == RANDOM_N + 1 )	// If not seeded
		{
			SetRandomSeed( 4357 );	// Default
		}

		for( kk = 0; kk < RANDOM_N - RANDOM_M; kk++ )
		{
			y = ( m_MT[ kk ] & RANDOM_UPPER_MASK ) | ( m_MT[ kk+1 ] & RANDOM_LOWER_MASK );
			m_MT[ kk ] = m_MT[ kk+RANDOM_M ] ^ ( y >> 1 ) ^ mag01[ y & 1 ];
		}

		for( ; kk < RANDOM_N - 1; kk++ )
		{
			y = ( m_MT[ kk ] & RANDOM_UPPER_MASK ) | ( m_MT[ kk+1 ] & RANDOM_LOWER_MASK );
			m_MT[ kk ] = m_MT[ kk + ( RANDOM_M-RANDOM_N ) ] ^ ( y >> 1 ) ^ mag01[ y & 1 ];
		}

		y = ( m_MT[ RANDOM_N - 1 ] & RANDOM_UPPER_MASK ) | ( m_MT[ 0 ] & RANDOM_LOWER_MASK );
		m_MT[ RANDOM_N - 1 ] = m_MT[ RANDOM_M - 1 ] ^ ( y >> 1 ) ^ mag01[ y & 1 ];

		m_MTI = 0;
	}

	y = m_MT[ m_MTI++ ];
	y ^= RANDOM_TEMPERING_SHIFT_U( y );
	y ^= RANDOM_TEMPERING_SHIFT_S( y ) & RANDOM_TEMPERING_MASK_B;
	y ^= RANDOM_TEMPERING_SHIFT_T( y ) & RANDOM_TEMPERING_MASK_C;
	y ^= RANDOM_TEMPERING_SHIFT_L( y );

	return y % Range;
}

void Random::SetRandomSeed( uint Seed )
{
	// setting initial seeds to m_MT[N] using the generator Line 25 of Table 1 in
	// [KNUTH 1981, The Art of Computer Programming Vol. 2 (2nd Ed.), pp102]
	m_MT[ 0 ] = Seed & 0xffffffff;	// I guess this is a guard against larger than 32-bit numbers
	for( m_MTI = 1; m_MTI < RANDOM_N; ++m_MTI )
	{
		m_MT[ m_MTI ] = ( 69069 * m_MT[ m_MTI - 1 ] ) & 0xffffffff;
	}

	m_Seed = Seed;

	PRINTF( "Random seed is %d\n", m_Seed );
}

uint Random::GetRandomSeed() const
{
	return m_Seed;
}

void Random::Seed()
{
	SetRandomSeed( static_cast<uint>( time( NULL ) ) );
}

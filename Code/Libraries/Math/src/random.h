#ifndef RANDOM_H
#define RANDOM_H

//Example:
//	Random RNG;
//	RNG.Seed();
//	uint Num = RNG.Get( 100 );	// returns a number from 0-99 inclusive

// Mersenne Twister PRNG, code from Game Coding Complete by Mike McShaffry

// Period parameters
#define RANDOM_N					624
#define RANDOM_M					397
#define RANDOM_MATRIX_A				0x9908b0df	// Constant vector a
#define RANDOM_UPPER_MASK			0x80000000	// Most significant w-r bits
#define RANDOM_LOWER_MASK			0x7fffffff	// Least significant r bits

// Tempering parameters
#define RANDOM_TEMPERING_MASK_B		0x9d2c5680
#define RANDOM_TEMPERING_MASK_C		0xefc60000
#define RANDOM_TEMPERING_SHIFT_U(y)	((y) >> 11)
#define RANDOM_TEMPERING_SHIFT_S(y)	((y) << 7)
#define RANDOM_TEMPERING_SHIFT_T(y)	((y) << 15)
#define RANDOM_TEMPERING_SHIFT_L(y)	((y) >> 18)

class Random
{
	// DATA
	uint		m_Seed;
	c_uint32	m_MT[ RANDOM_N ];	// Array for state vector
	uint		m_MTI;

	//FUNCTIONS
public:
	Random();

	uint	Get( uint Range );	// Returns a random number in [0, Range - 1] (i.e., excluding Range)
	void	Seed();
	void	SetRandomSeed( uint Seed );
	uint	GetRandomSeed() const;
};

#endif // RANDOM_H

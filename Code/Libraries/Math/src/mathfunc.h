#ifndef MATHFUNC_H
#define MATHFUNC_H

#include "vector2.h"
#include "vector.h"
#include "vector4.h"
#include "angles.h"
#include "array.h"

typedef void ( *BresenhamPointFunc )( int, int, bool&, void* );
typedef void ( *Bresenham3PointFunc )( int, int, int, bool&, void* );

namespace Math
{
	void	Bresenham( int x0, int y0, int x1, int y1, BresenhamPointFunc PointFunc, void* pContext );
	void	Bresenham3( int x0, int y0, int z0, int x1, int y1, int z1, Bresenham3PointFunc PointFunc, void* pContext );

	float	RandomF();						// Returns [0..1]
	bool	RandomF( float Chance );		// Chance in [0,1], returns rand < Chance.

	uint	Random( uint Range );			// Returns [0, Range-1], meant for e.g. random array indexing with Range as size of array
	uint	Random( uint Min, uint Max );	// Inclusive of Max
	int		Random( int Min, int Max );		// Inclusive of Max
	float	Random( float Min, float Max );
	Vector2	Random( const Vector2& Min, const Vector2& Max );
	Vector	Random( const Vector& Min, const Vector& Max );
	Vector4	Random( const Vector4& Min, const Vector4& Max );
	Angles	Random( const Angles& Min, const Angles& Max );
	bool	Random();

	template<class C> C& ArrayRandom( Array<C>& A ) { return A[ Random( A.Size() ) ]; }
	template<class C> const C& ArrayRandom( const Array<C>& A ) { return A[ Random( A.Size() ) ]; }

	// This returns the index, not the item, since that's what I typically want.
	template<class C, typename TWeightFunc> uint ArrayWeightedRandom( const Array<C>& A, TWeightFunc WeightFunc )
	{
		float		WeightSum		= 0.0f;
		//FOR_EACH_ARRAY( ArrayIter, A, C )
		for( typename Array<C>::Iterator ArrayIter = (A).Begin(); ArrayIter.IsValid(); ++ArrayIter )
		{
			const C&	Item	= ArrayIter.GetValue();
			WeightSum			+= WeightFunc( Item );
		}

		const float	SelectedWeight	= Math::Random( 0.0f, WeightSum );
		uint		SelectedIndex	= 0;
		//FOR_EACH_ARRAY_REVERSE( ArrayIter, A, C )
		for( typename Array<C>::Iterator ArrayIter = (A).End(); ArrayIter.IsValid(); --ArrayIter )
		{
			const C&	Item	= ArrayIter.GetValue();
			WeightSum			-= WeightFunc( Item );

			if( SelectedWeight > WeightSum )
			{
				SelectedIndex	= ArrayIter.GetIndex();
				break;
			}
		}

		return SelectedIndex;
	}

	// Fisher-Yates/Knuth shuffle
	template<class C> void ArrayShuffle( Array<C>& A )
	{
		if( A.Size() < 2 )
		{
			return;
		}

		for( uint i = A.Size() - 1; i > 0; --i )
		{
			const uint	j	= Random( i + 1 );	// 0 <= j <= i
			const C		Ai	= A[i];				// Make a copy for swap
			A[i]			= A[j];
			A[j]			= Ai;
		}
	}

	void	SeedGenerator();
	void	SeedGenerator( uint Seed );
}

#endif // MATHFUNC_H

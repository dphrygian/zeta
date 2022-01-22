#ifndef ROSADAMAGE_H
#define ROSADAMAGE_H

#include "array.h"
#include "map.h"
#include "hashedstring.h"

namespace RosaDamage
{
	enum EResistedDamageFlags
	{
		ERDF_None		= 0x0,
		ERDF_Unmodified	= 0x1,
		ERDF_Resisted	= 0x2,
		ERDF_Vulnerable	= 0x4,
	};

	struct SDamageVector
	{
		SDamageVector()
		:	m_DamageAmount( 0.0f )
		,	m_StaggerDuration( 0.0f )
		,	m_DamageType()
		{
		}

		float			m_DamageAmount;		// ROSATODO: Replace with PE if random ranges are desired
		float			m_StaggerDuration;
		HashedString	m_DamageType;
	};

	struct SDebuff
	{
		SDebuff()
		:	m_DebuffName()
		,	m_DamageScalar( 0.0f )
		{
		}

		HashedString	m_DebuffName;
		float			m_DamageScalar;
	};

	struct SDamageSet
	{
		SDamageSet()
		:	m_DamageVectors()
		,	m_Debuffs()
		,	m_Faction()
		{
		}

		Array<SDamageVector>	m_DamageVectors;
		Array<SDebuff>			m_Debuffs;
		HashedString			m_Faction;	// HACKHACK, set damaged entity to this faction when hit
	};

	// Formerly called "damage type mod", this can be used for both resistances and vulnerabilities
	// These are applied to both damage amount and stagger duration
	struct SResistance
	{
		SResistance()
		:	m_DamageScalar( 0.0f )
		,	m_StaggerScalar( 0.0f )
		{
		}

		float	m_DamageScalar;
		float	m_StaggerScalar;
	};

	struct SResistanceSet
	{
		SResistanceSet()
		:	m_Resistances()
		{
		}

		Map<HashedString, SResistance>	m_Resistances;	// Keyed by damage type; null key's resistance is applied to all unspecified types
	};

	struct SResistedDamage
	{
		SResistedDamage()
		:	m_DamageAmount( 0.0f )
		,	m_StaggerDuration( 0.0f )
		{
		}

		float	m_DamageAmount;
		float	m_StaggerDuration;
	};

	// For managing statically allocated memory
	void					AddRef();
	void					Release();

	const SDamageSet&		GetDamageSet( const HashedString& DamageSetName );
	const SResistanceSet&	GetResistanceSet( const HashedString& ResistanceSetName );

	const Array<SDebuff>&	GetDebuffs( const HashedString& DamageSetName );

	const HashedString		GetFaction( const HashedString& DamageSetName );

	// Returns a bitwise combination of EResistedDamageFlags
	uint					GetResistedDamageAmount( const HashedString& DamageSetName, const HashedString& ResistanceSetName, SResistedDamage& OutResistedDamage );
	uint					GetResistedDamageAmount( const SDamageVector& DamageVector, const SResistanceSet& ResistanceSet, SResistedDamage& OutResistedDamage );
}

#endif // ROSADAMAGE_H

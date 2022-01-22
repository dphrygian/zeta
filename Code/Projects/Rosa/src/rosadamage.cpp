#include "core.h"
#include "rosadamage.h"
#include "map.h"
#include "configmanager.h"
#include "mathcore.h"

typedef Map<HashedString, RosaDamage::SDamageSet>		TDamageSetMap;
typedef Map<HashedString, RosaDamage::SResistanceSet>	TResistanceSetMap;

static TDamageSetMap		sDamageSetMap;
static TResistanceSetMap	sResistanceSetMap;
static int					sRefCount = 0;

static void StaticShutDown()
{
	sDamageSetMap.Clear();
	sResistanceSetMap.Clear();
}

void RosaDamage::AddRef()
{
	sRefCount++;
}

void RosaDamage::Release()
{
	if( --sRefCount == 0 )
	{
		StaticShutDown();
	}
}

const RosaDamage::SDamageSet& RosaDamage::GetDamageSet( const HashedString& DamageSetName )
{
	TDamageSetMap::Iterator DamageSetIter = sDamageSetMap.Search( DamageSetName );
	if( DamageSetIter.IsValid() )
	{
		return DamageSetIter.GetValue();
	}

	// Lazily initialize damage set
	SDamageSet& DamageSet = sDamageSetMap.Insert( DamageSetName );

	STATICHASH( NumDamageVectors );
	const uint NumDamageVectors = ConfigManager::GetInheritedInt( sNumDamageVectors, 0, DamageSetName );
	FOR_EACH_INDEX( DamageVectorIndex, NumDamageVectors )
	{
		SDamageVector&	DamageVector	= DamageSet.m_DamageVectors.PushBack();
		DamageVector.m_DamageAmount		= ConfigManager::GetInheritedSequenceFloat(	"DamageVector%dAmount",				DamageVectorIndex, 0.0f,						DamageSetName );
		DamageVector.m_StaggerDuration	= ConfigManager::GetInheritedSequenceFloat(	"DamageVector%dStaggerDuration",	DamageVectorIndex, 0.0f,						DamageSetName );
		DamageVector.m_DamageType		= ConfigManager::GetInheritedSequenceHash(	"DamageVector%dType",				DamageVectorIndex, HashedString::NullString,	DamageSetName );
	}

	STATICHASH( NumDebuffs );
	const uint NumDebuffs = ConfigManager::GetInheritedInt( sNumDebuffs, 0, DamageSetName );
	FOR_EACH_INDEX( DebuffIndex, NumDebuffs )
	{
		SDebuff&	Debuff		= DamageSet.m_Debuffs.PushBack();
		Debuff.m_DebuffName		= ConfigManager::GetInheritedSequenceHash(	"Debuff%dName",		DebuffIndex, HashedString::NullString,	DamageSetName );
		Debuff.m_DamageScalar	= ConfigManager::GetInheritedSequenceFloat(	"Debuff%dScalar",	DebuffIndex, 0.0f,						DamageSetName );
	}

	STATICHASH( Faction );
	DamageSet.m_Faction = ConfigManager::GetInheritedHash( sFaction, HashedString::NullString, DamageSetName );

	return DamageSet;
}

const RosaDamage::SResistanceSet& RosaDamage::GetResistanceSet( const HashedString& ResistanceSetName )
{
	TResistanceSetMap::Iterator ResistanceSetIter = sResistanceSetMap.Search( ResistanceSetName );
	if( ResistanceSetIter.IsValid() )
	{
		return ResistanceSetIter.GetValue();
	}

	// Lazily initialize resistance set
	SResistanceSet& ResistanceSet = sResistanceSetMap.Insert( ResistanceSetName );

	STATICHASH( NumResistances );
	const uint NumResistances = ConfigManager::GetInheritedInt( sNumResistances, 0, ResistanceSetName );
	FOR_EACH_INDEX( ResistanceIndex, NumResistances )
	{
		const HashedString	DamageType	= ConfigManager::GetInheritedSequenceHash(	"Resistance%dDamageType",		ResistanceIndex, HashedString::NullString,	ResistanceSetName );

		SResistance&		Resistance	= ResistanceSet.m_Resistances.Insert( DamageType );
		const float			Scalar		= ConfigManager::GetInheritedSequenceFloat(	"Resistance%dScalar",			ResistanceIndex, 1.0f,						ResistanceSetName );
		Resistance.m_DamageScalar		= ConfigManager::GetInheritedSequenceFloat(	"Resistance%dDamageScalar",		ResistanceIndex, Scalar,					ResistanceSetName );
		Resistance.m_StaggerScalar		= ConfigManager::GetInheritedSequenceFloat(	"Resistance%dStaggerScalar",	ResistanceIndex, Scalar,					ResistanceSetName );
	}

	return ResistanceSet;
}

const Array<RosaDamage::SDebuff>& RosaDamage::GetDebuffs( const HashedString& DamageSetName )
{
	const SDamageSet& DamageSet = GetDamageSet( DamageSetName );
	return DamageSet.m_Debuffs;
}

const HashedString RosaDamage::GetFaction( const HashedString& DamageSetName )
{
	const SDamageSet& DamageSet = GetDamageSet( DamageSetName );
	return DamageSet.m_Faction;
}

uint RosaDamage::GetResistedDamageAmount( const HashedString& DamageSetName, const HashedString& ResistanceSetName, SResistedDamage& OutResistedDamage )
{
	const SDamageSet&		DamageSet			= GetDamageSet(		DamageSetName );
	const SResistanceSet&	ResistanceSet		= GetResistanceSet(	ResistanceSetName );
	uint					ResistedDamageFlags	= ERDF_None;

	FOR_EACH_ARRAY( DamageVectorIter, DamageSet.m_DamageVectors, SDamageVector )
	{
		const SDamageVector& DamageVector = DamageVectorIter.GetValue();
		ResistedDamageFlags |= GetResistedDamageAmount( DamageVector, ResistanceSet, OutResistedDamage );
	}

	return ResistedDamageFlags;
}

uint RosaDamage::GetResistedDamageAmount( const SDamageVector& DamageVector, const SResistanceSet& ResistanceSet, SResistedDamage& OutResistedDamage )
{
	Map<HashedString, SResistance>::Iterator ResistanceIter = ResistanceSet.m_Resistances.Search( DamageVector.m_DamageType );

	// If we don't have a resistance for the specific type, try a null/default resistance
	if( ResistanceIter.IsNull() )
	{
		ResistanceIter = ResistanceSet.m_Resistances.Search( HashedString::NullString );
	}

	if( ResistanceIter.IsNull() )
	{
		// If we still don't have a resistance, use the unmodified amounts
		OutResistedDamage.m_DamageAmount	+= DamageVector.m_DamageAmount;
		OutResistedDamage.m_StaggerDuration	+= DamageVector.m_StaggerDuration;
		return ERDF_Unmodified;
	}
	else
	{
		const SResistance&	Resistance		= ResistanceIter.GetValue();
		OutResistedDamage.m_DamageAmount	+= Max( 0.0f, DamageVector.m_DamageAmount		* Resistance.m_DamageScalar );
		OutResistedDamage.m_StaggerDuration	+= Max( 0.0f, DamageVector.m_StaggerDuration	* Resistance.m_StaggerScalar );

		return ( Resistance.m_DamageScalar == 1.0f ) ? ERDF_Unmodified : ( ( Resistance.m_DamageScalar > 1.0f ) ? ERDF_Vulnerable : ERDF_Resisted );
	}
}

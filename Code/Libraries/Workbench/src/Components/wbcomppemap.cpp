#include "core.h"
#include "wbcomppemap.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"

WBCompPEMap::WBCompPEMap()
:	m_PEMap()
{
}

WBCompPEMap::~WBCompPEMap()
{
	FOR_EACH_MAP( PEIter, m_PEMap, HashedString, WBPE* )
	{
		WBPE* PE = PEIter.GetValue();
		SafeDelete( PE );
	}
}

/*virtual*/ void WBCompPEMap::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	// Optionally add PEs from other sets (or other PEMap components, ignoring their other properties)
	STATICHASH( NumPEMaps );
	const uint NumPEMaps = ConfigManager::GetInheritedInt( sNumPEMaps, 0, sDefinitionName );
	FOR_EACH_INDEX( PEMapIndex, NumPEMaps )
	{
		const SimpleString PEMap = ConfigManager::GetInheritedSequenceString( "PEMap%d", PEMapIndex, "", sDefinitionName );
		AddPEMap( PEMap );
	}

	// Add local PE map last so it overrides anything else
	AddPEMap( DefinitionName );
}

void WBCompPEMap::AddPEMap( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumPEs );
	const uint NumPEs = ConfigManager::GetInheritedInt( sNumPEs, 0, sDefinitionName );
	for( uint PEIndex = 0; PEIndex < NumPEs; ++PEIndex )
	{
		const HashedString	PEKey		= ConfigManager::GetInheritedSequenceHash(		"PE%dKey", PEIndex, HashedString::NullString,	sDefinitionName );
		const SimpleString	PEName		= ConfigManager::GetInheritedSequenceString(	"PE%dDef", PEIndex, "",							sDefinitionName );

		// Delete any previous PE in case we're overriding something
		WBPE*				pExistingPE	= GetPE( PEKey );
		SafeDelete( pExistingPE );

		// Then create and add the new PE
		WBPE* const			pNewPE		= WBParamEvaluatorFactory::Create( PEName );
		DEVASSERT( pNewPE );

		m_PEMap.Insert( PEKey, pNewPE );
	}
}

WBPE* WBCompPEMap::GetPE( const HashedString& Name ) const
{
	const TPEMap::Iterator PEIter = m_PEMap.Search( Name );
	return PEIter.IsValid() ? PEIter.GetValue() : NULL;
}

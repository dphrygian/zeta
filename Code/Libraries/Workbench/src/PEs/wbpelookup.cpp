#include "core.h"
#include "wbpelookup.h"
#include "configmanager.h"
#include "Components/wbcomppemap.h"
#include "reversehash.h"

WBPELookup::WBPELookup()
:	m_Key()
#if BUILD_DEV
,	m_WarnFailure( false )
#endif
{
}

WBPELookup::~WBPELookup()
{
}

/*virtual*/ void WBPELookup::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBPE::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Key );
	m_Key = ConfigManager::GetHash( sKey, HashedString::NullString, sDefinitionName );

#if BUILD_DEV
	STATICHASH( WarnFailure );
	m_WarnFailure = ConfigManager::GetBool( sWarnFailure, true, sDefinitionName );
#endif
}

/*virtual*/ void WBPELookup::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	DEVASSERT( Context.m_Entity );

	WBCompPEMap* const PEMap = WB_GETCOMP( Context.m_Entity, PEMap );
	if( !PEMap )
	{
		WARNDESC( "No PEMap for PE lookup." );
		return;
	}

	WBPE* const PE = PEMap->GetPE( m_Key );
	if( !PE )
	{
#if BUILD_DEV
		if( m_WarnFailure )
		{
			PRINTF( "PE lookup for key %s found nothing in PEMap.\n", ReverseHash::ReversedHash( m_Key ).CStr() );
			WARNDESC( "PE lookup found nothing in PEMap." );
		}
#endif
		return;
	}

	PE->Evaluate( Context, EvaluatedParam );
}

#include "core.h"
#include "wbperosahaskeycard.h"
#include "rosagame.h"
#include "Components/wbcomprosakeyring.h"
#include "configmanager.h"

WBPERosaHasKeycard::WBPERosaHasKeycard()
:	m_Keycard()
{
}

WBPERosaHasKeycard::~WBPERosaHasKeycard()
{
}

/*virtual*/ void WBPERosaHasKeycard::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Keycard );
	m_Keycard = ConfigManager::GetHash( sKeycard, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPERosaHasKeycard::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	WBEntity* const			pPlayer		= RosaGame::GetPlayer();
	if( !pPlayer )
	{
		return;
	}

	WBCompRosaKeyRing* const	pKeyRing	= WB_GETCOMP( pPlayer, RosaKeyRing );
	if( !pKeyRing )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pKeyRing->HasKeycard( m_Keycard );
}

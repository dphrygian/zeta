#include "core.h"
#include "wbperosahasammo.h"
#include "rosagame.h"
#include "Components/wbcomprosaammobag.h"
#include "configmanager.h"

WBPERosaHasAmmo::WBPERosaHasAmmo()
:	m_Type()
,	m_TypePE()
,	m_Count( 0 )
,	m_CountPE()
{
}

WBPERosaHasAmmo::~WBPERosaHasAmmo()
{
}

/*virtual*/ void WBPERosaHasAmmo::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Type );
	m_Type = ConfigManager::GetHash( sType, HashedString::NullString, sDefinitionName );

	STATICHASH( TypePE );
	const SimpleString TypePEDef = ConfigManager::GetString( sTypePE, "", sDefinitionName );
	m_TypePE.InitializeFromDefinition( TypePEDef );

	STATICHASH( Count );
	m_Count = ConfigManager::GetInt( sCount, 1, sDefinitionName );

	STATICHASH( CountPE );
	const SimpleString CountPEDef = ConfigManager::GetString( sCountPE, "", sDefinitionName );
	m_CountPE.InitializeFromDefinition( CountPEDef );
}

/*virtual*/ void WBPERosaHasAmmo::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	WBEntity* const				pPlayer		= RosaGame::GetPlayer();
	if( !pPlayer )
	{
		return;
	}

	WBCompRosaAmmoBag* const	pAmmoBag	= WB_GETCOMP( pPlayer, RosaAmmoBag );
	if( !pAmmoBag )
	{
		return;
	}

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity						= pPlayer;

	m_TypePE.Evaluate( PEContext );
	const HashedString			Type		= m_TypePE.HasRoot() ? m_TypePE.GetString() : m_Type;

	m_CountPE.Evaluate( PEContext );
	const uint					Count		= m_CountPE.HasRoot() ? m_CountPE.GetInt() : m_Count;

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pAmmoBag->HasAmmo( Type, Count );
}

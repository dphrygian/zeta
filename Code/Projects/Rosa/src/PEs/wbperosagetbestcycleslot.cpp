#include "core.h"
#include "wbperosagetbestcycleslot.h"
#include "Components/wbcomprosainventory.h"
#include "rosagame.h"
#include "reversehash.h"

WBPERosaGetBestCycleSlot::WBPERosaGetBestCycleSlot()
{
}

WBPERosaGetBestCycleSlot::~WBPERosaGetBestCycleSlot()
{
}

/*virtual*/ void WBPERosaGetBestCycleSlot::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	WBEntity* const				pPlayer				= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaInventory* const	pInventory			= WB_GETCOMP( pPlayer, RosaInventory );
	DEVASSERT( pInventory );

	const HashedString			BestCycleSlot		= pInventory->GetBestCycleSlot();
	const SimpleString			BestCycleSlotStr	= ReverseHash::ReversedHash( BestCycleSlot );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
	EvaluatedParam.m_String	= BestCycleSlotStr;
}

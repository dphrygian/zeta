#include "core.h"
#include "wbperosahasmrusave.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosasaveload.h"

WBPERosaHasMRUSave::WBPERosaHasMRUSave()
{
}

WBPERosaHasMRUSave::~WBPERosaHasMRUSave()
{
}

/*virtual*/ void WBPERosaHasMRUSave::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pGame );

	RosaSaveLoad* const		pSaveLoad	= pGame->GetSaveLoad();
	DEVASSERT( pSaveLoad );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pSaveLoad->HasMRUSave();
}

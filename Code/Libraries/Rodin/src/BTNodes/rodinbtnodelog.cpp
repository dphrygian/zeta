#include "core.h"
#include "rodinbtnodelog.h"
#include "configmanager.h"
#include "wbentity.h"

RodinBTNodeLog::RodinBTNodeLog()
:	m_Text()
{
}

RodinBTNodeLog::~RodinBTNodeLog()
{
}

void RodinBTNodeLog::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Text );
	m_Text = ConfigManager::GetString( sText, "", sDefinitionName );
}

RodinBTNode::ETickStatus RodinBTNodeLog::Tick( const float DeltaTime )
{
	Unused( DeltaTime );

	PRINTF( "BT Log (%.2f): %s (%s)\n", GetTime(), m_Text.CStr(), GetEntity()->GetUniqueName().CStr() );

	return ETS_Success;
}

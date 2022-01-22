#ifndef WBACTIONROSAPLAYSOUND_H
#define WBACTIONROSAPLAYSOUND_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaPlaySound : public WBAction
{
public:
	WBActionRosaPlaySound();
	virtual ~WBActionRosaPlaySound();

	DEFINE_WBACTION_FACTORY( RosaPlaySound );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	WBParamEvaluator	m_EntityPE;		// Source entity for the sound; defaults to topmost owner
	HashedString		m_Sound;
	WBParamEvaluator	m_SoundPE;
	bool				m_Attached;
	bool				m_Muted;
	float				m_Volume;
	WBParamEvaluator	m_VolumePE;
	WBParamEvaluator	m_LocationPE;
};

#endif // WBACTIONROSAPLAYSOUND_H

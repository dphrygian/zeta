#ifndef ROSAHUDLOG_H
#define ROSAHUDLOG_H

#include "simplestring.h"
#include "array.h"

class RosaHUDLog
{
public:
	RosaHUDLog();
	~RosaHUDLog();

	void	Clear();
	void	AddEntry( const SimpleString& EntryMessage );

	void	Tick();

	static void	StaticAddMessage( const HashedString& Message );
	static void	StaticAddDynamicMessage( const HashedString& Message );

private:
	void	InitializeFromDefinition( const SimpleString& DefinitionName );

	void	PublishString();

	struct SEntry
	{
		SEntry()
		:	m_Message()
		,	m_ExpireTime( 0.0f )
		{
		}

		SimpleString	m_Message;
		float			m_ExpireTime;
	};

	float				m_EntryDuration;
	Array<SEntry>		m_Entries;
};

#endif // ROSAHUDLOG_H

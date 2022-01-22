#ifndef WBCOMPROSAOBJECTIVES_H
#define WBCOMPROSAOBJECTIVES_H

#include "wbrosacomponent.h"
#include "array.h"
#include "hashedstring.h"

class WBCompRosaObjectives : public WBRosaComponent
{
public:
	WBCompRosaObjectives();
	virtual ~WBCompRosaObjectives();

	DEFINE_WBCOMP( RosaObjectives, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsObjectiveComplete( const HashedString& ObjectiveTag, const bool RejectFail ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PushPersistence() const;
	void			PullPersistence();

	void			ClearObjectives();
	uint			AddObjective( const HashedString& ObjectiveTag );	// Returns index of added objective
	void			CompleteObjective( const HashedString& ObjectiveTag, const bool Fail, const bool ForceAdd );

	bool			ObjectiveExists( const HashedString& ObjectiveTag, uint& OutIndex ) const;

	void			PublishToHUD() const;

	enum EObjectiveStatus
	{
		EOS_None,
		EOS_Incomplete,
		EOS_Succeeded,
		EOS_Failed,
	};

	struct SObjective
	{
		SObjective()
		:	m_ObjectiveTag()
		,	m_ObjectiveStatus( EOS_None )
		{
		}

		HashedString		m_ObjectiveTag;
		EObjectiveStatus	m_ObjectiveStatus;
		// ROSANOTE: Need to change the serialization code if more elements are added!
		// It assumes the size of this struct is a constant between versions.
	};

	Array<SObjective>	m_Objectives;	// Serialized
	bool				m_Persist;		// Config; set to true if objectives should persist through levels
};

#endif // WBCOMPROSAOBJECTIVES_H

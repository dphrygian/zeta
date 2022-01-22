#ifndef WBACTIONFACTORY_H
#define WBACTIONFACTORY_H

#include "wbaction.h"
#include "array.h"

class SimpleString;
class WBEvent;
class WBEntity;

namespace WBActionFactory
{
	void		RegisterFactory( const HashedString& TypeName, WBActionFactoryFunc Factory );
	void		InitializeBaseFactories();
	void		ShutDown();

	WBAction*	Create( const SimpleString& DefinitionName );

	void		InitializeActionArray( const HashedString& DefinitionName, Array<WBAction*>& OutActionArray );
	void		ClearActionArray( Array<WBAction*>& OutActionArray );

	// Added so I can specify from sources like NumXYZActions/XYZAction0/... instead of just NumActions/Action0/...
	void		InitializeActionArray( const HashedString& DefinitionName, const SimpleString& ActionPrefix, Array<WBAction*>& OutActionArray );

	// Extending the meaning of this namespace into just everything dealing with action arrays
	void		ExecuteActionArray( const Array<WBAction*>& ActionArray, const WBEvent& Event, WBEntity* const pActingEntity );
};

#endif // WBACTIONFACTORY_H

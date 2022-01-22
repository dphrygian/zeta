#include "core.h"
#include "rosawbscenehelpers.h"
#include "wbscene.h"
#include "wbentity.h"
#include "Components/wbcomprosatransform.h"
#include "mathcore.h"

#if BUILD_DEBUG
#define BEGIN_ITERATING_ENTITIES do{ ++m_IteratingEntitiesRefCount; } while(0)
#define END_ITERATING_ENTITIES do{ --m_IteratingEntitiesRefCount; } while(0)
#define CHECK_ITERATING_ENTITIES do{ DEVASSERT( 0 == m_IteratingEntitiesRefCount ); } while(0)
#else
#define BEGIN_ITERATING_ENTITIES DoNothing
#define END_ITERATING_ENTITIES DoNothing
#define CHECK_ITERATING_ENTITIES DoNothing
#endif

void WBScene::GetEntitiesByRadius( Array<WBEntity*>& OutEntities, const Vector& Location, const float Radius ) const
{
	const float RadiusSquared = Square( Radius );

	BEGIN_ITERATING_ENTITIES;
	FOR_EACH_MAP( EntityIter, m_Entities, uint, SEntityRef )
	{
		const SEntityRef& EntityRef = EntityIter.GetValue();
		WBEntity* const pEntity = EntityRef.m_Entity;
		DEVASSERT( pEntity );

		if( EntityRef.m_Removed || pEntity->IsDestroyed() )
		{
			continue;
		}

		WBCompRosaTransform* const pTransform = pEntity->GetTransformComponent<WBCompRosaTransform>();
		if( !pTransform )
		{
			continue;
		}

		const Vector Offset = Location - pTransform->GetLocation();
		if( Offset.LengthSquared() <= RadiusSquared )
		{
			OutEntities.PushBack( pEntity );
		}
	}
	END_ITERATING_ENTITIES;
}

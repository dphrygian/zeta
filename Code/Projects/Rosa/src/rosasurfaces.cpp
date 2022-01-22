#include "core.h"
#include "rosasurfaces.h"
#include "map.h"
#include "hashedstring.h"
#include "configmanager.h"

typedef Map<HashedString, RosaSurfaces::SSurface> TSurfaceMap;

static TSurfaceMap	sSurfaceMap;
static int			sRefCount = 0;

static void StaticInitialize();
static void StaticShutDown();

void RosaSurfaces::AddRef()
{
	if( sRefCount++ == 0 )
	{
		StaticInitialize();
	}
}

void RosaSurfaces::Release()
{
	if( --sRefCount == 0 )
	{
		StaticShutDown();
	}
}

const RosaSurfaces::SSurface& RosaSurfaces::GetSurface( const HashedString& Surface )
{
	DEVASSERT( sRefCount > 0 );

	const TSurfaceMap::Iterator SurfaceIter = sSurfaceMap.Search( Surface );
	if( SurfaceIter.IsValid() )
	{
		return SurfaceIter.GetValue();
	}
	else
	{
		WARNDESC( "Unknown surface looked up in RosaSurfaces::GetSurface" );
		return sSurfaceMap[ HashedString::NullString ];	// Will create the default surface if it doesn't exist
	}
}

SimpleString RosaSurfaces::GetSound( const HashedString& Surface )
{
	return GetSurface( Surface ).m_Sound;
}

float RosaSurfaces::GetVolumeScalar( const HashedString& Surface )
{
	return GetSurface( Surface ).m_VolumeScalar;
}

float RosaSurfaces::GetRadiusScalar( const HashedString& Surface )
{
	return GetSurface( Surface ).m_RadiusScalar;
}

void StaticInitialize()
{
	STATICHASH( RosaSurfaces );

	STATICHASH( NumSurfaces );
	const uint NumSurfaces = ConfigManager::GetInt( sNumSurfaces, 0, sRosaSurfaces );

	for( uint SurfaceIndex = 0; SurfaceIndex < NumSurfaces; ++SurfaceIndex )
	{
		const HashedString			SurfaceName	= ConfigManager::GetSequenceHash( "Surface%d", SurfaceIndex, HashedString::NullString, sRosaSurfaces );
		RosaSurfaces::SSurface&	Surface		= sSurfaceMap[ SurfaceName ];

		Surface.m_Sound							= ConfigManager::GetSequenceString( "Surface%dSound", SurfaceIndex, "", sRosaSurfaces );
		Surface.m_VolumeScalar					= ConfigManager::GetSequenceFloat( "Surface%dVolumeScalar", SurfaceIndex, 1.0f, sRosaSurfaces );
		Surface.m_RadiusScalar					= ConfigManager::GetSequenceFloat( "Surface%dRadiusScalar", SurfaceIndex, 1.0f, sRosaSurfaces );
	}
}

void StaticShutDown()
{
	sSurfaceMap.Clear();
}

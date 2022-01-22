#include "core.h"
#include "cubemapcommon.h"
#include "packstream.h"
#include "configmanager.h"

CubemapCommon::CubemapCommon()
{
}

CubemapCommon::~CubemapCommon()
{
}

void CubemapCommon::Initialize( const SimpleString& CubemapDef, const bool NoMips )
{
	SCubemapData CubemapData;

	MAKEHASH( CubemapDef );

	Array<SimpleString> Filenames;

	// Ordered X+, X-, Y+, Y-, Z+, Z-
	// (or right, left, front, back, up, down)

	STATICHASH( Right );
	Filenames.PushBack( ConfigManager::GetInheritedString( sRight, "", sCubemapDef ) );

	STATICHASH( Left );
	Filenames.PushBack( ConfigManager::GetInheritedString( sLeft, "", sCubemapDef ) );

	STATICHASH( Front );
	Filenames.PushBack( ConfigManager::GetInheritedString( sFront, "", sCubemapDef ) );

	STATICHASH( Back );
	Filenames.PushBack( ConfigManager::GetInheritedString( sBack, "", sCubemapDef ) );

	STATICHASH( Up );
	Filenames.PushBack( ConfigManager::GetInheritedString( sUp, "", sCubemapDef ) );

	STATICHASH( Down );
	Filenames.PushBack( ConfigManager::GetInheritedString( sDown, "", sCubemapDef ) );

	for( uint Side = 0; Side < 6; ++Side )
	{
		const SimpleString&	Filename	= Filenames[ Side ];
		STextureData&		TextureData	= CubemapData.m_Textures[ Side ];

		if( Filename.EndsWith( "bmp" ) )
		{
			TextureCommon::StaticLoadBMP( PackStream( Filename.CStr() ), TextureData, NoMips );
		}
		else if( Filename.EndsWith( "tga" ) )
		{
			TextureCommon::StaticLoadTGA( PackStream( Filename.CStr() ), TextureData, NoMips );
		}
		else if( Filename.EndsWith( "dds" ) )
		{
			LoadDDS( PackStream( Filename.CStr() ), TextureData );
		}
		else
		{
			DEVWARNDESC( "CubemapCommon::Initialize: Unknown file extension." );
		}
	}

	CreateCubemap( CubemapData );
}

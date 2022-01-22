#include "core.h"
#include "renderstates.h"
#include "hashedstring.h"

ECullMode RenderStates::GetCullMode( const HashedString& CullMode )
{
#define TRY_CULLMODE( cullmode )		\
	STATIC_HASHED_STRING( cullmode );	\
	if( CullMode == s##cullmode )		\
	{									\
		return ECM_##cullmode;			\
	}

	TRY_CULLMODE( None );
	TRY_CULLMODE( CW );
	TRY_CULLMODE( CCW );

#undef TRY_CULLMODE

	DEVWARN;
	return ECM_Unknown;
}

EFunc RenderStates::GetFunc( const HashedString& Func )
{
#define TRY_FUNC( func )			\
	STATIC_HASHED_STRING( func );	\
	if( Func == s##func )			\
	{								\
	return EF_##func;				\
}

	TRY_FUNC( Never );
	TRY_FUNC( Less );
	TRY_FUNC( Equal );
	TRY_FUNC( LessEqual );
	TRY_FUNC( Greater );
	TRY_FUNC( NotEqual );
	TRY_FUNC( GreaterEqual );
	TRY_FUNC( Always );

#undef TRY_FUNC

	DEVWARN;
	return EF_Unknown;
}

EBlend RenderStates::GetBlend( const HashedString& Blend )
{
#define TRY_BLEND( blend )			\
	STATIC_HASHED_STRING( blend );	\
	if( Blend == s##blend )			\
	{								\
		return EB_##blend;			\
	}

	TRY_BLEND( Zero );
	TRY_BLEND( One );
	TRY_BLEND( SrcColor );
	TRY_BLEND( InvSrcColor );
	TRY_BLEND( SrcAlpha );
	TRY_BLEND( InvSrcAlpha );
	TRY_BLEND( DestAlpha );
	TRY_BLEND( InvDestAlpha );
	TRY_BLEND( DestColor );
	TRY_BLEND( InvDestColor );
	TRY_BLEND( SrcAlphaSat );

#undef TRY_BLEND

	DEVWARN;
	return EB_Unknown;
}

ESOp RenderStates::GetStencilOp( const HashedString& Op )
{
#define TRY_STENCILOP( op )		\
	STATIC_HASHED_STRING( op );	\
	if( Op == s##op )			\
	{							\
	return ESO_##op;			\
}

	TRY_STENCILOP( Keep );
	TRY_STENCILOP( Zero );
	TRY_STENCILOP( Replace );
	TRY_STENCILOP( Incr );
	TRY_STENCILOP( Decr );
	TRY_STENCILOP( Invert );
	TRY_STENCILOP( IncrWrap );
	TRY_STENCILOP( DecrWrap );

#undef TRY_STENCILOP

	DEVWARN;
	return ESO_Unknown;
}

ETextureAddress RenderStates::GetTextureAddress( const HashedString& TextureAddress )
{
#define TRY_TEXTUREADDRESS( textureaddress )	\
	STATIC_HASHED_STRING( textureaddress );		\
	if( TextureAddress == s##textureaddress )	\
	{											\
		return ETA_##textureaddress;			\
	}

	TRY_TEXTUREADDRESS( Wrap );
	TRY_TEXTUREADDRESS( Mirror );
	TRY_TEXTUREADDRESS( Clamp );
	TRY_TEXTUREADDRESS( Border );

#undef TRY_TEXTUREADDRESS

	DEVWARN;
	return ETA_Unknown;
}

ETextureFilter RenderStates::GetTextureFilter( const HashedString& TextureFilter )
{
#define TRY_TEXTUREFILTER( texturefilter )	\
	STATIC_HASHED_STRING( texturefilter );	\
	if( TextureFilter == s##texturefilter )	\
	{										\
		return ETF_##texturefilter;			\
	}

	TRY_TEXTUREFILTER( None );
	TRY_TEXTUREFILTER( Point );
	TRY_TEXTUREFILTER( Linear );
	TRY_TEXTUREFILTER( Anisotropic );

#undef TRY_TEXTUREFILTER

	DEVWARN;
	return ETF_Unknown;
}

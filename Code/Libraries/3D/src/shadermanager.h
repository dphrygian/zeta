#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include "map.h"
#include "hashedstring.h"

class IRenderer;
class IShaderProgram;
class IVertexShader;
class IPixelShader;
class IVertexDeclaration;

class ShaderManager
{
public:
	ShaderManager( IRenderer* Renderer );
	~ShaderManager();

	IShaderProgram*	GetShaderProgram( const SimpleString& VertexShaderFilename, const SimpleString& PixelShaderFilename, const uint VertexSignature );
	IVertexShader*	GetVertexShader( const SimpleString& Filename );
	IPixelShader*	GetPixelShader( const SimpleString& Filename );

	void			FreeShaders();

private:
	struct SShaderProgramKey
	{
		HashedString	m_VertexShaderHash;
		HashedString	m_PixelShaderHash;
		uint			m_VertexSignature;

		bool operator<( const SShaderProgramKey& Key ) const
		{
			if( m_VertexShaderHash < Key.m_VertexShaderHash )
			{
				return true;
			}

			if( m_VertexShaderHash > Key.m_VertexShaderHash )
			{
				return false;
			}

			if( m_PixelShaderHash < Key.m_PixelShaderHash )
			{
				return true;
			}

			if( m_PixelShaderHash > Key.m_PixelShaderHash )
			{
				return false;
			}

			if( m_VertexSignature < Key.m_VertexSignature )
			{
				return true;
			}

			return false;
		}

		bool operator==( const SShaderProgramKey& Key ) const
		{
			return
				m_VertexShaderHash == Key.m_VertexShaderHash &&
				m_PixelShaderHash == Key.m_PixelShaderHash &&
				m_VertexSignature == Key.m_VertexSignature;
		}
	};

	Map<SShaderProgramKey, IShaderProgram*>	m_ShaderPrograms;
	Map<HashedString, IVertexShader*>		m_VertexShaders;
	Map<HashedString, IPixelShader*>		m_PixelShaders;

	IRenderer*								m_Renderer;
};

#endif // SHADERMANAGER_H

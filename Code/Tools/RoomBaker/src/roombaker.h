#ifndef ROOMBAKER_H
#define ROOMBAKER_H

#include "idatastream.h"
#include "simplestring.h"
#include "rosaworldgen.h"

class RosaRoomEditor;
class MeshFactory;

class RoomBaker
{
public:
	RoomBaker();
	~RoomBaker();

	int Bake( const SimpleString& InFilename, const SimpleString& OutFilename );

	void BakeRoom( const RosaRoomEditor& PreBakeRoom, const SimpleString& OutFilename ) const;
	void BakeWorld( const RosaRoomEditor& PreBakeRoom, const IDataStream& Stream ) const;

private:
	struct SWorldGenBakedRoom
	{
		SWorldGenBakedRoom()
		:	m_RoomFilename()
		,	m_RoomLoc()
		,	m_RoomTransform( RosaWorldGen::ERoomXform::ERT_None )
		{
		}

		SimpleString			m_RoomFilename;
		RosaWorldGen::SRoomLoc	m_RoomLoc;
		uint					m_RoomTransform;
	};

	MeshFactory* m_pMeshFactory;
};

#endif // ROOMBAKER_H

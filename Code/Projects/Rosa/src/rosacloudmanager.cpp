#include "core.h"
#include "rosacloudmanager.h"
#include "configmanager.h"
#include "simplestring.h"
#include "stringmanager.h"
#include "httpsocket.h"
#include "packstream.h"
#include "checksum.h"
#include "wbworld.h"
#include "wbeventmanager.h"
#include "rosacampaign.h"

#define ROSA_CLOUD_DOWNLOAD_BY_DEFAULT_DEV	0
#define ROSA_CLOUD_DOWNLOAD_BY_DEFAULT		0 // BUILD_FINAL || ROSA_CLOUD_DOWNLOAD_BY_DEFAULT_DEV

RosaCloudManager::RosaCloudManager()
:	m_CloudAgent()
,	m_CloudHostName()
,	m_CloudPaths()
,	m_pHTTPSocket( NULL )
,	m_CloudReceiveBuffer()
,	m_PackageIndex( 0 )
,	m_CloudPackages()
,	m_CloudPackageChecksums()
{
	m_pHTTPSocket = new HTTPSocket;
}

RosaCloudManager::~RosaCloudManager()
{
	SafeDelete( m_pHTTPSocket );
}

void RosaCloudManager::Initialize()
{
	STATICHASH( CloudAgent );
	m_CloudAgent = StringManager::ParseConfigString( ConfigManager::GetString( sCloudAgent, "" ) );

	STATICHASH( CloudHostName );
	m_CloudHostName = ConfigManager::GetString( sCloudHostName, "" );

	STATICHASH( NumCloudPackages );
	const uint NumCloudPackages = ConfigManager::GetInt( sNumCloudPackages );

	FOR_EACH_INDEX( CloudPackageIndex, NumCloudPackages )
	{
		const SimpleString CloudPackage = ConfigManager::GetSequenceString( "CloudPackage%d", CloudPackageIndex, "" );
		DEVASSERT( CloudPackage != "" );
		m_CloudPaths.PushBack( CloudPackage );
	}

	// HACKHACK: Need to set this because it manages state
	m_PackageIndex = m_CloudPaths.Size();

	STATICHASH( CloudBufferSize );
	const uint CloudBufferSize = ConfigManager::GetInt( sCloudBufferSize );
	m_CloudReceiveBuffer.Resize( CloudBufferSize );

#if ROSA_CLOUD_DOWNLOAD_BY_DEFAULT
	DownloadPackages();
#else
	OnStarted();
	OnFinished();
#endif
}

void RosaCloudManager::DownloadPackages()
{
	if( m_PackageIndex < m_CloudPaths.Size() )
	{
		// Download is in progress, let it finish
		return;
	}

	OnStarted();

	m_PackageIndex = 0;
	StartNextDownload();
}

void RosaCloudManager::StartNextDownload()
{
	DEVASSERT( m_pHTTPSocket->AsyncHasFinished() );

	if( m_PackageIndex >= m_CloudPaths.Size() )
	{
		// We've finished all the downloads
		OnFinished();
		return;
	}

	HTTPSocket::SSocketInit SocketInit;
	SocketInit.m_Agent				= m_CloudAgent;
	SocketInit.m_HostName			= m_CloudHostName;
	SocketInit.m_Path				= m_CloudPaths[ m_PackageIndex ];
	SocketInit.m_pReceiveBuffer		= m_CloudReceiveBuffer.GetData();
	SocketInit.m_pReceiveBufferSize	= m_CloudReceiveBuffer.Size();

	m_pHTTPSocket->AsyncGet( SocketInit );
}

void RosaCloudManager::Tick()
{
	if( m_PackageIndex >= m_CloudPaths.Size() )
	{
		// We've finished all the downloads
		return;
	}

	if( m_pHTTPSocket->AsyncHasFinished() )
	{
		int BodyOffset, BodySize;
		if( m_pHTTPSocket->DidSucceed( BodyOffset, BodySize ) )
		{
			const SimpleString&	CloudPath		= m_CloudPaths[ m_PackageIndex ];
			Array<byte>&		CloudPackage	= m_CloudPackages[ CloudPath ];
			CloudPackage.Resize( BodySize );
			memcpy_s( CloudPackage.GetData(), BodySize, m_CloudReceiveBuffer.GetData() + BodyOffset, BodySize );

			c_uint32&			ChecksumOld		= m_CloudPackageChecksums[ CloudPath ];
			const c_uint32		ChecksumNew		= Checksum::Adler32( CloudPackage.GetData(), BodySize );
			if( ChecksumOld != ChecksumNew )
			{
				ChecksumOld = ChecksumNew;

				PackStream::StaticAddPackageInMemory(
					CloudPath.CStr() + 1 /*skip the leading slash*/,
					CloudPackage.GetData(),
					BodySize,
					true /*replace if open*/,
					false /*preempt existing packages*/ );

				PRINTF( "RosaCloudManager: Added cloud package %s\n", CloudPath.CStr() + 1 );
			}
			else
			{
				// We've downloaded the same package we already had, skip subsequent downloads and finish
				m_PackageIndex = m_CloudPaths.Size();
				OnFinished();
				return;
			}
		}

		++m_PackageIndex;
		StartNextDownload();
	}
}

void RosaCloudManager::OnStarted()
{
	RosaCampaign* const pCampaign = RosaCampaign::GetCampaign();
	DEVASSERT( pCampaign );

	// Campaign is recipient so it handles the results before anyone else touches them and gets stale data
	WB_MAKE_EVENT( OnCloudSyncStarted, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnCloudSyncStarted, pCampaign );
}

void RosaCloudManager::OnFinished()
{
	RosaCampaign* const pCampaign = RosaCampaign::GetCampaign();
	DEVASSERT( pCampaign );

	// Campaign is recipient so it handles the results before anyone else touches them and gets stale data
	WB_MAKE_EVENT( OnCloudSyncFinished, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnCloudSyncFinished, pCampaign );
}

#include "core.h"
#include "httpsocket.h"
#include "configmanager.h"
#include "thread.h"
#include "mutex.h"

#include <memory.h>

#if !BUILD_WINDOWS
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#define CRLF	"\r\n"

HTTPSocket::SSocketInit::SSocketInit()
:	m_Agent()
,	m_HostName()
,	m_Path()
,	m_ContentType()
{
}

HTTPSocket::HTTPSocket()
:	m_ConnectSendRecvThread( NULL )
,	m_ThreadFinishedMutex( NULL )
,	m_SocketInit()
,	m_SendBuffer()
,	m_Finished( false )
,	m_Success( false )
,	m_BytesReceived( 0 )
,	m_BodyOffset( 0 )
,	m_BodySize( 0 )
{
#if BUILD_WINDOWS
	WSADATA SocketData;
	CHECK( NO_ERROR == WSAStartup( MAKEWORD( 2, 2 ), &SocketData ) );
#endif
}

HTTPSocket::~HTTPSocket()
{
#if BUILD_WINDOWS
	WSACleanup();
#endif

	SafeDelete( m_ConnectSendRecvThread );
	SafeDelete( m_ThreadFinishedMutex );
}

void HTTPSocket::AsyncGet( const SSocketInit& SocketInit )
{
	// Set up everything the other thread will need
	InitConnect( SocketInit );

	const SimpleString Header =
		SimpleString( "GET " ) + SocketInit.m_Path + SimpleString( " HTTP/1.1" CRLF ) +
		SimpleString( "Host: " ) + SocketInit.m_HostName + SimpleString( CRLF ) +
		SimpleString( "User-Agent: " ) + SocketInit.m_Agent + SimpleString( CRLF ) +
		SimpleString( "Connection: close" CRLF ) +
		SimpleString( CRLF );

	const uint SendBufferSize = Header.Length() + 1;
	m_SendBuffer.Resize( SendBufferSize );
	memcpy_s( m_SendBuffer.GetData(), SendBufferSize, Header.CStr(), SendBufferSize );

	// Then launch the thread
	DEVASSERT( AsyncHasFinished() );
	SafeDelete( m_ConnectSendRecvThread );
	SafeDelete( m_ThreadFinishedMutex );
	m_ConnectSendRecvThread = new Thread( ConnectSendRecvThreadProc, this );
	m_ThreadFinishedMutex = new Mutex;
}

void HTTPSocket::AsyncPost( const SSocketInit& SocketInit, const Array<char>& Buffer )
{
	const uint BufferSize = Buffer.Size();

	// Set up everything the other thread will need
	InitConnect( SocketInit );

	const SimpleString Header =
		SimpleString( "POST " ) + SocketInit.m_Path + SimpleString( " HTTP/1.1" CRLF ) +
		SimpleString( "Host: " ) + SocketInit.m_HostName + SimpleString( CRLF ) +
		SimpleString( "Content-Type: " ) + SocketInit.m_ContentType + SimpleString( CRLF ) +
		SimpleString( "Content-Length: " ) + SimpleString::PrintF( "%d", BufferSize ) + SimpleString( CRLF ) +
		SimpleString( "User-Agent: " ) + SocketInit.m_Agent + SimpleString( CRLF ) +
		SimpleString( "Connection: close" CRLF ) +
		SimpleString( CRLF );

	const uint HeaderSize		= Header.Length();
	const uint SendBufferSize	= HeaderSize + BufferSize;
	m_SendBuffer.Resize( SendBufferSize );
	memcpy_s( m_SendBuffer.GetData(),				HeaderSize, Header.CStr(),		HeaderSize );
	memcpy_s( m_SendBuffer.GetData() + HeaderSize,	BufferSize, Buffer.GetData(),	BufferSize );
	m_SendBuffer.PushBack( '\0' );	// Need to append a terminating null after everything else

	// Then launch the thread
	DEVASSERT( AsyncHasFinished() );
	SafeDelete( m_ConnectSendRecvThread );
	SafeDelete( m_ThreadFinishedMutex );
	m_ConnectSendRecvThread = new Thread( ConnectSendRecvThreadProc, this );
	m_ThreadFinishedMutex = new Mutex;
}

void HTTPSocket::InitConnect( const SSocketInit& SocketInit )
{
	m_Finished		= false;
	m_Success		= false;
	m_BytesReceived	= 0;
	m_BodyOffset	= 0;
	m_BodySize		= 0;

	m_SocketInit	= SocketInit;
	DEVASSERT( m_SocketInit.m_HostName != "" );
}

bool HTTPSocket::AsyncHasFinished()
{
	if( NULL == m_ConnectSendRecvThread )
	{
		return true;
	}

	DEVASSERT( m_ThreadFinishedMutex );
	m_ThreadFinishedMutex->Wait();
	const bool Result = m_Finished;
	m_ThreadFinishedMutex->Release();

	if( Result )
	{
		m_ConnectSendRecvThread->Wait();	// HACKHACK: Wait on the thread to force it to clean up in SDL
		SafeDelete( m_ConnectSendRecvThread );
		SafeDelete( m_ThreadFinishedMutex );
	}

	return Result;
}

bool HTTPSocket::DidSucceed( int& OutBodyOffset, int& OutBodySize )
{
	DEVASSERT( AsyncHasFinished() );

	OutBodyOffset	= m_BodyOffset;
	OutBodySize		= m_BodySize;

	return m_Success;
}

void HTTPSocket::SocketThread_MarkFinished()
{
	m_ThreadFinishedMutex->Wait();
	m_Finished = true;
	m_ThreadFinishedMutex->Release();
}

void HTTPSocket::SocketThread_ConnectSendRecv()
{
#if BUILD_WINDOWS
	SOCKET Socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( INVALID_SOCKET == Socket )
#else
	int Socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( -1 == Socket )
#endif
	{
		return;
	}

	sockaddr_in SocketAddress;
	SocketAddress.sin_family	= AF_INET;
	SocketAddress.sin_port		= htons( 80 );	// HTTP is port 80

	addrinfo Hints;
	memset( &Hints, 0, sizeof( Hints ) );
	Hints.ai_family		= AF_INET;
	Hints.ai_socktype	= SOCK_STREAM;
	Hints.ai_protocol	= IPPROTO_TCP;

	addrinfo*	pAddressInfo	= NULL;
	const int	Success			= getaddrinfo( m_SocketInit.m_HostName.CStr(), "80", &Hints, &pAddressInfo );
	if( NULL != pAddressInfo )
	{
		sockaddr_in* const pResolvedSocketAddress = ( sockaddr_in* )pAddressInfo->ai_addr;

		// Use IP address resolved from host name
		SocketAddress.sin_addr.s_addr = pResolvedSocketAddress->sin_addr.s_addr;

		freeaddrinfo( pAddressInfo );
	}

	if( 0 != Success )
	{
		return;
	}

#if BUILD_WINDOWS
	if( SOCKET_ERROR == connect( Socket, ( SOCKADDR* )&SocketAddress, sizeof( sockaddr_in ) ) )
#else
	if( -1 == connect( Socket, ( struct sockaddr* )&SocketAddress, sizeof( sockaddr_in ) ) )
#endif
	{
		return;
	}

#if BUILD_DEV && BUILD_WINDOWS
	// Assert that we can send a message of the given size all at once
	uint MaxSize;
	int SizeOf = sizeof( uint );
	if( SOCKET_ERROR != getsockopt( Socket, SOL_SOCKET, SO_MAX_MSG_SIZE, ( char* )&MaxSize, &SizeOf ) )
	{
		DEVASSERT( MaxSize >= m_SendBuffer.Size() );
	}
#endif

	const uint BytesSent = send( Socket, m_SendBuffer.GetData(), m_SendBuffer.Size(), 0 );
	Unused( BytesSent );
	DEVASSERT( BytesSent == m_SendBuffer.Size() );

	int BytesReceived = 0;
	int TotalBytesReceived = 0;

	do
	{
		BytesReceived = recv( Socket, m_SocketInit.m_pReceiveBuffer + TotalBytesReceived, m_SocketInit.m_pReceiveBufferSize - TotalBytesReceived, 0 );
		TotalBytesReceived += BytesReceived;

		// Treat errors as end of stream
#if BUILD_WINDOWS
		if( SOCKET_ERROR == BytesReceived )
#else
		if( -1 == BytesReceived )
#endif
		{
			BytesReceived = 0;
		}
	}
	while( BytesReceived );

	m_SocketInit.m_pReceiveBuffer[ TotalBytesReceived ] = '\0';

	// Validate the response
	char* const pStatusLine			= m_SocketInit.m_pReceiveBuffer;
	char* const pStatusLineEnd		= strstr( m_SocketInit.m_pReceiveBuffer, CRLF );
	if( NULL == pStatusLineEnd )
	{
		return;
	}

	// Make sure we got an OK response
	char Temp = *pStatusLineEnd;
	*pStatusLineEnd = '\0';
	if( NULL == strstr( pStatusLine, "200" ) )
	{
		return;
	}
	*pStatusLineEnd = Temp;

	// Get the content length
	char* const pContentLength		= strstr( m_SocketInit.m_pReceiveBuffer, "Content-Length: " );
	if( NULL == pContentLength )
	{
		return;
	}

	char* const pContentLengthEnd	= strstr( m_SocketInit.m_pReceiveBuffer, CRLF );
	if( NULL == pContentLengthEnd )
	{
		return;
	}

	Temp = *pContentLengthEnd;
	*pContentLengthEnd = '\0';
	const uint ContentLength = atoi( pContentLength + 16 );	// Skip the "Content-Length: "
	*pContentLengthEnd = Temp;

	// Skip all the rest of the headers and find the content
	char* pContentStart				= strstr( m_SocketInit.m_pReceiveBuffer, CRLF CRLF );
	if( NULL == pContentStart )
	{
		return;
	}
	pContentStart += 4;	// Skip the CRLF CRLF

	// Everything checks out!
	m_BodyOffset	= static_cast<uint>( pContentStart - pStatusLine );
	m_BodySize		= ContentLength;
	m_BytesReceived	= TotalBytesReceived;
	m_Success		= true;
}

#if BUILD_WINDOWS_NO_SDL
/*static*/ DWORD WINAPI HTTPSocket::ConnectSendRecvThreadProc( void* Parameter )
#endif
#if BUILD_SDL
/*static*/ int HTTPSocket::ConnectSendRecvThreadProc( void* Parameter )
#endif
{
	HTTPSocket* const pThis = static_cast<HTTPSocket*>( Parameter );
	DEVASSERT( pThis );
	DEVASSERT( pThis->m_ThreadFinishedMutex );

	pThis->SocketThread_ConnectSendRecv();
	pThis->SocketThread_MarkFinished();

	return 0;
}

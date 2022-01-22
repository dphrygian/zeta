#ifndef HTTPSOCKET_H
#define HTTPSOCKET_H

// DLP 18 Jun 2019: I'm trimming this down a bit, it's now always async, always auto-closing,
// doesn't allocate its own memory, and does connect and GET/POST in one thread.

#include "simplestring.h"
#include "array.h"

#if BUILD_WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#endif

class Thread;
class Mutex;

class HTTPSocket
{
public:
	struct SSocketInit
	{
		SSocketInit();

		SimpleString	m_Agent;				// This app's name, for the server
		SimpleString	m_HostName;				// e.g., "www.dphrygian.com"
		SimpleString	m_Path;					// e.g., "/index.html"
		SimpleString	m_ContentType;			// For POSTs only
		char*			m_pReceiveBuffer;		// Should be initialized by caller, will be unsafe to use until thread has completed
		uint			m_pReceiveBufferSize;
	};

	HTTPSocket();
	~HTTPSocket();

	void			AsyncGet( const SSocketInit& SocketInit );
	void			AsyncPost( const SSocketInit& SocketInit, const Array<char>& SendBuffer );

	// Call this before checking results or touching the receive buffer, else socket thread may be using them
	bool			AsyncHasFinished();

	bool			DidSucceed( int& OutBodyOffset, int& OutBodySize );

private:
	void			InitConnect( const SSocketInit& SocketInit );

	void			SocketThread_ConnectSendRecv();
	void			SocketThread_MarkFinished();

	Thread*			m_ConnectSendRecvThread;
	Mutex*			m_ThreadFinishedMutex;

	// Used on socket thread after initialization on main thread
	SSocketInit		m_SocketInit;
	Array<char>		m_SendBuffer;

	bool			m_Finished;
	bool			m_Success;
	uint			m_BytesReceived;
	uint			m_BodyOffset;
	uint			m_BodySize;

#if BUILD_WINDOWS_NO_SDL
	static DWORD WINAPI	ConnectSendRecvThreadProc( void* Parameter );
#endif
#if BUILD_SDL
	static int			ConnectSendRecvThreadProc( void* Parameter );
#endif
};

#endif // HTTPSOCKET_H

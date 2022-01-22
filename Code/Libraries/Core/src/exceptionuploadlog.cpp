#include "core.h"
#include "exceptionuploadlog.h"
#include "httpsocket.h"
#include "filestream.h"

void FillSendBufferFromLog( Array<char>& SendBuffer, const char* LogFilename )
{
	const FileStream	LogStream	= FileStream( LogFilename, FileStream::EFM_Read );
	const int			LogSize		= LogStream.Size();

	SendBuffer.Resize( LogSize );
	LogStream.Read( LogSize, SendBuffer.GetData() );
}

void ExceptionUploadLog::UploadLog()
{
	// And hey, why not also upload the log?
	// Hard-coded server paths because I can't guarantee I'll have access to config manager.
	// Synchronous because uh yeah I'm in the middle of handling an exception.
	PrintManager* pPrintManager = PrintManager::GetInstance_NoAlloc();
	if( pPrintManager )
	{
		Array<char> SendBuffer;
		Array<char> ReceiveBuffer;
		FillSendBufferFromLog( SendBuffer, pPrintManager->GetLogFilename() );
		ReceiveBuffer.Resize( 1024 * 1024 );

		HTTPSocket::SSocketInit SocketInit;
		SocketInit.m_Agent				= "ExceptionHandler_LogUploader";
		SocketInit.m_ContentType		= "text/plain";
		SocketInit.m_HostName			= "www.dphrygian.com";
		SocketInit.m_Path				= "/cgi-bin/logs.cgi";
		SocketInit.m_pReceiveBuffer		= ReceiveBuffer.GetData();
		SocketInit.m_pReceiveBufferSize	= ReceiveBuffer.Size();

		HTTPSocket Socket;
		Socket.AsyncPost( SocketInit, SendBuffer );
		do
		{
		}
		while( !Socket.AsyncHasFinished() );
	}
}

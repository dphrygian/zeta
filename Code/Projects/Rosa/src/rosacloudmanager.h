#ifndef ROSACLOUDMANAGER_H
#define ROSACLOUDMANAGER_H

#include "array.h"
#include "simplestring.h"
#include "map.h"

class HTTPSocket;

class RosaCloudManager
{
public:
	RosaCloudManager();
	~RosaCloudManager();

	void	Initialize();
	void	DownloadPackages();

	void	Tick();

private:
	void	StartNextDownload();

	void	OnStarted();
	void	OnFinished();

	SimpleString		m_CloudAgent;
	SimpleString		m_CloudHostName;
	Array<SimpleString>	m_CloudPaths;
	HTTPSocket*			m_pHTTPSocket;
	Array<char>			m_CloudReceiveBuffer;
	uint				m_PackageIndex;

	// Contents of each downloaded path, for making a memory stream and adding to pack stream
	Map<SimpleString, Array<byte> >	m_CloudPackages;

	// Checksums to avoid subsequent redownloads when refreshing
	Map<SimpleString, c_uint32>		m_CloudPackageChecksums;
};

#endif // ROSACLOUDMANAGER_H

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

// Parses a plain text config file, either for use in the engine
// at runtime, or for offline compilation in a tool.

class IDataStream;

namespace ConfigParser
{
	void Parse( const IDataStream& Stream, const bool WarnIfOverwriting = false );
	void ParseTiny( const IDataStream& Stream, const bool WarnIfOverwriting = false );	// Parse compact form ([Context:]Name=Value)

	bool EvaluateConditional( const IDataStream& Stream );
};

#endif // CONFIGPARSER_H

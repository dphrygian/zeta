#include "core.h"
#include "test.h"
#include "hashedstring.h"
#include "list.h"
#include "array.h"
#include "stack.h"
#include "hashtable.h"
#include "map.h"
#include "allocator.h"
#include "filestream.h"
#include "console.h"
#include "stringmanager.h"
#include "configmanager.h"
#include "configparser.h"
#include "multimap.h"
#include "set.h"
#include "httpsocket.h"
#include "thread.h"

#include <Windows.h>
#include <crtdbg.h>

int CountDestructs()
{
	static int Destructs = 0;
	return Destructs++;
}

DWORD WINAPI ThreadTest( void* Parameter )
{
	int* i = ( int* )Parameter;
	for( int j = 0; j < 8192; ++j )
	{
		for( int k = 0; k < 8192; ++k )
		{
			(*i)++;
		}
	}

	return 0;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	Unused( hInstance );
	Unused( hPrevInstance );
	Unused( lpCmdLine );
	Unused( nCmdShow );

	Allocator::GetDefault().Initialize( 65536 );

	Console::GetInstance();

	STARTTESTS( core );

	TEST( HashedString( "asdf" ).Equals( HashedString( "asdf" ) ) );
	TEST( !HashedString( "asdf" ).Equals( HashedString( "fdsa" ) ) );
	TEST( HashedString( "" ).IsNull() );
	TEST( HashedString( "" ).Equals( HashedString( ( const char* )NULL ) ) );
	TEST( HashedString( "" ).Equals( HashedString( ( unsigned long )NULL ) ) );
	TEST( HashedString( "" ).Equals( HashedString::NullString ) );

	List<int> IntList;
	List<int>::Iterator IntIter;

	IntList.PushBack( 2 );
	IntList.PushBack( 3 );
	IntList.PushFront( 1 );
	TEST( IntList.Size() == 3 );
	IntList.Clear();
	IntList.Clear();

	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PopFront();
	IntList.PopFront();
	IntList.PopFront();
	IntList.PopFront();

	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PopBack();
	IntList.PopBack();
	IntList.PopBack();
	IntList.PopBack();

	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntIter = IntList.Front();
	IntList.Pop( IntIter );	// Pop head by iterator
	IntList.Pop( IntIter );	// Pop head by iterator
	IntList.Pop( IntIter );	// Pop head by iterator

	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntIter = IntList.Front();
	++IntIter;
	IntList.Pop( IntIter ); // Pop a middle node by iterator

	IntList.PushBack( 1 );
	IntIter = IntList.Back();
	IntList.Pop( IntIter ); // Pop tail by iterator
	IntIter = IntList.Back();
	IntList.Pop( IntIter ); // Pop tail by iterator
	IntIter = IntList.Back();
	IntList.Pop( IntIter ); // Pop tail by iterator

	IntList.PushBack( 1 );
	IntList.PushBack( 2 );
	IntList.PushBack( 3 );
	IntList.Remove( 1 );	// Remove head
	TEST( IntList.Size() == 2 );
	IntList.PushFront( 1 );
	IntList.Remove( 3 );	// Remove tail
	TEST( IntList.Size() == 2 );
	IntList.PushBack( 3 );
	IntList.Remove( 2 );	// Remove middle
	TEST( IntList.Size() == 2 );
	IntList.Clear();
	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.PushBack( 1 );
	IntList.Remove( 1 );	// Remove all
	TEST( IntList.Empty() );

	IntList.PushBack( 1 );
	IntList.PushBack( 2 );
	IntList.PushBack( 3 );
	IntList.PushBack( 4 );

	int Count = 0;
	FOR_EACH_LIST( IntIter, IntList, int )
	{
		Count++;
		TEST( *IntIter == Count );
		*IntIter = -1;
		TEST( *IntIter == -1 );
	}
	TEST( Count == 4 );
	IntList.Clear();

	IntList.PushFront( 5 );
	IntList.PushFront( 25 );
	IntList.PushFront( 14 );
	IntList.PushFront( 25 );
	IntList.PushFront( 109 );
	IntList.PushFront( 4 );
	IntList.PushFront( 19 );
	IntList.PushFront( 3 );
	IntList.PushFront( 5 );
	IntList.Sort();
	int Prev = -1;
	FOR_EACH_LIST( IntIter, IntList, int )
	{
		TEST( *IntIter >= Prev );
		Prev = *IntIter;
	}
	FOR_EACH_LIST_REVERSE( IntIter, IntList, int )
	{
		TEST( *IntIter <= Prev );
		Prev = *IntIter;
	}
	IntList.Clear();

	Array<int> IntArray;
	TEST( IntArray.Size() == 0 );
	TEST( IntArray.Empty() );
	IntArray.Resize( 0 );
	TEST( IntArray.Size() == 0 && IntArray.CheckCapacity() == 0 );
	IntArray.Resize( 100 );
	TEST( IntArray.Size() == 100 && IntArray.CheckCapacity() == 128 );
	IntArray.Resize( 64 );
	TEST( IntArray.Size() == 64 && IntArray.CheckCapacity() == 64 );
	IntArray.Resize( 50 );
	TEST( IntArray.Size() == 50 && IntArray.CheckCapacity() == 64 );
	IntArray.Resize( 0 );
	TEST( IntArray.Size() == 0 && IntArray.CheckCapacity() == 0 );
	IntArray.PushBack( 5 );
	IntArray.PushBack( 10 );
	IntArray[0] = 7;
	TEST( IntArray.Size() == 2 );
	TEST( !IntArray.Empty() );
	IntArray.Reserve( 100 );
	IntArray.Reserve( 0 );
	IntArray.PopBack();
	IntArray.PopBack();
	IntArray.PopBack();
	IntArray.Clear();
	IntArray.PushBack( 0 );
	IntArray.PushBack( 1 );
	IntArray.PushBack( 2 );
	IntArray.PushBack( 3 );
	IntArray.PushBack( 4 );
	IntArray.Remove( 1 );
	TEST( IntArray[1] == 2 );
	IntArray.FastRemove( 1 );
	TEST( IntArray[1] == 4 );
	IntArray.Clear();
	IntArray.PushBack( 2 );
	IntArray.PushBack( 4 );
	IntArray.PushBack( 0 );
	IntArray.PushBack( 3 );
	IntArray.PushBack( 6 );
	IntArray.PushBack( 5 );
	IntArray.PushBack( 1 );
	IntArray.QuickSort();
	for( int i = 0; i < (int)IntArray.Size(); ++i )
	{
		TEST( IntArray[i] == i );
	}
	IntArray.Clear();

	IntArray.PushBack( 1 );
	IntArray.PushBackUnique( 1 );
	IntArray.PushBackUnique( 2 );
	IntArray.PushBackUnique( 2 );
	IntArray.PushBackUnique( 3 );
	IntArray.PushBack( 3 );
	TEST( IntArray.Size() == 4 );
	IntArray.Clear();

	IntArray.Insert( 5, 1 );		// Should fail because 1 is an invalid index
	TEST( IntArray.Size() == 0 );
	IntArray.Insert( 4, 0 );		// Should succeed (internally same as PushBack)
	TEST( IntArray.Size() == 1 && IntArray[0] == 4 );
	IntArray.Insert( 3, 0 );		// Should succeed
	TEST( IntArray.Size() == 2 && IntArray[0] == 3 && IntArray[1] == 4 );
	IntArray.Clear();

	Stack<int> IntStack;
	TEST( IntStack.Size() == 0 );
	TEST( IntStack.Empty() );
	IntStack.Push( 5 );
	IntStack.Push( 10 );
	TEST( IntStack.Size() == 2 );
	TEST( !IntStack.Empty() );
	TEST( IntStack.Top() == 10 );
	IntStack.Pop();
	IntStack.Pop();
	IntStack.Pop();
	IntStack.Clear();

	ArrayStack<int> IntArrayStack;
	TEST( IntArrayStack.Size() == 0 );
	TEST( IntArrayStack.Empty() );
	IntArrayStack.Push( 5 );
	IntArrayStack.Push( 10 );
	TEST( IntArrayStack.Size() == 2 );
	TEST( !IntArrayStack.Empty() );
	TEST( IntArrayStack.Top() == 10 );
	IntArrayStack.Pop();
	IntArrayStack.Pop();
	IntArrayStack.Pop();
	IntArrayStack.Clear();

	{	// Scoped so that hash table is freed after use
		HashTable<int> IntTable;
		HashTable<int>::Iterator IntTableIter;
		IntTable.Insert( HashedString( "Ten" ), 10 );
		IntTable.Insert( "Twenty", 20 );
		IntTableIter = IntTable.Search( HashedString( "Twenty" ) );
		TEST( *IntTableIter == 20 );
		IntTableIter = IntTable.Search( "Ten" );
		TEST( *IntTableIter == 10 );
		TEST( IntTable.Size() == 2 );
		TEST( !IntTable.Empty() );
		IntTableIter = IntTable.Search( "Nope!" );
		//*IntTableIter;	// Can't dereference, will crash (TODO: Make this more robust?)
		IntTable.Clear();
		IntTable.Insert( HashedString( "Ten" ), 10 );
		IntTable.Insert( "Twenty", 20 );
		IntTable.Remove( "Ten" );
		IntTable.Remove( "Ten" );
		IntTable.Remove( "Twenty" );
		IntTable.Remove( "Twenty" );
	}

	Map<int, int> IntMap;
	IntMap.Insert( 4, 8 );
	IntMap.Insert( 2, 4 );
	IntMap.Insert( 1, 2 );
	IntMap.Insert( 3, 6 );
	IntMap.Insert( 6, 12 );
	IntMap.Insert( 5, 10 );
	IntMap.Insert( 7, 14 );

	TEST( IntMap.Size() == 7 );

	for( int i = 1; i < 8; ++i )
	{
		TEST( *IntMap.Search( i ) == i * 2 );
	}

	FOR_EACH_MAP( IntMapIter, IntMap, int, int )
	{
		printf( "%d\n", *IntMapIter );
		TEST( *IntMapIter );
	}

	for( Map<int, int>::Iterator IntMapIter = IntMap.Last(); IntMapIter != IntMap.End(); --IntMapIter )
	{
		printf( "%d\n", *IntMapIter );
		TEST( *IntMapIter );
	}

	IntMap.Remove( 1 );
	IntMap.Remove( 2 );
	IntMap.Remove( 3 );
	IntMap.Remove( 4 );
	IntMap.Remove( 5 );
	IntMap.Remove( 6 );
	IntMap.Remove( 7 );

	TEST( IntMap.Size() == 0 );

	IntMap.Insert( 1, 2 );
	IntMap.Insert( 2, 4 );
	IntMap.Insert( 3, 6 );
	IntMap.Insert( 4, 8 );
	IntMap.Insert( 5, 10 );
	IntMap.Insert( 6, 12 );
	IntMap.Insert( 7, 14 );
	IntMap[8] = 16;

	IntMap.Clear();

	TEST( IntMap.Size() == 0 );

	IntMap.Insert( 5, 5 );
	IntMap.Insert( 5, 6 );	// This DOES do something now.
	TEST( IntMap.Size() == 1 );
	TEST( 6 == IntMap[5] );
	IntMap.Clear();

	// This will cause a crash, but I'm not sure that's a problem.
	// It looks decent to remove something that an iterator is pointing at,
	// but what the the node is getting removed somewhere far from this loop?
	// How is this iterator supposed to be updated?
	//IntMap[1] = 1;
	//FOR_EACH_MAP( IntIter, IntMap, int, int )
	//{
	//	IntMap.Remove( *IntIter );
	//}

	// Testing for a crash that occurred in Map...
	for( int i = 0; i < 1000; ++i )
	{
		IntMap.Insert( i, i );
	}
	for( int i = 0; i < 1000; ++i )
	{
		IntMap.Remove( i );
	}

	Multimap<int, int> IntMultimap;
	IntMultimap.Insert( 1, 10 );
	IntMultimap.Insert( 2, 20 );
	IntMultimap.Insert( 3, 30 );
	IntMultimap.Insert( 3, 31 );
	IntMultimap.Insert( 4, 40 );
	IntMultimap.Insert( 5, 50 );
	IntMultimap[6] = 61;
	IntMultimap[6] = 60;	// Overwrites the existing 6 entry
	TEST( IntMultimap.Size() == 7 );
	TEST( IntMultimap[6] == 60 );

	int NumIters = 0;
	FOR_EACH_MULTIMAP( IMMIter, IntMultimap, int, int )
	{
		++NumIters;
	}
	TEST( NumIters == 7 );
	NumIters = 0;
	FOR_EACH_MULTIMAP_REVERSE( IMMIter, IntMultimap, int, int )
	{
		++NumIters;
	}
	TEST( NumIters == 7 );

	Multimap<int, int>::Iterator MultiIter;
	TEST( MultiIter.IsNull() );
	MultiIter = IntMultimap.Search( 3, MultiIter );
	TEST( !MultiIter.IsNull() );
	MultiIter = IntMultimap.Search( 3, MultiIter );
	TEST( !MultiIter.IsNull() );
	MultiIter = IntMultimap.Search( 3, MultiIter );
	TEST( MultiIter.IsNull() );
	MultiIter = IntMultimap.Search( 3, MultiIter );
	TEST( !MultiIter.IsNull() );
	Multimap<int, int>::Iterator MultimapIter = IntMultimap.Search( 4 );
	IntMultimap.Remove( MultimapIter );
	IntMultimap.Remove( 2 );
	IntMultimap.RemoveAll( 3 );
	TEST( IntMultimap.Size() == 3 );
	IntMultimap.Clear();
	TEST( IntMultimap.Size() == 0 );

	Set<int> IntSet;
	IntSet.Insert( 1 );
	IntSet.Insert( 2 );
	IntSet.Insert( 3 );
	IntSet.Insert( 4 );
	IntSet.Insert( 5 );
	TEST( IntSet.Size() == 5 );
	Set<int>::Iterator SetIter = IntSet.Search( 5 );
	TEST( *SetIter == 5 );
	SetIter = IntSet.Search( 6 );
	TEST( SetIter.IsNull() );
	IntSet.Clear();
	TEST( IntSet.Size() == 0 );

	PRINTF( "Variadic macros ftw! %d %s %f\n", 5, "asdf", 3.5f );
	PRINTF( "asdf\n" );
	SETPRINTLEVEL( PRINTLEVEL_Info );
	LEVELPRINTF( PRINTLEVEL_Normal, "0\n" );
	LEVELPRINTF( PRINTLEVEL_Info, "1\n" );
	LEVELPRINTF( PRINTLEVEL_Spam, "2\n" );	// Shouldn't be printed
	DEBUGLEVELPRINTF( PRINTLEVEL_Info, "debug!\n" );

	ConfigManager::SetBool( "MyBool", true );
	ConfigManager::SetInt( "MyInt", 1 );
	ConfigManager::SetFloat( "MyFloat", 1.0f );
	ConfigManager::SetString( "MyString", "asdf" );

	TEST( ConfigManager::GetBool( "MyBool" ) == true );
	//TEST( ConfigManager::GetBool( "MyInt" ) == false );		// Mismatched types assert now
	TEST( ConfigManager::GetInt( "MyInt" ) == 1 );
	//TEST( ConfigManager::GetInt( "MyFloat" ) == 0 );
	TEST( ConfigManager::GetFloat( "MyFloat" ) == 1.0f );
	//TEST( ConfigManager::GetFloat( "MyString" ) == 0.0f );
	TEST( 0 == strcmp( ConfigManager::GetString( "MyString" ), "asdf" ) );
	//TEST( ConfigManager::GetString( "MyBool" ) == NULL );

	bool TestBool = false;
	int TestInt = 0;
	float TestFloat = 0.0f;
	const char* TestString = NULL;

	ConfigManager::Bind( &TestBool, "MyBool" );
	ConfigManager::Bind( &TestInt, "MyInt" );
	ConfigManager::Bind( &TestFloat, "MyFloat" );
	ConfigManager::Bind( &TestString, "MyString" );

	TEST( TestBool == true );
	TEST( TestInt == 1 );
	TEST( TestFloat == 1.0f );
	TEST( 0 == strcmp( TestString, "asdf" ) );

	ConfigManager::SetBool( "MyBool", false );
	ConfigManager::SetInt( "MyInt", 0 );
	ConfigManager::SetFloat( "MyFloat", 0.0f );
	ConfigManager::SetString( "MyString", "fdsa" );

	TEST( TestBool == false );
	TEST( TestInt == 0 );
	TEST( TestFloat == 0.0f );
	TEST( 0 == strcmp( TestString, "fdsa" ) );

	ConfigManager::Unbind( &TestBool );
	ConfigManager::Unbind( &TestInt );
	ConfigManager::Unbind( &TestFloat );
	ConfigManager::Unbind( &TestString );

	float NewFloat = 1.0f;
	ConfigManager::Bind( &NewFloat, "NewFloat" );	// Binding overrides the existing value even if the key didn't exist yet
	TEST( NewFloat == 0.0f );
	ConfigManager::Unbind( &NewFloat );

	// Test deleting from maps
	//for( uint i = 5; i > 0; --i )
	//{
	//	Map< uint, uint > DeleteIntMap;
	//	for( uint j = 15; j > 0; --j )
	//	{
	//		DeleteIntMap[j] = j;
	//	}
	//	//FOR_EACH_MAP( DeleteIntIter, DeleteIntMap, uint, uint )
	//	for( Map< uint, uint >::Iterator DeleteIntIter = DeleteIntMap.Begin(); DeleteIntIter != DeleteIntMap.End(); )
	//	{
	//		uint Key = DeleteIntIter.GetKey();
	//		if( Key % i == 0 )
	//		{
	//			PRINTF( "%i: Deleted\n", Key );
	//			DeleteIntMap.Remove( DeleteIntIter );
	//		}
	//		else
	//		{
	//			PRINTF( "%i: Passed\n", Key );
	//			++DeleteIntIter;
	//		}
	//	}
	//}

	class TestClass
	{
	public:
		~TestClass() { CountDestructs(); }
		int Filler;
	};

	TestClass* TestClassArray = new TestClass[ 100 ];
	delete[] TestClassArray;
	TEST( CountDestructs() == 100 );

	// Tiny parser tests
	ConfigManager::LoadTiny( "Context:Bool=True Context:Int=1 Context:Float=1.0f Context:String=\"foo\"" );
	TEST( ConfigManager::GetBool( "Bool", false, "Context" ) == true );
	TEST( ConfigManager::GetInt( "Int", 0, "Context" ) == 1 );
	TEST( ConfigManager::GetFloat( "Float", 0.0f, "Context" ) == 1.0f );
	TEST( SimpleString( ConfigManager::GetString( "String", "", "Context" ) ) == "foo" );

	// Script conditional tests
	TEST( StringManager::ResolveAndEvaluateConditional( "False != True" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "0 == 0" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "1.0f < 2.0f" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "1 == 1.0f" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "2.0f == 2" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "\"asdf\" == \"asdf\"" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "\"asdf\" ~= \"ASDF\"" ) );
	TEST( StringManager::ResolveAndEvaluateConditional( "\"asdf\" != \"foo\"" ) );

	// Threading tests
	Thread* Threads[4];
	int Results[4];
	for( int i = 0; i < 4; ++i )
	{
		Results[i] = 0;
		Threads[i] = new Thread( ThreadTest, &Results[i] );
	}
	for( int i = 0; i < 4; ++i )
	{
		Threads[i]->Wait();
		delete Threads[i];
	}
	TEST( Results[0] == 8192*8192 );
	TEST( Results[1] == 8192*8192 );
	TEST( Results[2] == 8192*8192 );
	TEST( Results[3] == 8192*8192 );

	Results[0] = 0;
	Threads[0] = new Thread( ThreadTest, &Results[0] );

	while( !Threads[0]->IsDone() )
	{
	}
	delete Threads[0];

	// HTTP tests
	{
		HTTPSocket::SSocketInit SocketInit;
		SocketInit.m_CloseConnection = true;
		SocketInit.m_ContentType = "text/plain";
		SocketInit.m_HostName = "www.dphrygian.com";
		SocketInit.m_Path = "/cgi-bin/scores.cgi";

		HTTPSocket Socket;
		Socket.AsyncSetReceiveBufferSize( 1024 * 1024 );
		if( Socket.Connect( SocketInit ) )
		{
			Array< char > Received;
			Socket.Get( Received, "CoreUnitTest" );
			PRINTF( Received.GetData() );
		}

		{
			SocketInit.m_Async = true;
			Socket.Connect( SocketInit );

			while( Socket.AsyncIsWaiting() )
			{
				PRINTF( "." );
			}
			PRINTF( "\n" );

			if( Socket.AsyncConnectSucceeded() )
			{
				Array< char > Received;
				Socket.Get( Received, "CoreUnitTest" );	// Having to give it a Received buffer even tho it's async and the buffer won't be filled is lame

				while( Socket.AsyncIsWaiting() )
				{
					PRINTF( "." );
				}
				PRINTF( "\n" );

				Received = Socket.AsyncGetReceived();

				PRINTF( Received.GetData() );
			}
		}
	}

	int NumFailed = TESTSFAILED;
	printf( "%d tests failed\n", NumFailed );
	ENDTESTS;

	Console::DeleteInstance();
	PrintManager::DeleteInstance();		// This is lame--I have to manually free these even
	StringManager::DeleteInstance();	// though I never explicitly create them. Bad pattern.
	ConfigManager::DeleteInstance();

	Allocator::GetDefault().Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Write ) );
	Allocator::GetDefault().ShutDown();

	DEBUGASSERT( _CrtCheckMemory() );
	DEBUGASSERT( !_CrtDumpMemoryLeaks() );

	return NumFailed;
}
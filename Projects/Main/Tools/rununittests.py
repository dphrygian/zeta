import os

UNITTESTS = '../UnitTests/'
CORE = 'CoreUnitTest.exe'
MATH = 'MathUnitTest.exe'

totalNumFailed = 0

#-----------------------------------------------------
def unittest( testname ):
	global totalNumFailed
	numFailed = os.spawnl( os.P_WAIT, testname, testname )
	totalNumFailed += numFailed
	print '%s: %d' % ( testname, numFailed )
	return numFailed

#-----------------------------------------------------
print 'Running unit tests...'

os.chdir( UNITTESTS )

unittest( CORE )
unittest( MATH )

print 'Total: %d' % totalNumFailed

if totalNumFailed > 0:
	sys.exit(1)
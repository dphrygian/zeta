# Prepass parser for streamlining config files
# Transforms:
# Loom script -> token stream
# Token stream -> parse tree
# Parse tree -> pseudo-Workbench objects
# Workbench objects -> Config file

import sys

arglookup = {
	# HACKHACK for Rules parsed as PEs (for WaitForEvent)
	'Rule':							[ 'Event' ],

	# PEs
	'Add':							[ 'InputA', 'InputB' ],
	'Sub':							[ 'InputA', 'InputB' ],
	'Mul':							[ 'InputA', 'InputB' ],
	'Div':							[ 'InputA', 'InputB' ],
	'Neg':							[ 'Input' ],
	'NOT':							[ 'Input' ],
	'AND':							[ 'InputA', 'InputB' ],
	'OR':							[ 'InputA', 'InputB' ],
	'XOR':							[ 'InputA', 'InputB' ],
	'Dot':							[ 'InputA', 'InputB' ],
	'Cross':						[ 'InputA', 'InputB' ],
	'Normal':						[ 'Input' ],
	'Length':						[ 'Input' ],
	'Saturate':						[ 'Input' ],
	'ConstantBool':					[ 'Value' ],
	'ConstantInt':					[ 'Value' ],
	'ConstantFloat':				[ 'Value' ],
	'RandomBool':					[ 'Probability' ],
	'RandomInt':					[ 'ValueA', 'ValueB' ],
	'RandomFloat':					[ 'ValueA', 'ValueB' ],
	'ConstantString':				[ 'Value' ],
	'ConstantVector':				[ 'ValueX', 'ValueY', 'ValueZ' ],
	'ConstantAngles':				[ 'ValuePitch', 'ValueRoll', 'ValueYaw' ],
	'GetName':						[ 'Input', 'UniqueName' ],
	'Lookup':						[ 'Key' ],									# NOTE: Lookup is also the name of a BT node!!
	'Conditional':					[ 'Op', 'InputA', 'InputB' ],
	'GetConfigVar':					[ 'VarContext', 'VarName' ],
	'RodinBlackboardGet':			[ 'BlackboardKey' ],
	'RodinGetKnowledge':			[ 'Entity', 'Key' ],
	'RosaDistance':					[ 'InputA', 'InputB' ],
	'RosaGetLocation':				[ 'Entity' ],
	'RosaGetOrientation':			[ 'Entity' ],
	'RosaGetVelocity':				[ 'Entity' ],
	'RosaGetBoneLocation':			[ 'Entity', 'BoneName' ],
	'RosaDeadReckoning':			[ 'Location', 'Speed', 'TargetLocation', 'TargetVelocity' ],
	'RosaGetPersistentVar':			[ 'Key' ],
	'RosaIsConvoState':				[ 'EntityPE', 'State' ],
	'RosaHasKeycard':				[ 'Keycard' ],
	'RosaHasMoney':					[ 'Amount' ],
	'RosaHasAmmo':					[ 'Type', 'Count' ],
	'RosaGetItem':					[ 'EntityPE', 'SlotPE' ],
	'RosaIsObjectiveComplete':		[ 'Objective', 'RejectFail' ],
	'RosaGetCharacterVO':			[ 'EntityPE', 'VO' ],
	'RosaGetSurfaceProperty':		[ 'SurfaceParameter', 'SurfaceProperty' ],
	'RosaGetSpeedLimit':			[ 'Entity' ],
	'RosaIsInsideDoor':				[ 'Entity', 'Door' ],
	'RosaHeadshot':					[ 'EntityPE', 'BonePE' ],
	'RosaGetActor':					[ 'Role' ],
	'RosaGetNumActors':				[ 'Role' ],
	'RosaIsActorRels':				[ 'ActorIDAPE', 'ActorIDBPE', 'Rels', 'Commutative' ],
	'RosaIsActorProp':				[ 'ActorIDPE', 'Prop' ],
	'RosaGetThreatLevel':			[ 'Neighborhood' ],
	'RosaIsDifficulty':				[ 'RangeLo', 'RangeHi' ],
	'RosaCampaignModify':			[ 'Name', 'Input', 'Override', 'Force' ],
	'GetEntityByLabel':				[ 'Label' ],
	'GetVariable':					[ 'EntityPE', 'VariableName' ],
	'QueryActionStack':				[ 'Key' ],
	'PushContext':					[ 'EntityPE', 'Input' ],
	'StatMod':						[ 'EntityPE', 'StatName', 'Input' ],
	'GetState':						[ 'EntityPE' ],
	'IsState':						[ 'EntityPE', 'State' ],

	# Actions
	'Log':							[ 'Text' ],
	'Destroy':						[ 'DestroyPE' ],
	'SendEvent':					[ 'EventName', 'Recipient' ],				# NOTE: SendEvent is also the name of a BT node!!
	'UnqueueEvent':					[ 'EventOwner', 'VariableMapTag' ],
	'UIShowHideWidget':				[ 'Screen', 'Widget', 'Hidden' ],
	'RodinBlackboardWrite':			[ 'BlackboardKey', 'ValuePE' ],
	'RosaCheckCone':				[ 'EntityPE', 'ConeAngle', 'ConeLength', 'CheckTag', 'TraceCenter' ],
	'RosaCheckLine':				[ 'EntityPE', 'LineLength', 'CheckTag' ],
	'RosaCheckSphere':				[ 'EntityPE', 'Radius', 'CheckTag', 'TraceCenter' ],
	'RosaSpawnEntity':				[ 'Entity' ],
	'RosaSpawnDecal':				[ 'Entity', 'LocationPE', 'OrientationPE', 'NormalBasisPE' ],
	'RosaTeleportTo':				[ 'EntityPE', 'DestinationPE', 'SetOrientation' ],
	'RosaPlayAnim':					[ 'Animation', 'Loop' ],					# NOTE: RosaPlayAnim is also the name of a BT node!!
	'RosaPlayHandAnim':				[ 'Animation' ],
	'RosaPlaySound':				[ 'Sound' ],
	'RosaPlayBark':					[ 'SoundPE', 'Category' ],					# NOTE: RosaPlayBark is also the name of a BT node with the same properties!!
	'RosaGoToLevel':				[ 'Level' ],
	'RosaLoadGame':					[ 'SaveSlot' ],
	'RosaSetPersistentVar':			[ 'Key', 'ValuePE' ],
	'RosaGiveItem':					[ 'Item', 'GiveTo' ],
	'RosaShowBook':					[ 'BookString', 'IsDynamic' ],
	'RosaConditionalShowBook':		[ 'BookString', 'IsDynamic', 'PersistenceKey' ],
	'RosaSetTexture':				[ 'Texture' ],
	'RosaSetLightColor':			[ 'ColorH', 'ColorS', 'ColorV' ],
	'RosaAddKeycard':				[ 'Keycard' ],
	'RosaRemoveKeycard':			[ 'Keycard' ],
	'RosaAddObjective':				[ 'Objective' ],
	'RosaCompleteObjective':		[ 'Objective', 'Fail', 'ForceAdd' ],
	'RosaProgressConversation':		[ ],
	'RosaPublishResults':			[ ],
	'RosaSelectConversationChoice':	[ 'ChoiceIndex' ],
	'RosaAddAmmo':					[ 'Type', 'Count' ],
	'RosaRemoveAmmo':				[ 'Type', 'Count' ],
	'RosaIncomingCall':				[ 'Caller', 'Convo' ],
	'RosaStartConversation':		[ 'Convo' ],
	'RosaSetConvoState':			[ 'EntityPE', 'State' ],
	'RosaAddMoney':					[ 'Amount', 'ShowLogMessage' ],
	'RosaRemoveMoney':				[ 'Amount' ],
	'RosaAwardAchievement':			[ 'AchievementTag' ],
	'RosaSetStat':					[ 'StatTag', 'Value' ],
	'RosaIncrementStat':			[ 'StatTag', 'Amount' ],
	'RosaLogMessage':				[ 'String', 'IsDynamic' ],
	'RosaAddActorRels':				[ 'ActorIDAPE', 'ActorIDBPE', 'Rels', 'Commutative' ],
	'RosaRemoveActorRels':			[ 'ActorIDAPE', 'ActorIDBPE', 'Rels', 'Commutative' ],
	'RosaAddActorProp':				[ 'ActorIDPE', 'Prop' ],
	'RosaRemoveActorProp':			[ 'ActorIDPE', 'Prop' ],
	'RosaLaunchWebSite':			[ 'URL' ],
	'SetVariable':					[ 'EntityPE', 'VariableName', 'ValuePE' ],
	'SetConfigVar':					[ 'VarContext', 'VarName', 'ValuePE' ],
	'TriggerStatMod':				[ 'StatModEvent', 'Trigger' ],
	'SetState':						[ 'EntityPE', 'State' ],

	# BT nodes
	'UseResource':					[ 'Resource', 'ForceClaim' ],
	'Wait':							[ 'TimePE' ],
	'WaitForEvent':					[ 'Rule' ],
	'CastResult':					[ 'ValuePE' ],
	'Timeout':						[ 'TimeoutPE' ],
	'ConditionTimeout':				[ 'TimeoutPE' ],
	'ConditionPE':					[ 'ValuePE' ],
	'UseStatMod':					[ 'StatModEvent' ],
	'BlackboardWrite':				[ 'BlackboardKey', 'ValuePE' ],
	'RosaLookAt':					[ 'LookTargetBlackboardKey' ],
	'RosaMoveTo':					[ 'MoveTargetBlackboardKey', 'Stance' ],
	'RosaTurnToward':				[ 'TurnTargetBlackboardKey' ],
}

# HACKHACK: Global state for plot point roles
loomplotpointroles = []

loomautotag = ''
loomautonumber = -1
output = []
leadingwhitespace = ''

class LoomParseArgNode:
	def __init__( self ):
		self.parent = None	# either a LoomParseNode or a parent LoomParseArgNode
		self.lhs = None		# LHS should just be a string literal, but contained in a LoomParseNode; or None if the name is implicit
		self.rhs = None		# LoomParseNode

	def addchild( self, node ):
		self.rhs = node

class LoomParseNode:
	def __init__( self ):
		self.name = ''
		self.ref = False
		self.parent = None	# LoomParseNode
		self.args = []		# array of LoomParseArgNodes
		self.items = []		# array of LoomParseNodes

	def addchild( self, node ):
		self.items.append( node )

	def getarg( self, argname ):
		for arg in self.args:
			if arg.lhs and arg.lhs.name == argname:
				return arg.rhs
		return None

class LoomChoice:
	def __init__( self ):
		self.line = ''
		self.dynamic = ''
		self.convo = ''
		self.convope = None
		self.shownpe = None
		self.hiddenpe = None
		self.enabledpe = None
		self.disabledpe = None

class LoomLine:
	def __init__( self ):
		self.speaker = ''
		self.line = ''
		self.dynamic = ''

class LoomPE:
	def __init__( self ):
		self.name = ''
		self.ref = False
		self.type = ''
		self.args = []

class LoomPEPair:
	def __init__( self ):
		self.key = ''
		self.value = None

class LoomSelectionPE:
	def __init__( self ):
		self.condition = None
		self.value = None

class LoomSelectionAction:
	def __init__( self ):
		self.condition = None
		self.action = None

class LoomStatMod:
	def __init__( self ):
		self.name = ''
		self.event = ''
		self.stat = ''
		self.function = ''
		self.value = ''

class LoomUpgrade:
	def __init__( self ):
		self.tag = ''
		self.statmodevent = ''
		self.track = ''
		self.cost = ''
		self.icon = ''
		self.prereq = ''
		self.postevent = ''

class LoomFactionCon:
	def __init__( self ):
		self.a = ''
		self.b = ''
		self.c = ''

class LoomPlotPointRole:
	def __init__( self ):
		self.name = ''
		self.record = False

class LoomAction:
	def __init__( self ):
		self.name = ''
		self.ref = False
		self.type = ''
		self.args = []

class LoomRule:
	def __init__( self ):
		self.name = ''
		self.ref = False
		self.event = ''
		self.args = []
		# TODO: Criteria
		# HACKHACK: For now, only supporting condition PEs (which can be nested or references)
		self.conditions = []

class LoomReaction:
	def __init__( self ):
		self.name = ''
		self.ref = False
		self.rule = None
		self.actions = []

class LoomBTNode:
	def __init__( self ):
		self.name = ''
		self.ref = False
		self.type = ''
		self.children = []
		self.args = []

class LoomArrayEntry:
	def __init__( self ):
		self.args = []

class LoomVector:
	def __init__( self ):
		self.x = ''
		self.y = ''
		self.z = ''
		self.w = ''



# ********************************************************************************
# Tokenization

# Returns true if we should continue tokenizing after this
def tryaddtoken( loomtoken, loomtokens ):
	global loomplotpointroles

	strippedtoken = loomtoken.lstrip()

	if strippedtoken.startswith( '//' ):
		return False

	# HACKHACK: Insert role tokens
	elif strippedtoken.startswith( '%Role' ):
		index = int( strippedtoken[5:] )
		loomtokens.append( loomplotpointroles[ index ].name )

	elif strippedtoken.startswith( '%VGA' ):
		index = int( strippedtoken[4:] )
		loomtokens.append( 'RosaGetActor' )
		loomtokens.append( '(' )
		loomtokens.append( loomplotpointroles[ index ].name )
		loomtokens.append( ')' )

	elif strippedtoken.startswith( '%CS' ):
		index = int( strippedtoken[3:] )
		loomtokens.append( 'ConstantString' )
		loomtokens.append( '(' )
		loomtokens.append( loomplotpointroles[ index ].name )
		loomtokens.append( ')' )

	elif len( strippedtoken ) > 0:
		loomtokens.append( strippedtoken )

	return True

def loomtokenizeline( loomline ):
	loomtokens = []

	loomtoken = ''
	for c in loomline:
		if c == '{' or c == '}' or c == '(' or c == ')' or c == ',' or c == '=':
			# Close previous token, if any
			if not tryaddtoken( loomtoken, loomtokens ):
				return loomtokens
			loomtoken = ''
			# Add this separating character as its own token
			loomtokens.append( c )

		elif c == ' ' or c == '\n' or c == '\r' or c == '\t':
			# Close previous token, if any
			if not tryaddtoken( loomtoken, loomtokens ):
				return loomtokens
			loomtoken = ''

		else:
			loomtoken += c

	# Add open token, if any
	tryaddtoken( loomtoken, loomtokens )

	return loomtokens

def loomtokenize( loomline, loomiter ):
	loomtokens = []
	while True:
		try:
			# Tokenize the current line...
			loomtokens.extend( loomtokenizeline( loomline ) )

			# Stop when we close the primary braces
			if loomtokens.count('{') > 0 and loomtokens.count('{') == loomtokens.count('}'):
				break

			# ...THEN advance the iterator.
			loomline = loomiter.next()

		except StopIteration:
			break

	return loomtokens



# ********************************************************************************
# Parse tree

def loomparseargnode( tokeniter, parentnode ):
	node = LoomParseArgNode()

	if parentnode:
		parentnode.args.append( node )
		node.parent = parentnode

	currentnode = node

	while True:
		token = loomparsenode( tokeniter, node, True )

		if token == ')':
			return

		elif token == '=':
			currentnode.lhs = currentnode.rhs
			currentnode.rhs = None

		elif token == ',':
			node = LoomParseArgNode()
		
			if parentnode:
				parentnode.args.append( node )
				node.parent = parentnode
		
			currentnode = node
		elif token == None:
			break

	return node

def loommakenode( nodename, parentnode ):
	node = LoomParseNode()

	if parentnode:
		parentnode.addchild( node )
		node.parent = parentnode

	node.name = nodename
	if node.name.startswith( '"' ):
		node.ref = True

	return node

def loomparsenode( tokeniter, parentnode, isarg = False ):
	currentnode = None

	while True:
		try:
			token = tokeniter.next()

			if token == '(':
				# HACKHACK: If we don't have a current node, make an anonymous one and then close it
				anonymousnode = False
				if currentnode == None:
					anonymousnode = True
					currentnode = loommakenode( '', parentnode )

				loomparseargnode( tokeniter, currentnode )

				if anonymousnode:
					currentnode = None

			elif token == ')':
				# We're closing an args block
				return token

			elif token == '=':
				# We're moving to the RHS of an arg
				return token

			elif token == ',':
				# We're either moving on to the next arg in an arg block,
				# or we're just moving on to the next node in the current level.
				if isarg:
					return token
				else:
					pass

			elif token == '{':
				loomparsenode( tokeniter, currentnode )

			elif token == '}':
				# We're closing an item block
				return

			else:
				# We're adding a node
				currentnode = loommakenode( token, parentnode )

		except StopIteration:
			break

	return currentnode

def loomprintdepth( depth ):
	for i in range( depth ):
		sys.stdout.write( ' ' )

def loomprintparsearg( loomparseargnode, depth ):
	if loomparseargnode.lhs:
		loomprintdepth( depth )
		print 'Arg: {0}'.format( loomparseargnode.lhs.name )
		loomprintparsenode( loomparseargnode.rhs, depth, 'Value' )
	elif loomparseargnode.rhs:
		loomprintdepth( depth )
		print 'Arg: <implicit>'
		loomprintparsenode( loomparseargnode.rhs, depth, 'Value' )

def loomprintparsenode( loomparsenode, depth, prefix ):
	loomprintdepth( depth )
	print '{0}: {1} ({2})'.format( prefix, loomparsenode.name, loomparsenode.ref )

	for arg in loomparsenode.args:
		loomprintparsearg( arg, depth + 1 )

	for item in loomparsenode.items:
		loomprintparsenode( item, depth + 1, 'Node' )

def loomprintparsetree( loomparsetree ):
	loomprintparsenode( loomparsetree, 0, 'Node' )



# ********************************************************************************
# Workbench objects

def getargname( arg, index, actiontype ):
	if arg.lhs:
		return arg.lhs.name
	elif actiontype == '':
		# HACKHACK: Allow anonymous args for anonymous nodes, used for generic arrays
		return '';
	else:
		# Implicit name, look up by action name and argument index
		try:
			return arglookup[ actiontype ][ index ]
		except:
			return 'Argument lookup error: {0} {1}'.format( actiontype, index )

def loomemitpe( rhsnode ):
	pe = LoomPE()
	pe.ref = rhsnode.ref or len( rhsnode.args ) == 0	# Literal strings or numbers won't have an arg block and should be treated the same as refs

	if pe.ref:
		pe.name = rhsnode.name
	else:
		pe.name = loommakename( 'PE' )
		pe.type = rhsnode.name

		for i in range( len( rhsnode.args ) ):
			arg = rhsnode.args[i]

			# Ignore empty arguments
			if not arg.rhs:
				continue

			argname = getargname( arg, i, pe.type )
			argvalue = loomemitpe( arg.rhs )
			pe.args.append( [ argname, argvalue ] )
			#print 'pe.args[ "{0}" ] = {1}'.format( argname, argvalue.name )

	return pe
	
def loomemitaction( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	action = LoomAction()
	action.ref = node.ref

	if action.ref:
		action.name = node.name
	else:
		action.name = loommakename( 'Action' )
		action.type = node.name

		for i in range( len( node.args ) ):
			arg = node.args[i]

			# Ignore empty arguments
			if not arg.rhs:
				continue

			argname = getargname( arg, i, action.type )
			argvalue = loomemitpe( arg.rhs )
			action.args.append( [ argname, argvalue ] )
			#print 'action.args[ "{0}" ] = {1}'.format( argname, argvalue.name )

	return action

def loomemitrule( rhsnode ):
	#print 'Emitting {0} with {1} args and {2} items'.format( rhsnode.name, len( rhsnode.args ), len( rhsnode.items ) )

	rule = LoomRule()
	rule.ref = rhsnode.ref

	if rule.ref:
		rule.name = rhsnode.name
		rule.event = rhsnode.name
	else:
		if len( rhsnode.args ) > 0:
			rule.name = loommakename( 'Rule' )
			rule.event = rhsnode.args[0].rhs.name	# HACKHACK: Assuming Event is the first or only arg in the Rule

		for i in range( 1, len( rhsnode.args ) ):
			arg = rhsnode.args[i]

			# Ignore empty arguments
			if not arg.rhs:
				continue

			if arg.lhs:
				# This is where stuff like Additive should go
				argname = arg.lhs.name # I can skip getargname() since I already have a lhs
				argvalue = loomemitpe( arg.rhs )
				rule.args.append( [ argname, argvalue ] )
			else:
				# Empty arguments are assumed to be conditionals
				# TODO: Maybe someday care about Criteria but I doubt it
				rule.conditions.append( loomemitpe( rhsnode.args[i].rhs ) )

	return rule

def loomemitreaction( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	reaction = LoomReaction()
	reaction.ref = node.ref

	if reaction.ref:
		reaction.name = node.name
	else:
		reaction.name = loommakename( 'Reaction' )

		if len( node.args ) > 0:
			reaction.rule = loomemitrule( node.args[0].rhs )

		for action in node.items:
			reaction.actions.append( loomemitaction( action ) )

	return reaction

def loomemitreactions( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	reactions = []

	if root.name != 'lmReactions':
		print 'Something went wrong!'

	for node in root.items:
		reactions.append( loomemitreaction( node ) )

	return reactions

def loomemitstatmod( node ):
	statmod = LoomStatMod()

	statmod.name = loommakename( 'StatMod' )
	# HACKHACK: Assuming order and number of arguments
	statmod.event		= node.args[0].rhs.name
	statmod.stat		= node.args[1].rhs.name
	statmod.function	= node.args[2].rhs.name
	statmod.value		= node.args[3].rhs.name

	return statmod

def loomemitstatmods( root ):
	statmods = []

	if root.name != 'lmStatMods':
		print 'Something went wrong!'

	for node in root.items:
		statmods.append( loomemitstatmod( node ) )

	return statmods

def loomemitupgrade( node ):
	upgrade = LoomUpgrade()

	upgrade.tag = node.args[0].rhs.name
	upgrade.statmodevent = node.args[1].rhs.name
	upgrade.track = node.args[2].rhs.name
	upgrade.cost = node.args[3].rhs.name
	upgrade.icon = node.args[4].rhs.name
	if len( node.args ) > 5:
		upgrade.prereq = node.args[5].rhs.name
	if len( node.args ) > 6:
		upgrade.postevent = node.args[6].rhs.name

	return upgrade

def loomemitupgrades( root ):
	upgrades = []

	if root.name != 'lmUpgrades':
		print 'Something went wrong!'

	for node in root.items:
		upgrades.append( loomemitupgrade( node ) )

	return upgrades

def loomemitfactioncon( node ):
	factioncon = LoomFactionCon()

	factioncon.a = node.args[0].rhs.name
	factioncon.b = node.args[1].rhs.name
	factioncon.c = node.args[2].rhs.name

	return factioncon

def loomemitfactioncons( root ):
	factioncons = []

	if root.name != 'lmFactionCons':
		print 'Something went wrong!'

	for node in root.items:
		factioncons.append( loomemitfactioncon( node ) )

	return factioncons

def loomemitplotpointrole( node ):
	plotpointrole = LoomPlotPointRole()

	plotpointrole.name = node.args[0].rhs.name

	if len( node.args ) > 1:
		plotpointrole.record = node.args[1].rhs.name.lower().startswith( 't' )
	else:
		plotpointrole.record = True

	return plotpointrole

def loomemitplotpointroles( root ):
	plotpointroles = []

	if root.name != 'lmPlotPoint':
		print 'Something went wrong!'

	for node in root.items:
		plotpointroles.append( loomemitplotpointrole( node ) )

	return plotpointroles

def loomemitactions( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	actions = []

	if root.name != 'lmActions':
		print 'Something went wrong!'

	for node in root.items:
		actions.append( loomemitaction( node ) )

	return actions

def loomemitpepair( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	pepair = LoomPEPair()

	if len( node.args ) > 1:
		pepair.key = node.args[0].rhs.name
		pepair.value = loomemitpe( node.args[1].rhs )

	return pepair

def loomemitpemap( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	pepairs = []

	if root.name != 'lmPEMap':
		print 'Something went wrong!'

	for node in root.items:
		pepairs.append( loomemitpepair( node ) )

	return pepairs

def loomemitselectionpe( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	selection = LoomSelectionPE()

	if len( node.args ) > 1:
		selection.condition = loomemitpe( node.args[0].rhs )
		selection.value = loomemitpe( node.args[1].rhs )

	return selection

def loomemitselectionaction( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	selection = LoomSelectionAction()

	if len( node.args ) > 1:
		selection.condition = loomemitpe( node.args[0].rhs )
		selection.action = loomemitaction( node.args[1].rhs )

	return selection

def loomemitselectorpe( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	selections = []

	if root.name != 'lmSelectorPE':
		print 'Something went wrong!'

	for node in root.items:
		selections.append( loomemitselectionpe( node ) )

	return selections

def loomemitselectoraction( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	selections = []

	if root.name != 'lmSelectorAction':
		print 'Something went wrong!'

	for node in root.items:
		selections.append( loomemitselectionaction( node ) )

	return selections

def loomemitarray( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	array = []

	if root.name != 'lmArray':
		print 'Something went wrong!'

	for node in root.items:
		array.append( loomemitarrayentry( node ) )

	return array

def loomemitarrayentry( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	arrayentry = LoomArrayEntry()

	for i in range( len( node.args ) ):
		arg = node.args[i]

		# Ignore empty arguments
		if not arg.rhs:
			continue

		argname = getargname( arg, i, '' )
		argvalue = loomemitpe( arg.rhs )
		arrayentry.args.append( [ argname, argvalue ] )
		#print 'arrayentry.args[ "{0}" ] = {1}'.format( argname, argvalue.name )

	return arrayentry

def loomemitvector( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	vector = LoomVector()

	vector.x		= root.items[0].name
	vector.y		= root.items[1].name
	vector.z		= root.items[2].name
	if len( root.items ) > 3:
		vector.w	= root.items[3].name

	return vector

def loomemitline( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	line = LoomLine()

	line.speaker = node.args[0].rhs.name
	line.line = node.args[1].rhs.name
	if len( node.args ) > 2:
		line.dynamic = node.args[2].rhs.name

	return line

def loomemitlines( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	lines = []

	if root.name != 'lmLines':
		print 'Something went wrong!'

	for node in root.items:
		lines.append( loomemitline( node ) )

	return lines

def loomemitchoice( node ):
	#print 'Emitting {0} with {1} args and {2} items'.format( node.name, len( node.args ), len( node.items ) )

	choice = LoomChoice()

	choice.line = node.args[0].rhs.name
	if node.getarg( 'IsDynamic' ):
		choice.dynamic = node.getarg( 'IsDynamic' ).name
	if node.getarg( 'Convo' ):
		choice.convo = node.getarg( 'Convo' ).name
	if node.getarg( 'ConvoPE' ):
		choice.convope = loomemitpe( node.getarg( 'ConvoPE' ) )
	if node.getarg( 'ShownPE' ):
		choice.shownpe = loomemitpe( node.getarg( 'ShownPE' ) )
	if node.getarg( 'HiddenPE' ):
		choice.hiddenpe = loomemitpe( node.getarg( 'HiddenPE' ) )
	if node.getarg( 'EnabledPE' ):
		choice.enabledpe = loomemitpe( node.getarg( 'EnabledPE' ) )
	if node.getarg( 'DisabledPE' ):
		choice.disabledpe = loomemitpe( node.getarg( 'DisabledPE' ) )

	return choice

def loomemitchoices( root ):
	#print 'Emitting {0} with {1} args and {2} items'.format( root.name, len( root.args ), len( root.items ) )

	choices = []

	if root.name != 'lmChoices':
		print 'Something went wrong!'

	for node in root.items:
		choices.append( loomemitchoice( node ) )

	return choices

def loomemitbtnode( node ):
	btnode = LoomBTNode()
	btnode.ref = node.ref

	if btnode.ref:
		btnode.name = node.name
	else:
		# Since these names may be visible for AI debugging purposes, make the name using the type instead of just 'BTNode'
		btnode.name = loommakename( node.name )
		btnode.type = node.name

		for i in range( len( node.args ) ):
			arg = node.args[i]

			# Ignore empty arguments
			if not arg.rhs:
				continue

			argname = getargname( arg, i, btnode.type )
			argvalue = loomemitpe( arg.rhs )
			btnode.args.append( [ argname, argvalue ] )

		for child in node.items:
			btnode.children.append( loomemitbtnode( child ) )

	return btnode

def loomemitbt( root ):
	if root.name != 'lmBT':
		print 'Something went wrong!'

	# Because BTs (and sub-BTs) should be named for lookup mapping,
	# this assumes we have one and only one item in the root node,
	# and it will populate a named section in the .loom script.
	return loomemitbtnode( root.items[0] )



# ********************************************************************************
# Directives

def printoutput( outputstring, appendleadingwhitespace = True ):
	global leadingwhitespace
	global output
	if appendleadingwhitespace:
		output.append( leadingwhitespace )
	output.append( outputstring )

def loomoutputpe( pe, outputcontext = True ):
	# HACKHACK: If this pe is actually a rule, convert and marshal it to the correct function
	if pe.type == 'Rule':
		rule = LoomRule()
		rule.name = pe.name
		rule.ref = pe.ref
		# Assume the first arg is just the Event name
		rule.event = pe.args[0][1].name
		for i in range( 1, len( pe.args ) ):
			rule.conditions.append( pe.args[i][1] )
		loomoutputrule( rule )
		return

	if outputcontext:
		printoutput( '\n' )
		printoutput( '[+{0}]\n'.format( pe.name ) )
	printoutput( 'PEType = "{0}"\n'.format( pe.type ) )

	for argname, argvalue in pe.args:
		if argvalue.ref:
			printoutput( '{0} = {1}\n'.format( argname, argvalue.name ) )
		else:
			printoutput( '{0} = "{1}"\n'.format( argname, argvalue.name ) )

	if outputcontext:
		printoutput( '[-]\n' )

	for argname, argvalue in pe.args:
		if argvalue.ref:
			continue
		loomoutputpe( argvalue )

def loomoutputrule( rule ):
	printoutput( '\n' )
	printoutput( '[+{0}]\n'.format( rule.name ) )
	printoutput( 'Event = {0}\n'.format( rule.event ) )

	for argname, argvalue in rule.args:
		if argvalue.ref:
			printoutput( '{0} = {1}\n'.format( argname, argvalue.name ) )
		else:
			printoutput( '{0} = "{1}"\n'.format( argname, argvalue.name ) )

	if len( rule.conditions ) > 0:
		printoutput( 'NumConditions = &\n' )
		printoutput( '@ Condition\n' )

	for condition in rule.conditions:
		if condition.ref:
			printoutput( '@@& = {0}\n'.format( condition.name ) )
		else:
			printoutput( '@@& = "{0}"\n'.format( condition.name ) )

	printoutput( '[-]\n' )

	for condition in rule.conditions:
		if condition.ref:
			continue
		loomoutputpe( condition )

def loomoutputaction( action, outputcontext = True ):
	if outputcontext:
		printoutput( '\n' )
		printoutput( '[+{0}]\n'.format( action.name ) )
	printoutput( 'ActionType = "{0}"\n'.format( action.type ) )

	# HACKHACK: For SendEvent actions, interpret arguments as a NumParameters list
	if action.type == 'SendEvent':
		loomoutputsendeventargs( action )
	else:
		loomoutputactionargs( action )

	if outputcontext:
		printoutput( '[-]\n' )	

	for argname, argvalue in action.args:
		if argvalue.ref:
			continue
		loomoutputpe( argvalue )

def loomoutputarrayentrypes( arrayentry ):
	for argname, argvalue in arrayentry.args:
		if argvalue.ref:
			continue
		loomoutputpe( argvalue )

sendeventnames = [
	'EventName',
	'Recipient',
	'LogEvent',
	'DispatchDelay',
	'DispatchDelayPE',
	'DispatchTicks',
	'QueueEvent',
	'EventOwner',
	'RecipientLabel',
	'VariableMapTag',
]

def loomoutputsendeventargs( action ):
	trueargs = {}
	paramargs = {}

	# Divvy up the args which are proper SendEvent members and which are event parameters
	for argname, argvalue in action.args:
		if argname in sendeventnames:
			trueargs[ argname ] = argvalue
		else:
			paramargs[ argname ] = argvalue

	# First, emit SendEvent members
	for argname, argvalue in trueargs.iteritems():
		if argvalue.ref:
			printoutput( '{0} = {1}\n'.format( argname, argvalue.name ) )
		else:
			printoutput( '{0} = "{1}"\n'.format( argname, argvalue.name ) )

	# Then emit event parameters, if any
	if len( paramargs ) > 0:
		printoutput( 'NumParameters = &\n' )
		printoutput( '@ Parameter\n' )
		for argname, argvalue in paramargs.iteritems():
			printoutput( '@@&Name = "{0}"\n'.format( argname ) )
			if argvalue.ref:
				printoutput( '@@^Value = {0}\n'.format( argvalue.name ) )
			else:
				printoutput( '@@^Value = "{0}"\n'.format( argvalue.name ) )

def loomoutputactionargs( action ):
	for argname, argvalue in action.args:
		if argvalue.ref:
			printoutput( '{0} = {1}\n'.format( argname, argvalue.name ) )
		else:
			printoutput( '{0} = "{1}"\n'.format( argname, argvalue.name ) )

def loomoutputarrayentryargs( arrayentry ):
	firstindex = True
	for argname, argvalue in arrayentry.args:
		fronttoken = '@@&' if firstindex else '@@^'
		firstindex = False;
		if argvalue.ref:
			printoutput( '{0}{1} = {2}\n'.format( fronttoken, argname, argvalue.name ) )
		else:
			printoutput( '{0}{1} = "{2}"\n'.format( fronttoken, argname, argvalue.name ) )

def loomparsereaction( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	reaction = loomemitreaction( loomparsetree )

	if reaction and not reaction.ref:
		loomoutputreaction( reaction, False )

def loomoutputreaction( reaction, outputcontext = True ):
	if outputcontext:
		printoutput( '\n' )
		printoutput( '[+{0}]\n'.format( reaction.name ) )

	# Print the rule belonging to the reaction
	rule = reaction.rule
	if rule.ref:
		printoutput( 'Rule = {0}\n'.format( rule.name ) )
	else:
		printoutput( 'Rule = "{0}"\n'.format( rule.name ) )

	# Print the actions belonging to the reaction
	if len( reaction.actions ) > 0:
		printoutput( 'NumActions = &\n' )
		printoutput( '@ Action\n' )
		for action in reaction.actions:
			if action.ref:
				printoutput( '@@& = {0}\n'.format( action.name ) )
			else:
				printoutput( '@@& = "{0}"\n'.format( action.name ) )

	if outputcontext:
		printoutput( '[-]\n' )

	# Print the rule definition (ignore if it is a literal reference)
	if rule.ref:
		pass
	else:
		loomoutputrule( rule )

	# Print the action definitions and associated PEs (ignore if they are literal references)
	if len( reaction.actions ) > 0:
		for action in reaction.actions:
			if action.ref:
				continue

			loomoutputaction( action )

def loomparsereactions( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	reactions = loomemitreactions( loomparsetree )

	if len( reactions ) > 0:
		# First, print all the reactions belonging to the component
		printoutput( 'NumReactions = &\n' )
		printoutput( '@ Reaction\n' )
		for reaction in reactions:
			if reaction.ref:
				printoutput( '@@& = {0}\n'.format( reaction.name ) )
			else:
				printoutput( '@@& = "{0}"\n'.format( reaction.name ) )

		# Then print the reaction definitions themselves, with associated rules and actions following
		for reaction in reactions:
			if reaction.ref:
				# Ignore reactions referenced by literal
				continue

			loomoutputreaction( reaction )

# Hacked up version of loomparsereactions, to insert an action list anywhere, like in a convo
def loomparseactions( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	actions = loomemitactions( loomparsetree )

	actionsarrayname = loomparsetree.args[0].rhs.name if len( loomparsetree.args ) > 0 else ''

	if len( actions ) > 0:
		printoutput( 'Num{0}Actions = &\n'.format( actionsarrayname ) )
		printoutput( '@ {0}Action\n'.format( actionsarrayname ) )
		for action in actions:
			if action.ref:
				printoutput( '@@& = {0}\n'.format( action.name ) )
			else:
				printoutput( '@@& = "{0}"\n'.format( action.name ) )

		for action in actions:
			if action.ref:
				continue

			loomoutputaction( action )

# Hacked up version of loomparseactions, to insert a single action definition in its named context
def loomparseaction( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )

	if len( loomparsetree.items ) > 0:
		action = loomemitaction( loomparsetree.items[0] )

		if action and not action.ref:
			loomoutputaction( action, False )

def loomparsepe( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )

	if len( loomparsetree.items ) > 0:
		pe = loomemitpe( loomparsetree.items[0] )

		if pe and not pe.ref:
			loomoutputpe( pe, False )

def loomparsepemap( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	pepairs = loomemitpemap( loomparsetree )

	if len( pepairs ) > 0:
		printoutput( 'NumPEs = &\n' )
		printoutput( '@ PE\n' )

		for pepair in pepairs:
			printoutput( '@@&Key = {0}\n'.format( pepair.key ) )
			if pepair.value.ref:
				printoutput( '@@^Def = {0}\n'.format( pepair.value.name ) )
			else:
				printoutput( '@@^Def = "{0}"\n'.format( pepair.value.name ) )

		for pepair in pepairs:
			if pepair.value.ref:
				continue
			loomoutputpe( pepair.value )

def loomparsestatmods( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	statmods = loomemitstatmods( loomparsetree )

	if len( statmods ) > 0:
		printoutput( 'NumModifiers = &\n' )
		printoutput( '@ Modifier\n' )
		for statmod in statmods:
			printoutput( '@@& = "{0}"\n'.format( statmod.name ) )

		for statmod in statmods:
			printoutput( '\n' )
			printoutput( '[+{0}]\n'.format( statmod.name ) )
			printoutput( 'Event = {0}\n'.format( statmod.event ) )
			printoutput( 'Stat = {0}\n'.format( statmod.stat ) )
			printoutput( 'Function = {0}\n'.format( statmod.function ) )
			printoutput( 'Value = {0}\n'.format( statmod.value ) )
			printoutput( '[-]\n' )

def loomparseupgrades( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	upgrades = loomemitupgrades( loomparsetree )

	if len( upgrades ) > 0:
		printoutput( 'NumUpgrades = &\n' )
		printoutput( '@ Upgrade\n' )
		for upgrade in upgrades:
			printoutput( '@@& = {0}\n'.format( upgrade.tag ) )
			printoutput( '@@^StatMod = {0}\n'.format( upgrade.statmodevent ) )
			printoutput( '@@^Track = {0}\n'.format( upgrade.track ) )
			printoutput( '@@^Cost = {0}\n'.format( upgrade.cost ) )
			printoutput( '@@^Icon = {0}\n'.format( upgrade.icon ) )
			if upgrade.prereq != '':
				printoutput( '@@^Prereq = {0}\n'.format( upgrade.prereq ) )
			if upgrade.postevent != '':
				printoutput( '@@^PostEvent = {0}\n'.format( upgrade.postevent ) )

def loomparsefactioncons( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	factioncons = loomemitfactioncons( loomparsetree )

	if len( factioncons ) > 0:
		printoutput( 'NumFactionCons = &\n' )
		printoutput( '@ FactionCon\n' )
		for factioncon in factioncons:
			printoutput( '@@&A = {0}\n'.format( factioncon.a ) )
			printoutput( '@@^B = {0}\n'.format( factioncon.b ) )
			printoutput( '@@^C = {0}\n'.format( factioncon.c ) )

def loomparseplotpoint( loomline, loomiter ):
	global loomplotpointroles

	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	plotpointroles = loomemitplotpointroles( loomparsetree )

	# Copy up to global state for access by other blocks
	loomplotpointroles = plotpointroles

	if len( plotpointroles ) > 0:
		printoutput( 'NumRoles = &\n' )
		printoutput( '@ Role\n' )
		for plotpointrole in plotpointroles:
			if plotpointrole.record:
				printoutput( '@@& = {0}\n'.format( plotpointrole.name ) )

def loomparselines( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	lines = loomemitlines( loomparsetree )

	if len( lines ) > 0:
		printoutput( 'NumLines = &\n' )
		printoutput( '@ Line\n' )
		for line in lines:
			printoutput( '@@&Speaker = {0}\n'.format( line.speaker ) )
			printoutput( '@@^ = {0}\n'.format( line.line ) )
			if line.dynamic != '':
				printoutput( '@@^IsDynamic = {0}\n'.format( line.dynamic ) )

def loomparsechoices( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	choices = loomemitchoices( loomparsetree )

	if len( choices ) > 0:
		printoutput( 'NumChoices = &\n' )
		printoutput( '@ Choice\n' )
		for choice in choices:
			printoutput( '@@& = {0}\n'.format( choice.line ) )
			if choice.dynamic != '':
				printoutput( '@@^IsDynamic = {0}\n'.format( choice.dynamic ) )
			if choice.convo != '':
				printoutput( '@@^Convo = {0}\n'.format( choice.convo ) )
			if choice.convope:
				if choice.convope.ref:
					printoutput( '@@^ConvoPE = {0}\n'.format( choice.convope.name ) )
				else:
					printoutput( '@@^ConvoPE = "{0}"\n'.format( choice.convope.name ) )
			if choice.shownpe:
				if choice.shownpe.ref:
					printoutput( '@@^ShownPE = {0}\n'.format( choice.shownpe.name ) )
				else:
					printoutput( '@@^ShownPE = "{0}"\n'.format( choice.shownpe.name ) )
			if choice.hiddenpe:
				if choice.hiddenpe.ref:
					printoutput( '@@^HiddenPE = {0}\n'.format( choice.hiddenpe.name ) )
				else:
					printoutput( '@@^HiddenPE = "{0}"\n'.format( choice.hiddenpe.name ) )
			if choice.enabledpe:
				if choice.enabledpe.ref:
					printoutput( '@@^EnabledPE = {0}\n'.format( choice.enabledpe.name ) )
				else:
					printoutput( '@@^EnabledPE = "{0}"\n'.format( choice.enabledpe.name ) )
			if choice.disabledpe:
				if choice.disabledpe.ref:
					printoutput( '@@^DisabledPE = {0}\n'.format( choice.disabledpe.name ) )
				else:
					printoutput( '@@^DisabledPE = "{0}"\n'.format( choice.disabledpe.name ) )

		for choice in choices:
			if choice.convope and not choice.convope.ref:
				loomoutputpe( choice.convope )
			if choice.shownpe and not choice.shownpe.ref:
				loomoutputpe( choice.shownpe )
			if choice.hiddenpe and not choice.hiddenpe.ref:
				loomoutputpe( choice.hiddenpe )
			if choice.enabledpe and not choice.enabledpe.ref:
				loomoutputpe( choice.enabledpe )
			if choice.disabledpe and not choice.disabledpe.ref:
				loomoutputpe( choice.disabledpe )

def loommakename( nametype ):
	global loomautotag
	global loomautonumber
	loomautonumber += 1
	loomautoname = 'lmAuto_{0}_{1}_{2}'.format( loomautotag, nametype, loomautonumber )
	return loomautoname

def loomoutputbtnodeargs( btnode, outputcontext = True ):
	if outputcontext:
		printoutput( '\n' )
		printoutput( '[+{0}]\n'.format( btnode.name ) )

	printoutput( 'NodeType = "{0}"\n'.format( btnode.type ) )

	# Output argument names
	for argname, argvalue in btnode.args:
		if argvalue.ref:
			printoutput( '{0} = {1}\n'.format( argname, argvalue.name ) )
		else:
			printoutput( '{0} = "{1}"\n'.format( argname, argvalue.name ) )

	# Output child names
	# (In case this is a decorator, output Child as well as NumChildren/Child0...)
	if len( btnode.children ) > 0:
		printoutput( 'NumChildren = &\n' )
		printoutput( '@ Child\n' )
		for child in btnode.children:
			if child.ref:
				printoutput( '@@& = {0}\n'.format( child.name ) )
			else:
				printoutput( '@@& = "{0}"\n'.format( child.name ) )
	if len( btnode.children ) == 1:
		child = btnode.children[0]
		if child.ref:
			printoutput( 'Child = {0}\n'.format( child.name ) )
		else:
			printoutput( 'Child = "{0}"\n'.format( child.name ) )

	if outputcontext:
		printoutput( '[-]\n' )

	# Output argument PEs
	for argname, argvalue in btnode.args:
		if argvalue.ref:
			continue
		loomoutputpe( argvalue )

	# Output children (recursive)
	for child in btnode.children:
		if child.ref:
			continue
		loomoutputbtnodeargs( child )

def loomparsebt( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	btroot = loomemitbt( loomparsetree )
	loomoutputbtnodeargs( btroot, False )

def loomparseselectorpe( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	selections = loomemitselectorpe( loomparsetree )

	printoutput( 'PEType = "Selector"\n' )

	if len( selections ) > 0:
		printoutput( 'NumSelections = &\n' )
		printoutput( '@ Selection\n' )

		for selection in selections:
			if selection.condition.ref:
				printoutput( '@@&Condition = {0}\n'.format( selection.condition.name ) )
			else:
				printoutput( '@@&Condition = "{0}"\n'.format( selection.condition.name ) )
			if selection.value.ref:
				printoutput( '@@^Value = {0}\n'.format( selection.value.name ) )
			else:
				printoutput( '@@^Value = "{0}"\n'.format( selection.value.name ) )

		for selection in selections:
			if not selection.condition.ref:
				loomoutputpe( selection.condition )
			if not selection.value.ref:
				loomoutputpe( selection.value )

def loomparseselectoraction( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	selections = loomemitselectoraction( loomparsetree )

	printoutput( 'ActionType = "Selector"\n' )

	if len( selections ) > 0:
		printoutput( 'NumSelections = &\n' )
		printoutput( '@ Selection\n' )

		for selection in selections:
			if selection.condition.ref:
				printoutput( '@@&Condition = {0}\n'.format( selection.condition.name ) )
			else:
				printoutput( '@@&Condition = "{0}"\n'.format( selection.condition.name ) )
			if selection.action.ref:
				printoutput( '@@^Action = {0}\n'.format( selection.action.name ) )
			else:
				printoutput( '@@^Action = "{0}"\n'.format( selection.action.name ) )

		for selection in selections:
			if not selection.condition.ref:
				loomoutputpe( selection.condition )
			if not selection.action.ref:
				loomoutputaction( selection.action )

def loomparsearray( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )
	array = loomemitarray( loomparsetree )
	
	arrayname = loomparsetree.args[0].rhs.name if len( loomparsetree.args ) > 0 else ''
	arraypluralname = loomparsetree.args[1].rhs.name if len( loomparsetree.args ) > 1 else 'Num' + arrayname + 's'

	if len( array ) > 0:
		printoutput( '{0} = &\n'.format( arraypluralname ) )
		printoutput( '@ {0}\n'.format( arrayname ) )
		for arrayentry in array:
			loomoutputarrayentryargs( arrayentry )

		for arrayentry in array:
			loomoutputarrayentrypes( arrayentry )

def loomparsecolor( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )

	color = loomemitvector( loomparsetree )

	colorname = loomparsetree.args[0].rhs.name if len( loomparsetree.args ) > 0 else 'Color'
	elementX = 'R'
	elementY = 'G'
	elementZ = 'B'
	elementW = 'A'

	if 'HSV' in loomparsetree.name:
		elementX = 'H'
		elementY = 'S'
		elementZ = 'V'	
	elif 'HDSV' in loomparsetree.name:
		elementX = 'HD'
		elementY = 'S'
		elementZ = 'V'

	printoutput( '{0}{1} = {2}\n'.format( colorname, elementX, color.x ) )
	printoutput( '{0}{1} = {2}\n'.format( colorname, elementY, color.y ) )
	printoutput( '{0}{1} = {2}\n'.format( colorname, elementZ, color.z ) )
	if 'A' in loomparsetree.name or len( loomparsetree.items ) > 3:
		printoutput( '{0}{1} = {2}\n'.format( colorname, elementW, color.w ) )

def loomparsevector( loomline, loomiter ):
	loomtokens = loomtokenize( loomline, loomiter )
	loomparsetree = loomparsenode( iter( loomtokens ), None )

	vector = loomemitvector( loomparsetree )

	vectorname = loomparsetree.args[0].rhs.name if len( loomparsetree.args ) > 0 else ''

	printoutput( '{0}{1} = {2}\n'.format( vectorname, 'X', vector.x ) )
	printoutput( '{0}{1} = {2}\n'.format( vectorname, 'Y', vector.y ) )
	printoutput( '{0}{1} = {2}\n'.format( vectorname, 'Z', vector.z ) )
	if len( loomparsetree.items ) > 3:
		printoutput( '{0}{1} = {2}\n'.format( vectorname, 'W', vector.w ) )

def loomparse( loomiter ):
	global leadingwhitespace
	while True:
		try:
			loomline = loomiter.next()
			strippedline = loomline.lstrip()
			leadingwhitespace = loomline[:-len(strippedline)]
			if strippedline.startswith( 'lmReactions' ):
				loomparsereactions( loomline, loomiter )
			elif strippedline.startswith( 'lmReaction' ):
				loomparsereaction( loomline, loomiter )
			elif strippedline.startswith( 'lmActions' ):
				loomparseactions( loomline, loomiter )
			elif strippedline.startswith( 'lmAction' ):
				loomparseaction( loomline, loomiter )
			elif strippedline.startswith( 'lmPEMap' ):
				loomparsepemap( loomline, loomiter )
			elif strippedline.startswith( 'lmPE' ):
				loomparsepe( loomline, loomiter )
			elif strippedline.startswith( 'lmLines' ):
				loomparselines( loomline, loomiter )
			elif strippedline.startswith( 'lmChoices' ):
				loomparsechoices( loomline, loomiter )
			elif strippedline.startswith( 'lmBT' ):
				loomparsebt( loomline, loomiter )
			elif strippedline.startswith( 'lmSelectorPE' ):
				loomparseselectorpe( loomline, loomiter )
			elif strippedline.startswith( 'lmSelectorAction' ):
				loomparseselectoraction( loomline, loomiter )
			elif strippedline.startswith( 'lmStatMods' ):
				loomparsestatmods( loomline, loomiter )
			elif strippedline.startswith( 'lmUpgrades' ):
				loomparseupgrades( loomline, loomiter )
			elif strippedline.startswith( 'lmFactionCons' ):
				loomparsefactioncons( loomline, loomiter )
			elif strippedline.startswith( 'lmPlotPoint' ):
				loomparseplotpoint( loomline, loomiter )
			elif strippedline.startswith( 'lmArray' ):
				loomparsearray( loomline, loomiter )
			elif strippedline.startswith( 'lmColor' ):	# Expected: lmColorRGB(A), lmColorHSV(A), or lmColorHDSV(A)
				loomparsecolor( loomline, loomiter )
			elif strippedline.startswith( 'lmVector' ):
				loomparsevector( loomline, loomiter )
			else:
				# Pass through unmodified config file line
				printoutput( loomline, False )
		except StopIteration:
			break

def loommain( infilename, outfilename ):
	global output

	loomfile = open( infilename, 'r' )
	loomlines = []
	for line in loomfile:
		loomlines.append( line )
	loomfile.close()

	global loomautotag
	loomautotag = str( hex( hash( infilename ) & 0xffffffff ) )
	loomparse( iter( loomlines ) )

	configfile = open( outfilename, 'w' )
	for line in output:
		configfile.write( line )
	configfile.close()

def main():
	if len( sys.argv ) > 2:
		infilename = sys.argv[1]
		outfilename = sys.argv[2]
		loommain( infilename, outfilename )
	else:
		print "Arguments: python loom.py [infile] [outfile]"

if __name__ == "__main__":
	main()

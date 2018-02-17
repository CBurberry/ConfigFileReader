#include "ConfigFileReader.h"


//Reset all member variables, empties data containers.
void ConfigFileReader::Reset()
{
	ResetFloatConverter();
	ResetLineReader();
	
	if ( m_cIFile.is_open() )
	{
		m_cIFile.close();
	}
	
	memset( m_caLine, 0, MAX_LINE_LENGTH );
	m_strLine			= "";
	m_strVariableName	= "";
	m_strValue			= "";

	m_eReaderLoadState = EState_Undefined;

	//Uninstanced / Globals data containers.
	m_cUninstancedFloatVariables	= {};
	m_cUninstancedVectorsVariables	= {};

	//Spawn parameter containers
	m_cPatrolEnemySpawns		= {};
	m_cPursueEnemySpawns		= {};
	m_cStaticPlatformSpawns		= {};
	m_cMovingPlatformSpawns		= {};
	m_cBouncyPlatformSpawns		= {};
	m_cCrumblingPlatformSpawns	= {};
	m_cHazardSpawns				= {};
}

//Reset the string to float stringstream
void ConfigFileReader::ResetFloatConverter()
{
	static std::locale uselocale( "" );
	ssFloatCoverter.imbue( uselocale );
	ssFloatCoverter.str( "" );
	ssFloatCoverter.clear();
}

void ConfigFileReader::ResetLineReader() 
{
	//Booleans for line reader status tracking
	m_bIsValue2DVector		= false;
	m_bIsFirstCharacter		= true;
	m_bSkipUntilEqualSign	= false;
	m_bVariableNameFound	= false;
	m_bValueFound			= false;
	m_bEndSymbolEncountered = false;

	//Input string variables
	m_strVariableName	= "";
	m_strValue			= "";
}


//Setup the input file for reading, setup mode and reset variables.
void ConfigFileReader::Init( std::string strFilename, EInputState EState_InputMode ) 
{
	Reset();
	m_cIFile.open(strFilename);

	//If stream is in bad state, exit.
	if ( !m_cIFile )
	{
		return;
	}

	switch ( EState_InputMode )
	{
	case EState_Uninstanced:
		m_eReaderLoadState = EState_Uninstanced;
		ReadVariablesFile();
		break;

	case EState_Spawn:
		m_eReaderLoadState = EState_Spawn;
		ReadSpawningListFile();
		break;

	case EState_Undefined:
	default:
		m_eReaderLoadState = EState_Undefined;
		break;
	}

	m_cIFile.close();
}




//================== File Reading Loopers ===================

//Input loop for reading in variables that don't belong to entity instances.
void ConfigFileReader::ReadVariablesFile() 
{
	ResetLineReader();
	std::pair<std::string, std::string> sOutputValues;

	while ( m_cIFile.getline( m_caLine, MAX_LINE_LENGTH ) )
	{
		if ( m_bEndSymbolEncountered )
		{
			break;
		}
		m_bIsFirstCharacter = true;

		//Reset the string
		m_cIss.clear();
		m_strLine = std::string( m_caLine );
		m_cIss.str( m_strLine );
		m_strVariableName	= "";
		m_strValue			= "";

		sOutputValues = ReadVariableFromLine();

		//if line data isn't empty, print.
		if ( !( IsLineDataEmpty(sOutputValues) ) )
		{
			//std::cout << sOutputValues.first << std::endl << sOutputValues.second << std::endl;
			//Add data to containers.
			ProcessVariableStringIntoData( sOutputValues );
		}

		//Reset boolean line variables after every line.
		ResetLineReader();
	}
}


//Input loop for reading data that belongs to entity instances.
void ConfigFileReader::ReadSpawningListFile() 
{
	bool bStartNewInstanceList		= false;
	bool bBreakForLoop				= false;
	std::string strSpawnCategory	= "";
	ResetLineReader();

	while ( m_cIFile )
	{
		while ( m_cIFile.getline( m_caLine, MAX_LINE_LENGTH ) )
		{
			if ( m_bEndSymbolEncountered )
			{
				break;
			}
			m_bIsFirstCharacter = true;
			bBreakForLoop		= false;

			//Reset the string & stringstream
			m_cIss.clear();
			m_strLine = std::string( m_caLine );
			m_cIss.str( m_strLine );

			for ( char c : m_caLine )
			{
				//Check if the current line can be discarded
				if ( bBreakForLoop )
				{
					break;
				}

				switch ( c )
				{
				//Special character to end read - debug.
				case '!':
					return;
				//ignore the commented line - next loop iteration
				case '#':
					bBreakForLoop = true;
					break;
				//New spawn type encountered
				case '[':
					strSpawnCategory = "";
					m_cIss >> strSpawnCategory;
					m_strCurrentSpawn = strSpawnCategory;
					m_eCurrentSpawn = m_cStringToEntityEnumMap[strSpawnCategory];
					bBreakForLoop = true;
					break;
				//Start a new instance list
				case '@':
					bStartNewInstanceList = true;
					bBreakForLoop = true;
					break;
				default:
					if ( iswspace( c ) != 0 )
					{
						continue;
					}
					else
					{
						bBreakForLoop = true;
						break;
					}
				}
			}

			//Reset boolean line variables after every line.
			ResetLineReader();

			//Break reading inner while loop to start new reading function.
			if ( bStartNewInstanceList )
			{
				break;
			}
		}


		//Read spawn instance data.
		if ( bStartNewInstanceList )
		{
			SetNewInstanceData();
			bStartNewInstanceList = false;
		}
	}
}


//=================== Line-by-Line Reader ====================

//Read the line to find the name and associated value in text file
//Returns pair of two strings <name, value>
std::pair<std::string, std::string> ConfigFileReader::ReadVariableFromLine()
{
	//Reset booleans before reading line characters
	ResetLineReader();

	for ( char c : m_strLine )
	{
		//Special character to end read - debug.
		if ( c == '!' )
		{
			m_bEndSymbolEncountered = true;
			break;
		}

		//Continue looping in the string until '=' met.
		if ( m_bSkipUntilEqualSign )
		{
			if ( c == '=' )
			{
				m_bSkipUntilEqualSign = false;
			}
			continue;
		}

		//iswspace returns non-zero if whitespace characters
		if ( iswspace( c ) != 0 )
		{
			//consume
			continue;
		}
		else if ( c == '#' && m_bIsFirstCharacter )
		{
			//ignore the commented line - next loop iteration
			break;
		}
		else if ( c == 'v' && m_bIsFirstCharacter )
		{
			//This value is a vector value
			m_bIsValue2DVector = true;
		}
		else if ( m_bVariableNameFound == false )
		{
			//store the contiguous string element
			m_cIss >> m_strVariableName;
			//If it's a 2d vector, we skip the v from the name.
			if ( m_bIsValue2DVector )
			{
				m_cIss >> m_strVariableName;
			}

			m_bVariableNameFound = true;
			//Jump to the equals sign
			m_bSkipUntilEqualSign = true;
		}
		else
		{
			//Read the value at this block and store it.
			m_strValue.push_back( c );
			m_bValueFound = true;
		}

		m_bIsFirstCharacter = false;
	}

	return std::pair<std::string, std::string>(m_strVariableName, m_strValue);
}



//============ Data Processing Auxillary Functions ===========

//Take the string pair and process them into the relevant data container.
void ConfigFileReader::ProcessVariableStringIntoData( std::pair<std::string, std::string> cDataPair )
{
	ResetFloatConverter();
	
	//Different data storage for different types of data.
	if ( m_bIsValue2DVector )
	{
		std::string strXValue = "";
		std::string strYValue = "";
		float fXValue = 0.0f;
		float fYValue = 0.0f;
		bool bFirstValueRead = false;

		for ( char c : cDataPair.second )
		{
			switch ( c )
				{
			case '(':
			case ')':
				break;
			case ',':
				bFirstValueRead = true;
				break;
			default:
				if ( iswspace( c ) != 0 )
				{
					continue;
				}

				//Output the characters into the respective strings.
				if ( bFirstValueRead )
				{
					strYValue.push_back( c );
				}
				else
				{
					strXValue.push_back( c );
				}
				break;
			}
		}

		//first value.
		ssFloatCoverter << strXValue;
		ssFloatCoverter >> fXValue;

		//Clear the stringstream so it can be reused.
		ssFloatCoverter.str( "" );
		ssFloatCoverter.clear();

		//second value.
		ssFloatCoverter << strYValue;
		ssFloatCoverter >> fYValue;

		//add pair to map.
		std::pair<float, float> pair_value( fXValue, fYValue );
		if ( m_eReaderLoadState == EState_Uninstanced )
		{
			m_cUninstancedVectorsVariables[cDataPair.first] = pair_value;
		}
		else if ( m_eReaderLoadState == EState_Spawn )
		{
			m_cSpawnVectorVariables.push_back( pair_value );
		}

	}
	else
	{
		float fValue;
		ssFloatCoverter << cDataPair.second;
		ssFloatCoverter >> fValue;
		if ( m_eReaderLoadState == EState_Uninstanced )
		{
			m_cUninstancedFloatVariables[cDataPair.first] = fValue;
		}
		else if ( m_eReaderLoadState == EState_Spawn )
		{
			m_cSpawnFloatVariables.push_back(fValue);
		}
	}
}


//Make a new instance of data for current spawn type.
void ConfigFileReader::SetNewInstanceData() 
{
	//Potential data members
	SEnemyParams sEnemyParams;
	SMovingPlatformParams sMovingPlatformParams;
	SBouncyPlatformParams sBouncyPlatformParams;
	std::pair<float, float> sSpawnPoint;

	//Instantiate a vector of string pairs
	std::vector<std::pair<std::string, std::string>> cStringPairValues;

	//Loop m_eCurrentSpawn length times.
	for ( int iLoop = 0; iLoop < m_eCurrentSpawn; ++iLoop )
	{
		//Get a new line per iteration.
		m_cIFile.getline( m_caLine, MAX_LINE_LENGTH );

		//Reset the string & stringstream
		m_cIss.clear();
		m_strLine = std::string( m_caLine );
		m_cIss.str( m_strLine );

		//Get String Value pair
		std::pair<std::string, std::string> sStringPair = ReadVariableFromLine();

		cStringPairValues.push_back( sStringPair );

		//Process string value pair into container.
		ProcessVariableStringIntoData( sStringPair );

		int iContainerLength = -1;
		//Add data to relevant struct
		if ( m_strCurrentSpawn == "[Patrol_Enemy]" || m_strCurrentSpawn == "[Pursuing_Enemy]" )
		{
			switch ( iLoop )
			{
			case 0:
			{
				iContainerLength = m_cSpawnVectorVariables.size();
				sSpawnPoint = m_cSpawnVectorVariables[iContainerLength - 1];
				m_cSpawnVectorVariables.pop_back();
				sEnemyParams.sSpawnPosition = sSpawnPoint;
			}
				break;
			case 1:
			{
				iContainerLength = m_cSpawnVectorVariables.size();
				std::pair<float, float> sMovementDirection;
				sMovementDirection = m_cSpawnVectorVariables[iContainerLength - 1];
				m_cSpawnVectorVariables.pop_back();
				sEnemyParams.sMovementDirection = sMovementDirection;
			}
				break;
			case 2:
			{
				iContainerLength = m_cSpawnFloatVariables.size();
				float fMaximumMoveDistance;
				fMaximumMoveDistance = m_cSpawnFloatVariables[iContainerLength - 1];
				m_cSpawnFloatVariables.pop_back();
				sEnemyParams.fMaximumMoveDistance = fMaximumMoveDistance;

				//Push value/data into vector.
				if ( m_strCurrentSpawn == "[Patrol_Enemy]" )
				{
					m_cPatrolEnemySpawns.push_back(sEnemyParams);
				}
				else if ( m_strCurrentSpawn == "[Pursuing_Enemy]" )
				{
					m_cPursueEnemySpawns.push_back(sEnemyParams);
				}
			}
				break;
			default:
				//Error
				return;
			}
		}
		else if ( m_strCurrentSpawn == "[Moving_Platforms]" )
		{
			switch ( iLoop )
			{
			case 0:
			{
				iContainerLength = m_cSpawnVectorVariables.size();
				std::pair<float, float> sStartingPosition;
				sStartingPosition = m_cSpawnVectorVariables[iContainerLength - 1];
				m_cSpawnVectorVariables.pop_back();
				sMovingPlatformParams.sStartingPosition = sStartingPosition;
			}
				break;
			case 1:
			{
				iContainerLength = m_cSpawnVectorVariables.size();
				std::pair<float, float> sEndingPosition;
				sEndingPosition = m_cSpawnVectorVariables[iContainerLength - 1];
				m_cSpawnVectorVariables.pop_back();
				sMovingPlatformParams.sEndingPosition = sEndingPosition;
			}
				break;
			case 2:
			{
				iContainerLength = m_cSpawnVectorVariables.size();
				std::pair<float, float> sMovementVelocity;
				sMovementVelocity = m_cSpawnVectorVariables[iContainerLength - 1];
				m_cSpawnVectorVariables.pop_back();
				sMovingPlatformParams.sMovementVelocity = sMovementVelocity;

				//Push value/data into vector.
				m_cMovingPlatformSpawns.push_back(sMovingPlatformParams);
			}
				break;
			default:
				//Error
				break;
			}
		}
		else if ( m_strCurrentSpawn == "[Bouncy_Platforms]" )
		{
			switch ( iLoop )
			{
			case 0:
				iContainerLength = m_cSpawnVectorVariables.size();
				sSpawnPoint = m_cSpawnVectorVariables[iContainerLength - 1];
				m_cSpawnVectorVariables.pop_back();
				sBouncyPlatformParams.sSpawnPosition = sSpawnPoint;
				break;
			case 1:
				iContainerLength = m_cSpawnFloatVariables.size();
				float fPlatformAngle;
				fPlatformAngle = m_cSpawnFloatVariables[iContainerLength - 1];
				m_cSpawnFloatVariables.pop_back();
				sBouncyPlatformParams.fPlatformRotationInDegrees = fPlatformAngle;

				//Push value/data into vector.
				m_cBouncyPlatformSpawns.push_back(sBouncyPlatformParams);
				break;
			default:
				//Error
				break;
			}
		}
		else if ( m_strCurrentSpawn == "[Static_Platforms]"
			|| m_strCurrentSpawn == "[Crumbling_Platforms]" 
			|| m_strCurrentSpawn == "[Hazards]")
		{
			iContainerLength = m_cSpawnVectorVariables.size();
			sSpawnPoint = m_cSpawnVectorVariables[iContainerLength - 1];
			m_cSpawnVectorVariables.pop_back();
			
			//Push value/data into vector.
			if ( m_strCurrentSpawn == "[Static_Platforms]" )
			{
				m_cStaticPlatformSpawns.push_back( sSpawnPoint );
			}
			else if ( m_strCurrentSpawn == "[Crumbling_Platforms]" )
			{
				m_cCrumblingPlatformSpawns.push_back(sSpawnPoint);
			}
			else if ( m_strCurrentSpawn == "[Hazards]" ) 
			{
				m_cHazardSpawns.push_back(sSpawnPoint);
			}
		}
	}	
}


//Check if line read result is empty
bool ConfigFileReader::IsLineDataEmpty( std::pair<std::string, std::string> sOutputValues )
{
	//Run a check on the line read result.
	bool bIsFirstEmpty = sOutputValues.first == "";
	bool bIsSecondEmpty = sOutputValues.second == "";
	return ( bIsFirstEmpty && bIsSecondEmpty );
}


//================ Data container accessors ==================

std::map<std::string, float>					ConfigFileReader::GetUninstancedFloatVariables() 
{
	return m_cUninstancedFloatVariables;
}
std::map<std::string, std::pair<float, float>>	ConfigFileReader::GetUninstancedVectorVariables() 
{
	return m_cUninstancedVectorsVariables;
}
std::vector<SEnemyParams>						ConfigFileReader::GetPatrolEnemySpawns() 
{
	return m_cPatrolEnemySpawns;
}
std::vector<SEnemyParams>						ConfigFileReader::GetPursueEnemySpawns() 
{
	return m_cPursueEnemySpawns;
}
std::vector<std::pair<float, float>>			ConfigFileReader::GetStaticPlatformSpawns() 
{
	return m_cStaticPlatformSpawns;
}
std::vector<SMovingPlatformParams>				ConfigFileReader::GetMovingPlatformSpawns() 
{
	return m_cMovingPlatformSpawns;
}
std::vector<SBouncyPlatformParams>				ConfigFileReader::GetBouncyPlatformSpawns() 
{
	return m_cBouncyPlatformSpawns;
}
std::vector<std::pair<float, float>>			ConfigFileReader::GetCrumblingPlatformSpawns() 
{
	return m_cCrumblingPlatformSpawns;
}
std::vector<std::pair<float, float>>			ConfigFileReader::GetHazardSpawns() 
{
	return m_cHazardSpawns;
}




//==================== DEBUG / TESTING =====================

void ConfigFileReader::PrintOutUninstancedVariables()
{
	std::cout << "\n\nPrinting Out Stored Values:\n" << std::endl;
	for ( auto& elem : m_cUninstancedFloatVariables )
	{
		std::cout << elem.first << "\t" << elem.second << std::endl;
	}

	for ( auto& elem : m_cUninstancedVectorsVariables )
	{
		std::cout << elem.first << "\t( " << elem.second.first << ", " << elem.second.second << " )" << std::endl;
	}
}

void ConfigFileReader::PrintOutSpawnVariables() 
{
	std::cout << "\n\nPrinting Out Stored Values:\n" << std::endl;
	std::cout << "\nCategory: [Patrol_Enemy]" << std::endl;
	for ( SEnemyParams& sParams : m_cPatrolEnemySpawns )
	{
		std::cout << "SpawnPosition: " 
			<< "( " << sParams.sSpawnPosition.first << ", " << sParams.sSpawnPosition.second << " )" << std::endl;

		std::cout << "MovementDirection: "
			<< "( " << sParams.sSpawnPosition.first << ", " << sParams.sSpawnPosition.second << " )" << std::endl;

		std::cout << "MaximumMoveDistance: " << sParams.fMaximumMoveDistance << std::endl;
	}

	std::cout << "\nCategory: [Pursuing_Enemy]" << std::endl;
	for ( SEnemyParams& sParams : m_cPatrolEnemySpawns )
	{
		std::cout << "SpawnPosition: "
			<< "( " << sParams.sSpawnPosition.first << ", " << sParams.sSpawnPosition.second << " )" << std::endl;

		std::cout << "MovementDirection: "
			<< "( " << sParams.sSpawnPosition.first << ", " << sParams.sSpawnPosition.second << " )" << std::endl;

		std::cout << "MaximumMoveDistance: " << sParams.fMaximumMoveDistance << std::endl;
	}


	std::cout << "\nCategory: [Static_Platforms]" << std::endl;
	for ( auto& fParam : m_cStaticPlatformSpawns )
	{
		std::cout << "SpawnPosition: "
			<< "( " << fParam.first << ", " << fParam.second << " )" << std::endl;
	}

	std::cout << "\nCategory: [Moving_Platforms]" << std::endl;
	for ( SMovingPlatformParams& sParams : m_cMovingPlatformSpawns )
	{
		std::cout << "StartingPosition: "
			<< "( " << sParams.sStartingPosition.first << ", " << sParams.sStartingPosition.second << " )" << std::endl;

		std::cout << "EndingPosition: "
			<< "( " << sParams.sEndingPosition.first << ", " << sParams.sEndingPosition.second << " )" << std::endl;

		std::cout << "MovementVelocity: "
			<< "( " << sParams.sMovementVelocity.first << ", " << sParams.sMovementVelocity.second << " )" << std::endl;
	}

	std::cout << "\nCategory: [Bouncy_Platforms]" << std::endl;
	for ( SBouncyPlatformParams& sParam : m_cBouncyPlatformSpawns )
	{
		std::cout << "SpawnPosition: "
			<< "( " << sParam.sSpawnPosition.first << ", " << sParam.sSpawnPosition.second << " )" << std::endl;
		std::cout << "PlatformAngle (degrees): " << sParam.fPlatformRotationInDegrees << std::endl;
	}

	std::cout << "\nCategory: [Crumbling_Platforms]" << std::endl;
	for ( auto& fParam : m_cCrumblingPlatformSpawns )
	{
		std::cout << "SpawnPosition: "
			<< "( " << fParam.first << ", " << fParam.second << " )" << std::endl;
	}

	std::cout << "\nCategory: [Hazards]" << std::endl;
	for ( auto& fParam : m_cHazardSpawns )
	{
		std::cout << "SpawnPosition: "
			<< "( " << fParam.first << ", " << fParam.second << " )" << std::endl;
	}
}
#ifndef __CONFIGFILEREADER_H_
#define __CONFIGFILEREADER_H_

#define MAX_LINE_LENGTH 512

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>



//Struct for holding Enemy class type instance parameters.
struct SEnemyParams
{
	std::pair<float, float> sSpawnPosition		= { 0.0f, 0.0f };
	std::pair<float, float> sMovementDirection	= { 0.0f, 0.0f };
	float fMaximumMoveDistance					= 0.0f;
};

//Struct for holding MovingPlatform class type instance parameters.
struct SMovingPlatformParams
{
	std::pair<float, float> sStartingPosition	= { 0.0f, 0.0f };
	std::pair<float, float> sEndingPosition		= { 0.0f, 0.0f };
	std::pair<float, float> sMovementVelocity	= { 0.0f, 0.0f };
};

//Struct for holding BouncyPlatform class type instance parameters.
struct SBouncyPlatformParams 
{
	std::pair<float, float> sSpawnPosition = { 0.0f, 0.0f };
	float fPlatformRotationInDegrees = 0.0f;
};


//Class for reading in the separate files for passing to the game level.
class ConfigFileReader 
{
public:
	//Enums
	enum EInputState
	{
		EState_Undefined = 0,
		EState_Uninstanced,
		EState_Spawn
	};


	//Constructor
	ConfigFileReader()
	{
	}

	//Reset all member variables, empties data containers.
	void Reset();

	//Setup the input file for reading, setup mode and reset variables.
	void Init( std::string strFilename, EInputState EState_InputMode );

	//Input loop for reading in variables that don't belong to entity instances.
	void ReadVariablesFile();

	//Input loop for reading data that belongs to entity instances.
	void ReadSpawningListFile();


	//Data container accessors
	std::map<std::string, float>					GetUninstancedFloatVariables();
	std::map<std::string, std::pair<float, float>>	GetUninstancedVectorVariables();
	std::vector<SEnemyParams>						GetPatrolEnemySpawns();
	std::vector<SEnemyParams>						GetPursueEnemySpawns();
	std::vector<std::pair<float, float>>			GetStaticPlatformSpawns();
	std::vector<SMovingPlatformParams>				GetMovingPlatformSpawns();
	std::vector<SBouncyPlatformParams>				GetBouncyPlatformSpawns();
	std::vector<std::pair<float, float>>			GetCrumblingPlatformSpawns();
	std::vector<std::pair<float, float>>			GetHazardSpawns();

	//DEBUG FUNCTIONS
	void PrintOutUninstancedVariables();
	void PrintOutSpawnVariables();


private:
	//Enum
	enum ESpawnableEntity
	{
		ESpawn_Patrol_Enemy = 3,
		ESpawn_Pursue_Enemy = 3,
		ESpawn_Static_Platform = 1,
		ESpawn_Moving_Platform = 3,
		ESpawn_Bouncy_Platform = 2,
		ESpawn_Crumbling_Platform = 1,
		ESpawn_Hazard = 1,
		ESpawn_Undefined = 0
	};

	
	std::map<std::string, ESpawnableEntity> m_cStringToEntityEnumMap =
	{
		{ "[Patrol_Enemy]" , ESpawn_Patrol_Enemy },
		{ "[Pursuing_Enemy]", ESpawn_Pursue_Enemy },
		{ "[Static_Platforms]", ESpawn_Static_Platform },
		{ "[Moving_Platforms]", ESpawn_Moving_Platform },
		{ "[Bouncy_Platforms]", ESpawn_Bouncy_Platform },
		{ "[Crumbling_Platforms]", ESpawn_Crumbling_Platform },
		{ "[Hazards]", ESpawn_Hazard }
	};


	std::ifstream m_cIFile;
	char m_caLine[MAX_LINE_LENGTH];
	std::istringstream m_cIss;
	std::stringstream ssFloatCoverter;
	std::string m_strLine				= "";
	std::string m_strVariableName		= "";
	std::string m_strValue				= "";

	//Booleans for line reader status tracking
	bool m_bIsValue2DVector			= false;
	bool m_bIsFirstCharacter		= true;
	bool m_bSkipUntilEqualSign		= false;
	bool m_bVariableNameFound		= false;
	bool m_bValueFound				= false;
	bool m_bEndSymbolEncountered	= false;

	//File Reader State
	EInputState			m_eReaderLoadState	= EState_Undefined;
	ESpawnableEntity	m_eCurrentSpawn		= ESpawn_Undefined;
	std::string			m_strCurrentSpawn = "";

	//Uninstanced / Globals data containers.
	std::map<std::string, float>					m_cUninstancedFloatVariables	= {};
	std::map<std::string, std::pair<float, float>>	m_cUninstancedVectorsVariables	= {};

	//Unsorted parameter containers for spawn data.
	std::vector<float>						m_cSpawnFloatVariables	= {};
	std::vector<std::pair<float, float>>	m_cSpawnVectorVariables = {};

	//Spawn parameter containers
	std::vector<SEnemyParams>				m_cPatrolEnemySpawns		= {};
	std::vector<SEnemyParams>				m_cPursueEnemySpawns		= {};
	std::vector<std::pair<float, float>>	m_cStaticPlatformSpawns		= {};
	std::vector<SMovingPlatformParams>		m_cMovingPlatformSpawns		= {};
	std::vector<SBouncyPlatformParams>		m_cBouncyPlatformSpawns		= {};
	std::vector<std::pair<float, float>>	m_cCrumblingPlatformSpawns	= {};
	std::vector<std::pair<float, float>>	m_cHazardSpawns = {};

	//Reset the string to float stringstream
	void ResetFloatConverter();

	//Reset the line reader booleans.
	void ResetLineReader();

	//Read the line to find the name and associated value in text file
	//Returns pair of two strings <name, value>
	std::pair<std::string, std::string> ReadVariableFromLine();

	//Take the string pair and process them into the relevant data container.
	void ProcessVariableStringIntoData( std::pair<std::string, std::string> cDataPair );

	//Make a new instance of data for current spawn type.
	void SetNewInstanceData();

	//Check if line read result is empty
	bool IsLineDataEmpty(std::pair<std::string, std::string> sOutputValues);
};

#endif //__CONFIGFILEREADER_H_
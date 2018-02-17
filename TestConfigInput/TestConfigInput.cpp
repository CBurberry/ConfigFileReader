// TestConfigInput.cpp : Testing config ini files for the Prototype
//

#include "stdafx.h"
#include "ConfigFileReader.h"


int main()
{
	ConfigFileReader cFileReader;
	cFileReader.Init("Variables.txt", ConfigFileReader::EState_Uninstanced);
	cFileReader.PrintOutUninstancedVariables();

	cFileReader.Init( "SpawningLists.txt", ConfigFileReader::EState_Spawn );

	cFileReader.PrintOutSpawnVariables();

	cFileReader.Init( "Variables.txt", ConfigFileReader::EState_Uninstanced );
	cFileReader.PrintOutUninstancedVariables();

	return 0;
}
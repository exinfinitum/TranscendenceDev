//	Armor.cpp
//
//	Generate stats about armor
//	Copyright (c) 2019 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

void GenerateStdArmorTable (CUniverse &Universe, CXMLElement *pCmdLine)
	{
	int i;

	printf("Level\tHP\tCost\tMass\tEffective HP\n");

	for (i = 1; i <= MAX_ITEM_LEVEL; i++)
		{
		printf("%d\t%d\t%d\t%d\t%d\n",
				i,
				CArmorClass::GetStdHP(i),
				CArmorClass::GetStdCost(i),
				CArmorClass::GetStdMass(i),
				CArmorClass::GetStdEffectiveHP(Universe, i));
		}
	}

void GenerateStdShieldTable (CUniverse &Universe, CXMLElement *pCmdLine)
	{
	int i;

	printf("Level\tHP\tRegen\tCost\tPower\tEffective HP\n");

	for (i = 1; i <= MAX_ITEM_LEVEL; i++)
		{
		char szBuffer[1024];
		Metric rCost = CShieldClass::GetStdCost(i);
		if (rCost >= 1000000.0)
			wsprintf(szBuffer, "%dM", mathRound(rCost / 1000000.0));
		else
			wsprintf(szBuffer, "%d", (int)rCost);

		printf("%d\t%d\t%d\t%s\t%d\t%d\n",
				i,
				CShieldClass::GetStdHP(i),
				CShieldClass::GetStdRegen(i),
				szBuffer,
				CShieldClass::GetStdPower(i),
				CShieldClass::GetStdEffectiveHP(Universe, i));
		}
	}

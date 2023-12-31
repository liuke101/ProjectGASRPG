﻿// 


#include "GAS/Data/LevelDataAsset.h"

int32 ULevelDataAsset::FindLevelForExp(int32 Exp) const
{
	int32 Level = 1;
	bool bSearching = true;
	
	while(bSearching)
	{
		//占位符，我们认为LevelUpInfos[0]是无效信息，LevelUpInfos[1]是1级信息...
		if(LevelUpInfos.Num() - 1 <= Level)
		{
			return Level;
		}
		if(Exp >= LevelUpInfos[Level].LevelUpRequirement)
		{
			++Level;
		}
		else
		{
			bSearching = false;
		} 
		  
	}
	return Level;
}

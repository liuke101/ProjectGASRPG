// 


#include "GAS/Data/CharacterClassDataAsset.h"

FCharacterClassDefaultInfo UCharacterClassDataAsset::GetClassDefaultInfo(ECharacterClass CharacterClass)
{
	return CharacterClassDefaultInfos.FindChecked(CharacterClass);
}

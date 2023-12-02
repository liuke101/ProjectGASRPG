// 


#include "GAS/Data/CharacterClassDataAsset.h"

FCharacterClassDefaultInfo UCharacterClassDataAsset::GetCharacterClassDefaultInfo(ECharacterClass CharacterClass)
{
	return CharacterClass_To_CharacterClassDefaultInfo.FindChecked(CharacterClass);
}

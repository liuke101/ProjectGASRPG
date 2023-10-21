// 


#include "GAS/Data/CharacterClassInfo.h"

FCharacterClassDefaultInfo UCharacterClassInfo::GetClassDefaultInfo(ECharacterClass CharacterClass)
{
	return CharacterClassDefaultInfo.FindChecked(CharacterClass);
}

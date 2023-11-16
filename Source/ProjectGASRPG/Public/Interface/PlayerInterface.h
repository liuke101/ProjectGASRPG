// 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerInterface.generated.h"

UINTERFACE()
class UPlayerInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTGASRPG_API IPlayerInterface
{
	GENERATED_BODY()

public:
	virtual void LevelUp() = 0;
	
	/** 根据经验值获取对应的等级 */
	virtual int32 FindLevelForExp(const int32 InExp) const = 0;
	
	virtual int32 GetExp() const = 0;
	virtual void AddToExp(const int32 InExp) = 0;
	
	virtual void AddToLevel(const int32 InLevel) = 0;
	
	virtual int32 GetAttributePointReward(const int32 Level) const = 0;
	virtual void AddToAttributePoint(const int32 InPoints) = 0;
	virtual int32 GetAttributePoint() const = 0;

	virtual int32 GetSkillPointReward(const int32 Level) const = 0;
	virtual void AddToSkillPoint(const int32 InPoints) = 0;
	virtual int32 GetSkillPoint() const = 0;
	
};

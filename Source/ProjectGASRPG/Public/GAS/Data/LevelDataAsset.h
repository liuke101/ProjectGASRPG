// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelDataAsset.generated.h"

/** 升级信息 */
USTRUCT(BlueprintType)
struct FMageLevelUpInfo
{
	GENERATED_BODY()

	/** 升级所需经验,包含累积的上一级经验 */
	UPROPERTY(EditDefaultsOnly)
	int32 LevelUpRequirement = 0;

	UPROPERTY(EditDefaultsOnly)
	int32 AttributePointAward = 0;

	UPROPERTY(EditDefaultsOnly)
	int32 SkillPointAward = 0;
};

UCLASS()
class PROJECTGASRPG_API ULevelDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FMageLevelUpInfo> LevelUpInfos;

	/** 根据经验值获取等级数 */
	int32 FindLevelForExp(int32 Exp) const;
};

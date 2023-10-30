// VRM4U Copyright (c) 2021-2023 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmImportUI.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Factories/ImportSettings.h"
#include "VrmConvert.h"

const FImportOptionData* UVrmImportUI::GenerateOptionData() {

	FImportOptionData &d = data;

#define c(a)  d.a = a;

	d.init();

	c(bUEFN);

	c(bAPoseRetarget);

	c(bMobileBone);

	c(MaterialType);

	c(bForceOverride);

	c(ModelScale);

	c(AnimationTranslateScale);

	c(PlayRateScale);

	c(bVrm10RemoveLocalRotation);

	c(bVrm10Bindpose);

	c(bStrictBoneName);

	c(bGenerateHumanoidRenamedMesh);

	c(bGenerateIKBone);

	c(bGenerateRigIK);

	c(bSkipPhysics);

	c(bSkipMorphTarget);

	c(bEnableMorphTargetNormal);

	c(bStrictMorphTargetNameMode);

	c(bRemoveBlendShapeGroupPrefix);

	c(bForceOpaque);

	c(bForceTwoSided);

	c(bSingleUAssetFile);

	c(bDefaultGridTextureMode);

	c(bBC7Mode);

	c(bMipmapGenerateMode);

	c(bGenerateOutlineMaterial);

	c(bMergeMaterial);

	c(bMergePrimitive);

	c(bOptimizeVertex);

	c(bRemoveDegenerateTriangles);

	c(BoneWeightInfluenceNum);

	c(bSimpleRoot);

	c(bActiveBone);

	c(bSkipNoMeshBone);

	c(bDebugOneBone);

	c(Skeleton);

	return &data;
}


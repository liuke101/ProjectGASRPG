// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NGAAnimGraphConnectionDrawingPolicy.h"

#ifdef NGA_WITH_ENGINE_CPP
	#if ENGINE_MAJOR_VERSION == 5
		#include "AnimationPins/SGraphPinPose.cpp"
	#endif
#include "AnimGraphConnectionDrawingPolicy.cpp"
#else
#include "../EngineCppFiles/AnimGraphConnectionDrawingPolicy.cpp"
#endif


void FNGAAnimGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	OverrideDrawPreviewConnector(PinGeometry, StartPoint, EndPoint, Pin);
}


void FNGAAnimGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	ResetPayloadData();
	FAnimGraphConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
	OverrideDraw(InPinGeometries, ArrangedNodes);
}


void FNGAAnimGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	OverrideDrawConnection(LayerId, Start, End, Params);
}
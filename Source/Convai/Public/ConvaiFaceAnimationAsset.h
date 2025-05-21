// Copyright 2022 Convai Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ConvaiDefinitions.h"
#include "ConvaiFaceAnimationAsset.generated.h"

UCLASS(BlueprintType)
class CONVAI_API UConvaiFaceAnimationAsset : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Convai|Face Animation")
    FAnimationSequence FaceSequence;

    // Duration of the animation in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Convai|Face Animation")
    float Duration;

    // Frame rate of the animation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Convai|Face Animation")
    float FrameRate;

    // Number of frames in the animation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Convai|Face Animation")
    int32 NumFrames;
}; 
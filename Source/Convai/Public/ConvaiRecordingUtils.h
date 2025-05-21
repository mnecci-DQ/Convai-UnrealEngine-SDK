// Copyright 2022 Convai Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConvaiDefinitions.h"
#include "ConvaiRecordingUtils.generated.h"

UCLASS()
class CONVAI_API UConvaiRecordingUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Save audio data to a WAV file
    UFUNCTION(BlueprintCallable, Category = "Convai|Recording")
    static bool SaveAudioToWavFile(const TArray<uint8>& PCMData, const FString& FilePath, int32 SampleRate, int32 NumChannels);

    // Save viseme/blendshape data to a JSON file
    UFUNCTION(BlueprintCallable, Category = "Convai|Recording")
    static bool SaveFaceDataToJsonFile(const FAnimationSequence& FaceSequence, const FString& FilePath);

    // Save both audio and face data with matching timestamps
    UFUNCTION(BlueprintCallable, Category = "Convai|Recording")
    static bool SaveAudioAndFaceData(const TArray<uint8>& PCMData, const FAnimationSequence& FaceSequence, 
        const FString& BaseFilePath, int32 SampleRate, int32 NumChannels);

    // Save audio data as a Sound Wave asset
    UFUNCTION(BlueprintCallable, Category = "Convai|Recording")
    static USoundWave* SaveAudioAsSoundWaveAsset(const TArray<uint8>& PCMData, const FString& AssetName, int32 SampleRate, int32 NumChannels);

    // Save face animation data as a custom asset
    UFUNCTION(BlueprintCallable, Category = "Convai|Recording")
    static UConvaiFaceAnimationAsset* SaveFaceDataAsAsset(const FAnimationSequence& FaceSequence, const FString& AssetName);

    // Save both audio and face data as assets
    UFUNCTION(BlueprintCallable, Category = "Convai|Recording")
    static bool SaveAudioAndFaceDataAsAssets(const TArray<uint8>& PCMData, const FAnimationSequence& FaceSequence, 
        const FString& BaseAssetName, int32 SampleRate, int32 NumChannels, USoundWave*& OutSoundWave, UConvaiFaceAnimationAsset*& OutFaceAnimation);

private:
    static bool WriteWavHeader(FArchive& Ar, int32 DataSize, int32 SampleRate, int32 NumChannels);
}; 
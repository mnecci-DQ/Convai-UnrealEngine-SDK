// Copyright 2022 Convai Inc. All Rights Reserved.

#include "ConvaiRecordingUtils.h"
#include "ConvaiAudioStreamer.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Sound/SoundWave.h"
#include "ConvaiFaceAnimationAsset.h"
#include "ConvaiUtils.h"

bool UConvaiRecordingUtils::SaveAudioToWavFile(const TArray<uint8>& PCMData, const FString& FilePath, int32 SampleRate, int32 NumChannels)
{
    if (PCMData.Num() == 0)
    {
        UE_LOG(ConvaiAudioStreamerLog, Warning, TEXT("SaveAudioToWavFile: PCMData is empty"));
        return false;
    }

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioToWavFile: Attempting to save WAV file to: %s"), *FilePath);

    // Create the file
    FArchive* FileAr = IFileManager::Get().CreateFileWriter(*FilePath);
    if (!FileAr)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioToWavFile: Failed to create file writer for path: %s"), *FilePath);
        return false;
    }

    // Write WAV header
    if (!WriteWavHeader(*FileAr, PCMData.Num(), SampleRate, NumChannels))
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioToWavFile: Failed to write WAV header"));
        delete FileAr;
        return false;
    }

    // Write PCM data
    FileAr->Serialize((void*)PCMData.GetData(), PCMData.Num());
    delete FileAr;

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioToWavFile: Successfully saved WAV file to: %s"), *FilePath);
    return true;
}

bool UConvaiRecordingUtils::SaveFaceDataToJsonFile(const FAnimationSequence& FaceSequence, const FString& FilePath)
{
    if (FaceSequence.AnimationFrames.Num() == 0)
    {
        UE_LOG(ConvaiAudioStreamerLog, Warning, TEXT("SaveFaceDataToJsonFile: FaceSequence has no frames"));
        return false;
    }

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveFaceDataToJsonFile: Attempting to save JSON file to: %s"), *FilePath);
    
    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
    
    // Add sequence metadata
    RootObject->SetNumberField("Duration", FaceSequence.Duration);
    RootObject->SetNumberField("FrameRate", FaceSequence.FrameRate);

    // Create frames array
    TArray<TSharedPtr<FJsonValue>> FramesArray;
    for (const FAnimationFrame& Frame : FaceSequence.AnimationFrames)
    {
        TSharedPtr<FJsonObject> FrameObject = MakeShared<FJsonObject>();
        FrameObject->SetNumberField("FrameIndex", Frame.FrameIndex);

        // Create blendshapes object
        TSharedPtr<FJsonObject> BlendShapesObject = MakeShared<FJsonObject>();
        for (const auto& Pair : Frame.BlendShapes)
        {
            BlendShapesObject->SetNumberField(Pair.Key.ToString(), Pair.Value);
        }
        FrameObject->SetObjectField("BlendShapes", BlendShapesObject);

        FramesArray.Add(MakeShared<FJsonValueObject>(FrameObject));
    }
    RootObject->SetArrayField("Frames", FramesArray);

    // Convert to string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    // Save to file
    bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FilePath);
    if (bSuccess)
    {
        UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveFaceDataToJsonFile: Successfully saved JSON file to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveFaceDataToJsonFile: Failed to save JSON file to: %s"), *FilePath);
    }
    return bSuccess;
}

bool UConvaiRecordingUtils::SaveAudioAndFaceData(const TArray<uint8>& PCMData, const FAnimationSequence& FaceSequence, 
    const FString& BaseFilePath, int32 SampleRate, int32 NumChannels)
{
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAndFaceData: Starting save process with base path: %s"), *BaseFilePath);

    // Save audio
    FString AudioFilePath = FPaths::ChangeExtension(BaseFilePath, TEXT("wav"));
    if (!SaveAudioToWavFile(PCMData, AudioFilePath, SampleRate, NumChannels))
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAndFaceData: Failed to save audio file"));
        return false;
    }

    // Save face data
    FString FaceDataFilePath = FPaths::ChangeExtension(BaseFilePath, TEXT("json"));
    bool bSuccess = SaveFaceDataToJsonFile(FaceSequence, FaceDataFilePath);
    
    if (bSuccess)
    {
        UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAndFaceData: Successfully saved both files:"));
        UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("  Audio: %s"), *AudioFilePath);
        UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("  Face Data: %s"), *FaceDataFilePath);
    }
    else
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAndFaceData: Failed to save face data file"));
    }
    
    return bSuccess;
}

bool UConvaiRecordingUtils::WriteWavHeader(FArchive& Ar, int32 DataSize, int32 SampleRate, int32 NumChannels)
{
    // RIFF chunk
    Ar.Serialize((void*)"RIFF", 4);
    int32 ChunkSize = DataSize + 36; // 36 is the size of the header minus the RIFF and size fields
    Ar << ChunkSize;
    Ar.Serialize((void*)"WAVE", 4);

    // fmt chunk
    Ar.Serialize((void*)"fmt ", 4);
    int32 Subchunk1Size = 16;
    Ar << Subchunk1Size;
    int16 AudioFormat = 1; // PCM
    Ar << AudioFormat;
    Ar << NumChannels;
    Ar << SampleRate;
    int32 ByteRate = SampleRate * NumChannels * 2; // 2 bytes per sample
    Ar << ByteRate;
    int16 BlockAlign = NumChannels * 2;
    Ar << BlockAlign;
    int16 BitsPerSample = 16;
    Ar << BitsPerSample;

    // data chunk
    Ar.Serialize((void*)"data", 4);
    Ar << DataSize;

    return true;
}

USoundWave* UConvaiRecordingUtils::SaveAudioAsSoundWaveAsset(const TArray<uint8>& PCMData, const FString& AssetName, int32 SampleRate, int32 NumChannels)
{
    if (PCMData.Num() == 0)
    {
        UE_LOG(ConvaiAudioStreamerLog, Warning, TEXT("SaveAudioAsSoundWaveAsset: PCMData is empty"));
        return nullptr;
    }

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAsSoundWaveAsset: Starting to save audio as asset: %s"), *AssetName);

    // Create a temporary WAV file
    FString TempFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Temp"), FString::Printf(TEXT("%s_temp.wav"), *AssetName));
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAsSoundWaveAsset: Creating temporary WAV file at: %s"), *TempFilePath);
    
    if (!SaveAudioToWavFile(PCMData, TempFilePath, SampleRate, NumChannels))
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAsSoundWaveAsset: Failed to create temporary WAV file"));
        return nullptr;
    }

    // Create the asset
    FString PackagePath = TEXT("/Game/Convai/Recordings");
    FString AssetPath = FPaths::Combine(PackagePath, AssetName);
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAsSoundWaveAsset: Creating asset at: %s"), *AssetPath);
    
    UPackage* Package = CreatePackage(*AssetPath);
    if (!Package)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAsSoundWaveAsset: Failed to create package"));
        return nullptr;
    }

    // Create the SoundWave asset
    USoundWave* SoundWave = NewObject<USoundWave>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!SoundWave)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAsSoundWaveAsset: Failed to create SoundWave object"));
        Package->MarkAsGarbage();
        return nullptr;
    }

    // Load the WAV data into the SoundWave
    TArray<uint8> RawData;
    if (!FFileHelper::LoadFileToArray(RawData, *TempFilePath))
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAsSoundWaveAsset: Failed to load WAV data from temporary file"));
        Package->MarkAsGarbage();
        return nullptr;
    }

    // Convert WAV data to SoundWave
    SoundWave = UConvaiUtils::WavDataToSoundWave(RawData);
    if (!SoundWave)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAsSoundWaveAsset: Failed to convert WAV data to SoundWave"));
        Package->MarkAsGarbage();
        return nullptr;
    }

    // Register the asset
    FAssetRegistryModule::AssetCreated(SoundWave);
    SoundWave->MarkPackageDirty();

    // Clean up temporary file
    IFileManager::Get().Delete(*TempFilePath);
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAsSoundWaveAsset: Successfully created SoundWave asset: %s"), *AssetName);

    return SoundWave;
}

UConvaiFaceAnimationAsset* UConvaiRecordingUtils::SaveFaceDataAsAsset(const FAnimationSequence& FaceSequence, const FString& AssetName)
{
    if (FaceSequence.AnimationFrames.Num() == 0)
    {
        UE_LOG(ConvaiAudioStreamerLog, Warning, TEXT("SaveFaceDataAsAsset: FaceSequence has no frames"));
        return nullptr;
    }

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveFaceDataAsAsset: Starting to save face data as asset: %s"), *AssetName);

    // Create the asset
    FString PackagePath = TEXT("/Game/Convai/Recordings");
    FString AssetPath = FPaths::Combine(PackagePath, AssetName);
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveFaceDataAsAsset: Creating asset at: %s"), *AssetPath);
    
    UPackage* Package = CreatePackage(*AssetPath);
    if (!Package)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveFaceDataAsAsset: Failed to create package"));
        return nullptr;
    }

    // Create the face animation asset
    UConvaiFaceAnimationAsset* FaceAnimation = NewObject<UConvaiFaceAnimationAsset>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!FaceAnimation)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveFaceDataAsAsset: Failed to create FaceAnimation object"));
        Package->MarkAsGarbage();
        return nullptr;
    }

    // Set the face sequence data
    FaceAnimation->FaceSequence = FaceSequence;
    FaceAnimation->Duration = FaceSequence.Duration;
    FaceAnimation->FrameRate = FaceSequence.FrameRate;
    FaceAnimation->NumFrames = FaceSequence.AnimationFrames.Num();

    // Register the asset
    FAssetRegistryModule::AssetCreated(FaceAnimation);
    FaceAnimation->MarkPackageDirty();

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveFaceDataAsAsset: Successfully created FaceAnimation asset: %s"), *AssetName);
    return FaceAnimation;
}

bool UConvaiRecordingUtils::SaveAudioAndFaceDataAsAssets(const TArray<uint8>& PCMData, const FAnimationSequence& FaceSequence, 
    const FString& BaseAssetName, int32 SampleRate, int32 NumChannels, USoundWave*& OutSoundWave, UConvaiFaceAnimationAsset*& OutFaceAnimation)
{
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAndFaceDataAsAssets: Starting to save both assets with base name: %s"), *BaseAssetName);

    // Save audio as SoundWave asset
    OutSoundWave = SaveAudioAsSoundWaveAsset(PCMData, BaseAssetName + TEXT("_Audio"), SampleRate, NumChannels);
    if (!OutSoundWave)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAndFaceDataAsAssets: Failed to save audio asset"));
        return false;
    }

    // Save face data as face animation asset
    OutFaceAnimation = SaveFaceDataAsAsset(FaceSequence, BaseAssetName + TEXT("_Face"));
    if (!OutFaceAnimation)
    {
        UE_LOG(ConvaiAudioStreamerLog, Error, TEXT("SaveAudioAndFaceDataAsAssets: Failed to save face animation asset"));
        // Clean up the SoundWave asset if face animation creation fails
        OutSoundWave->MarkAsGarbage();
        OutSoundWave = nullptr;
        return false;
    }

    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("SaveAudioAndFaceDataAsAssets: Successfully saved both assets:"));
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("  Audio: %s_Audio"), *BaseAssetName);
    UE_LOG(ConvaiAudioStreamerLog, Log, TEXT("  Face Animation: %s_Face"), *BaseAssetName);
    return true;
} 
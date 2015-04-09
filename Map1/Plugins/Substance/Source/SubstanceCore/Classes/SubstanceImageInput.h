// Copyright 2014 Allegorithmic All Rights Reserved.
#pragma once

#include "Engine.h"
#include "SubstanceGraphInstance.h"
#include "SubstanceImageInput.generated.h"

namespace Substance
{
	struct FImageInputInstance;
}

UCLASS(hideCategories=Object, MinimalAPI, BlueprintType)
class USubstanceImageInput : public UObject
{
	GENERATED_UCLASS_BODY()
public:

    FByteBulkData CompressedImageRGB;
    FByteBulkData CompressedImageA;
    int32 CompRGB;
    int32 CompA;
	
	UPROPERTY(Category = File, VisibleAnywhere)
	int32 SizeX;

	UPROPERTY(Category = File, VisibleAnywhere)
	int32 SizeY;

	UPROPERTY(Category = File, VisibleAnywhere)
    int32 NumComponents;

	UPROPERTY(Category = File, VisibleAnywhere, BlueprintReadOnly)
    FString SourceFilePath;

	UPROPERTY(Category = File, VisibleAnywhere, BlueprintReadOnly)
    FString SourceFileTimestamp;

	UPROPERTY()
	TArray< USubstanceGraphInstance* > Consumers;

public:

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void Serialize(FArchive& Ar) override;
	virtual FString GetDesc() override;

	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type) override;
};

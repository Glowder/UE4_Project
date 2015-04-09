//! @file SubstanceImageInput.cpp
//! @brief Implementation of the USubstanceImageInput class
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceCoreTypedefs.h"
#include "SubstanceInput.h"
#include "SubstanceImageInput.h"


USubstanceImageInput::USubstanceImageInput(class FObjectInitializer const & PCIP) : Super(PCIP)
{

}


#if WITH_EDITOR
void USubstanceImageInput::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


void USubstanceImageInput::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	//! todo: remove image date if all consumers are baked

	CompressedImageRGB.Serialize(Ar, this);
	CompressedImageA.Serialize(Ar, this);

	// image inputs can be used multiple times
	CompressedImageRGB.ClearBulkDataFlags(BULKDATA_SingleUse); 
	CompressedImageA.ClearBulkDataFlags(BULKDATA_SingleUse);

	Ar << CompRGB;
	Ar << CompA;

	if (Ar.IsCooking())
	{
		SourceFilePath = FString();
		SourceFileTimestamp = FString();
	}
}


FString USubstanceImageInput::GetDesc()
{
	return FString::Printf( TEXT("%dx%d (%d kB)"), SizeX, SizeY, GetResourceSize(EResourceSizeMode::Exclusive)/1024);
}


SIZE_T USubstanceImageInput::GetResourceSize(EResourceSizeMode::Type)
{
	return CompressedImageRGB.GetBulkDataSize() + CompressedImageA.GetBulkDataSize();
}

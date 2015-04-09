//! @file SubstanceImageInputThumbnailRenderer.cpp
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#include "SubstanceEditorPrivatePCH.h"
#include "SubstanceCoreTypedefs.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceImageInput.h"

#include "ImageUtils.h"


USubstanceImageInputThumbnailRenderer::USubstanceImageInputThumbnailRenderer(const class FObjectInitializer& PCIP)
	: Super(PCIP)
{
}

void USubstanceImageInputThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas)
{
	USubstanceImageInput* ImageInput = Cast<USubstanceImageInput>(Object);

	UTexture2DDynamic* ThumbnailTexture =
			Cast<UTexture2DDynamic>(StaticConstructObject(UTexture2DDynamic::StaticClass(), GetTransientPackage(), NAME_None, RF_Transient));

	if (ImageInput)
	{
		int32 SizeCompressedImageRGB = ImageInput->CompressedImageRGB.GetBulkDataSize();
		int32 SizeCompressedImageA = ImageInput->CompressedImageA.GetBulkDataSize();

		int32 ImageW = 0;
		int32 ImageH = 0;

		// if the image input is color, decompress it
		if (SizeCompressedImageRGB)
		{
			int32 SizeCompressedImageRGB = ImageInput->CompressedImageRGB.GetBulkDataSize();

			void* CompressedImageRGB =
				(void*)ImageInput->CompressedImageRGB.Lock(LOCK_READ_ONLY);

			TArray<uint8> Temp;
			TArray<FColor> ThumbnailToScale;
			TArray<FColor> ScaledThumbnail;

			// the decompressed output is RGBA
			Substance::Helpers::DecompressJpeg(
				CompressedImageRGB,
				SizeCompressedImageRGB,
				Temp,
				&ImageW,
				&ImageH);

			ImageInput->CompressedImageRGB.Unlock();

			if (ImageW != Width || ImageH != Height)
			{
				uint32 Size = ImageW * ImageH; 

				ThumbnailToScale.Empty();
				ThumbnailToScale.AddUninitialized(Size * sizeof(FColor));

				for (uint32 i = 0; i < Size; i++)
				{
					uint32 Tex = i * 4;
					ThumbnailToScale[i] = FColor(Temp[Tex+2], Temp[Tex + 1], Temp[Tex], Temp[Tex + 3]);
				}

				// resize the image input to required size
				FImageUtils::CropAndScaleImage(ImageW, ImageH, Width, Height, ThumbnailToScale, ScaledThumbnail);

				uint32 NewSize = Width * Height; 

				Temp.Empty();
				Temp.AddUninitialized(NewSize * sizeof(FColor));
				
				FMemory::Memcpy(&Temp[0], &ScaledThumbnail[0], NewSize * sizeof(FColor));
			}

			ThumbnailTexture->Init(Width, Height);
			
			struct FMipUpdateParams
			{
				int32 MipIdx;
				TArray<uint8> MipData;
				FTexture2DDynamicResource* Resource;
			};

			FMipUpdateParams* MipUpdateData = new FMipUpdateParams();
			MipUpdateData->MipIdx = 0;
			MipUpdateData->MipData = Temp;
			MipUpdateData->Resource = (FTexture2DDynamicResource*)ThumbnailTexture->Resource;

			// queue command to update texture mip, copies MipData for asynch operation
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				FUpdateMipData,
				FMipUpdateParams*, MipUpdateData, MipUpdateData,
			{
				// Lock new texture.
				uint32 DestPitch;

				void* LockedMipData = RHILockTexture2D(
					MipUpdateData->Resource->GetTexture2DRHI(), 
					MipUpdateData->MipIdx, RLM_WriteOnly, DestPitch, false);

				if (LockedMipData != NULL)
				{
					FMemory::Memcpy(LockedMipData,MipUpdateData->MipData.GetData(),MipUpdateData->MipData.Num());
					RHIUnlockTexture2D(MipUpdateData->Resource->GetTexture2DRHI(), MipUpdateData->MipIdx, false);
				}
				delete MipUpdateData;
			});

			// Use the texture interface to draw
			Canvas->DrawTile(X,Y,Width,Height,0.f,0.f,1.f,1.f,FLinearColor::White,
				ThumbnailTexture->Resource);

			float SrcRatio = (float)ImageW/(float)ImageH;
			float destRatio = (float)Width/(float)Height;

			// warn the user in case the image has been cropped
			if ( ! FMath::IsNearlyZero(SrcRatio - destRatio))
			{
				UFont* Font = GEngine->GetTinyFont();

				if (ImageW > 256)
				{
					Font = GEngine->GetLargeFont();
				}
				else if (ImageW > 128)
				{
					Font = GEngine->GetMediumFont();
				}
				else if (ImageW > 64)
				{
					Font = GEngine->GetSmallFont();
				}

				Canvas->DrawShadowedString(
					0.0f, Height * 0.8f,
					TEXT("Thumbnail is cropped"),
					Font, FColor(255,50,50));
			}	
		}

		//else if(SizeCompressedImageA)
		{
			// grayscale image
		}
	}
}

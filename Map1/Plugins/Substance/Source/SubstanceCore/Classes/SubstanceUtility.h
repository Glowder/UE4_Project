// Copyright 2014 Allegorithmic All Rights Reserved.

#pragma once

#include "SubstanceUtility.generated.h"

UCLASS(BlueprintType, MinimalAPI)
class USubstanceUtility : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/* Get the list of Substance Graph Instances used by a material */
	UFUNCTION(BlueprintCallable, Category="Substance")
	static TArray<class USubstanceGraphInstance*> GetSubstances(class UMaterialInterface* Material);

	/* Get the textures (from enabled outputs) of a Substance Graph Instance*/
	UFUNCTION(BlueprintCallable, Category="Substance")
	static TArray<class USubstanceTexture2D*> GetSubstanceTextures(class USubstanceGraphInstance* GraphInstance);

	/* Get the current rendering progress of the Substances currently loaded ([0, 1.0]) */
	UFUNCTION(BlueprintCallable, Category="Substance")
	static float GetSubstanceLoadingProgress();
	
	/* Create a dynamic Substance Graph Instance (no outputs enabled by default) */
	UFUNCTION(BlueprintCallable, Category="Substance", meta=(WorldContext="WorldContextObject"))
	static USubstanceGraphInstance* CreateGraphInstance(UObject* WorldContextObject, class USubstanceInstanceFactory* Factory, int32 GraphDescIndex, FString InstanceName = TEXT(""));

	/* Create a copy of Substance Graph Instance */
	UFUNCTION(BlueprintCallable, Category="Substance", meta=(WorldContext = "WorldContextObject"))
	static USubstanceGraphInstance* DuplicateGraphInstance(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance, bool bCopyOutputs);

	/* Create the textures of a Substance Graph Instance (enable its outputs) using their indices */
	UFUNCTION(BlueprintCallable, Category="Substance", meta=(WorldContext="WorldContextObject"))
	static void CreateInstanceOutputs(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance, TArray<int32> OutputIndices);

	/* Copy the inputs values from a Substance Graph Instance to another one */
	UFUNCTION(BlueprintCallable, Category="Substance")
	static void CopyInputParameters(USubstanceGraphInstance* SourceGraphInstance, USubstanceGraphInstance* DestGraphInstance);

	/* Reset the input values of a Substance Graph Instance to their default values */
	UFUNCTION(BlueprintCallable, Category="Substance")
	static void ResetInputParameters(USubstanceGraphInstance* GraphInstance);

	/* Queue a Substance Graph Instance in the renderer */
	UFUNCTION(BlueprintCallable, Category = "Substance|Render")
	static void AsyncRendering(USubstanceGraphInstance* InstancesToRender);

	/* Start the synchronous rendering of a Substance */
	UFUNCTION(BlueprintCallable, Category="Substance|Render")
	static void SyncRendering(USubstanceGraphInstance* InstancesToRender);
};

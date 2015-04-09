//! @file SubstanceUtility.cpp
//! @brief Implementation of the SubstanceUtility blueprint utility class
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceUtility.h"
#include "SubstanceFGraph.h"
#include "SubstanceFPackage.h"
#include "SubstanceTexture2D.h"
#include "SubstanceGraphInstance.h"
#include "SubstanceInstanceFactory.h"


#include "Materials/MaterialExpressionTextureSample.h"

DEFINE_LOG_CATEGORY_STATIC(LogSbsBP, Warning, All);

USubstanceUtility::USubstanceUtility(class FObjectInitializer const & PCIP) : Super(PCIP)
{
}


TArray<class USubstanceGraphInstance*> USubstanceUtility::GetSubstances(class UMaterialInterface* MaterialInterface)
{
	TArray<class USubstanceGraphInstance*> Substances;

	if (!MaterialInterface)
	{
		return Substances;
	}

	UMaterial* Material = MaterialInterface->GetMaterial(); 

	for (int32 ExpressionIndex = Material->Expressions.Num() - 1; ExpressionIndex >= 0; ExpressionIndex--)
	{
		UMaterialExpressionTextureSample* Expression = Cast<UMaterialExpressionTextureSample>(Material->Expressions[ExpressionIndex]);

		if (Expression)
		{
			USubstanceTexture2D* SubstanceTexture = Cast<USubstanceTexture2D>(Expression->Texture);

			if (SubstanceTexture && SubstanceTexture->ParentInstance)
			{
				Substances.AddUnique(SubstanceTexture->ParentInstance);
			}
		}
	}

	return Substances;
}


TArray<class USubstanceTexture2D*> USubstanceUtility::GetSubstanceTextures(class USubstanceGraphInstance* SubstanceInstance)
{
	TArray<class USubstanceTexture2D*> SubstanceTextures;

	if (!SubstanceInstance)
	{
		return SubstanceTextures;
	}

	for (uint32 Idx=0 ; Idx<SubstanceInstance->Instance->Outputs.size() ; ++Idx)
	{
		output_inst_t* OutputInstance = &SubstanceInstance->Instance->Outputs[Idx];

		if (OutputInstance->bIsEnabled)
		{
			SubstanceTextures.Add(*OutputInstance->Texture.get());
		}
	}
	return SubstanceTextures;
}


float USubstanceUtility::GetSubstanceLoadingProgress()
{
	return Substance::Helpers::GetSubstanceLoadingProgress();
}


USubstanceGraphInstance* USubstanceUtility::CreateGraphInstance(UObject* WorldContextObject, USubstanceInstanceFactory* Factory, int32 GraphDescIndex, FString InstanceName)
{	
	check(WorldContextObject);
	USubstanceGraphInstance* GraphInstance = NULL;

	if (Factory && Factory->SubstancePackage && GraphDescIndex < Factory->SubstancePackage->Graphs.Num())
	{
		if (Factory->GenerationMode != SGM_OnLoadAsync && Factory->GenerationMode != SGM_OnLoadSync)
		{
			UE_LOG(LogSbsBP, Warning, TEXT("Cannot create Graph Instance for Instance Factory %s, GenerationMode value not set to OnLoadAsync or OnLoadSync!"),
				*Factory->GetName());
			return GraphInstance;
		}

		GraphInstance = ConstructObject<USubstanceGraphInstance>(
			USubstanceGraphInstance::StaticClass(),
			WorldContextObject ? WorldContextObject : GetTransientPackage(),
			*InstanceName);

		Factory->SubstancePackage->Graphs[GraphDescIndex]->Instantiate(
			GraphInstance, 
			false/*bCreateOutputs*/,
			true /*bSubscribeInstance*/,
			true /*bDynamicInstance*/);

		GraphInstance->Instance->bIsFreezed = false;
	}
	else
	{

	}

	return GraphInstance;
}	


USubstanceGraphInstance* USubstanceUtility::DuplicateGraphInstance(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance, bool bCopyOutputs)
{
	check(WorldContextObject);

	USubstanceGraphInstance* NewGraphInstance = NULL;

	if (GraphInstance && GraphInstance->Parent && GraphInstance->Parent->SubstancePackage)
	{
		int idx = 0;
		for (auto itGraph = GraphInstance->Parent->SubstancePackage->Graphs.itfront(); itGraph; ++itGraph)
		{
			if (GraphInstance->Instance->Desc == *itGraph)
			{
				break;
			}
			++idx;
		}

		NewGraphInstance = CreateGraphInstance(WorldContextObject, GraphInstance->Parent, idx);

		CopyInputParameters(GraphInstance, NewGraphInstance);

		if (bCopyOutputs)
		{
			TArray<int32> Outputs;

			for(auto itOut = GraphInstance->Instance->Outputs.itfrontconst(); itOut; ++itOut)
			{
				if (itOut->bIsEnabled)
				{
					Outputs.AddUnique(itOut.GetIndex());
				}
			}

			CreateInstanceOutputs(WorldContextObject, NewGraphInstance, Outputs);
		}
	}

	return NewGraphInstance;
}


void USubstanceUtility::CreateInstanceOutputs(UObject* WorldContextObject, USubstanceGraphInstance* GraphInstance, TArray<int32> OutputIndices)
{
	if (!GraphInstance || !GraphInstance->Instance)
	{
		return;
	}

	for (auto IdxIt = OutputIndices.CreateConstIterator(); IdxIt; ++IdxIt)
	{
		if (*IdxIt >= GraphInstance->Instance->Outputs.Num())
		{
			UE_LOG(LogSbsBP, Warning, TEXT("Cannot create Output for %s, index %d out of range!"),
				*GraphInstance->GetName(), *IdxIt);
			continue;
		}

		output_inst_t* OutputInstance = &GraphInstance->Instance->Outputs[*IdxIt];
		USubstanceTexture2D** ptr = OutputInstance->Texture.get();

		if (ptr && *ptr == NULL)
		{
			Substance::Helpers::CreateSubstanceTexture2D(
				OutputInstance, 
				true,
				FString() /*name does not matter for dynamic instances*/, 
				WorldContextObject ? WorldContextObject : GetTransientPackage());
			OutputInstance->bIsEnabled = true;
			OutputInstance->bIsDirty = true;
		}
		else
		{
			UE_LOG(LogSbsBP, Warning, TEXT("Cannot create Output for %s, index %d already created!"),
				*GraphInstance->GetName(), *IdxIt);
			continue;
		}
	}

	// upload some place holder content in the texture to make the texture usable
	Substance::Helpers::CreatePlaceHolderMips(GraphInstance->Instance);
}


void USubstanceUtility::CopyInputParameters(USubstanceGraphInstance* SourceGraphInstance, USubstanceGraphInstance* DestGraphInstance)
{
	if (!SourceGraphInstance ||
		!SourceGraphInstance->Instance ||
		!DestGraphInstance ||
		!DestGraphInstance->Instance)
	{
		return;
	}

	Substance::Helpers::CopyInstance(SourceGraphInstance->Instance, DestGraphInstance->Instance, false);
}


void USubstanceUtility::ResetInputParameters(USubstanceGraphInstance* GraphInstance)
{
	if (!GraphInstance ||
		!GraphInstance->Instance)
	{
		return;
	}

	Substance::Helpers::ResetToDefault(GraphInstance->Instance);
}


void USubstanceUtility::SyncRendering(USubstanceGraphInstance* GraphInstance)
{
	if (!GraphInstance || !GraphInstance->Instance)
	{
		return;
	}

	Substance::Helpers::RenderSync(GraphInstance->Instance);
	Substance::List<graph_inst_t*> Instances;
	Instances.AddUnique(GraphInstance->Instance);
	Substance::Helpers::UpdateTextures(Instances);
}


void USubstanceUtility::AsyncRendering(USubstanceGraphInstance* GraphInstance)
{
	if (!GraphInstance || !GraphInstance->Instance)
	{
		return;
	}

	Substance::Helpers::RenderAsync(GraphInstance->Instance);
}

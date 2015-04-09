//! @file SubstanceStructures.cpp
//! @author Antoine Gonzalez - Allegorithmic
//! @date 20110105
//! @copyright Allegorithmic. All rights reserved.

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceCoreTypedefs.h"
#include "SubstanceFGraph.h"
#include "SubstanceFPackage.h"
#include "SubstanceInput.h"
#include "SubstanceImageInput.h"
#include "substance_public.h"

namespace Substance
{

FGraphDesc::~FGraphDesc()
{
	OutputDescs.Empty();
	InputDescs.Empty();

	Substance::List<graph_inst_t*> InstancesToDelete(LoadedInstances);

	// the graph can be deleted before their instance
	Substance::List<graph_inst_t*>::TIterator ItInst(InstancesToDelete.itfront());

	for (;ItInst;++ItInst)
	{
		UnSubscribe(*ItInst);
	}
	
	Parent = NULL;
}


input_hash_t FGraphDesc::getDefaultHeavyInputHash() const
{
	input_hash_t Hash;

	for (uint32 Idx=0 ; Idx<InputDescs.size() ; ++Idx)
	{
		if (InputDescs[Idx]->IsHeavyDuty)
		{
			switch((SubstanceInputType)InputDescs[Idx]->Type)
			{
			case Substance_IType_Float:
				{
					FNumericalInputDesc<float>* Input = 
						(FNumericalInputDesc<float>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%f;"), 
						Input->DefaultValue);
				}
				break;
			case Substance_IType_Float2:
				{
					FNumericalInputDesc<vec2float_t>* Input = 
						(FNumericalInputDesc<vec2float_t>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%f%f;"),
						Input->DefaultValue.X,
						Input->DefaultValue.Y);
				}
				break;
			case Substance_IType_Float3:
				{
					FNumericalInputDesc<vec3float_t>* Input = 
						(FNumericalInputDesc<vec3float_t>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%f%f%f;"),
						Input->DefaultValue.X,
						Input->DefaultValue.Y,
						Input->DefaultValue.Z);
				}
				break;
			case Substance_IType_Float4:
				{
					FNumericalInputDesc<vec4float_t>* Input = 
						(FNumericalInputDesc<vec4float_t>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%f%f%f%f;"),
						Input->DefaultValue.X,
						Input->DefaultValue.Y,
						Input->DefaultValue.Z,
						Input->DefaultValue.W);
				}
				break;
			case Substance_IType_Integer:
				{
					FNumericalInputDesc<int32>* Input = 
						(FNumericalInputDesc<int32>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%d;"), 
						(int32)Input->DefaultValue);
				}
				break;
			case Substance_IType_Integer2:
				{
					FNumericalInputDesc<vec2int_t>* Input = 
						(FNumericalInputDesc<vec2int_t>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%d%d;"),
						(int32)Input->DefaultValue.X,
						(int32)Input->DefaultValue.Y);
				}
				break;
			case Substance_IType_Integer3:
				{
					FNumericalInputDesc<vec3int_t>* Input = 
						(FNumericalInputDesc<vec3int_t>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%d%d%d;"),
						(int32)Input->DefaultValue.X,
						(int32)Input->DefaultValue.Y,
						(int32)Input->DefaultValue.Z);
				}
				break;
			case Substance_IType_Integer4:
				{
					FNumericalInputDesc<vec4int_t>* Input = 
						(FNumericalInputDesc<vec4int_t>*)InputDescs[Idx].Get();

					Hash += FString::Printf(
						TEXT("%d%d%d%d;"),
						(int32)Input->DefaultValue.X,
						(int32)Input->DefaultValue.Y,
						(int32)Input->DefaultValue.Z,
						(int32)Input->DefaultValue.W);
				}
				break;
			case Substance_IType_Image:
				{
					FImageInputInstance* Input = 
						(FImageInputInstance*)InputDescs[Idx].Get();

					if (Input->ImageSource != NULL && Input->ImageSource->IsPendingKill())
					{
						Hash += Input->ImageSource->GetFullName();
					}
					else
					{
						Hash += TEXT("NoImage;");
					}
				}
				break;
			default:
				break;
			}
		}
	}

	return Hash;
}


FOutputDesc* FGraphDesc::GetOutputDesc(const uint32 Uid)
{
	for (auto itOut = OutputDescs.itfront(); itOut; ++itOut)
	{
		if (Uid == itOut->Uid)
		{
			return &(*itOut);
		}
	}
	
	return NULL;
}


input_desc_ptr FGraphDesc::GetInputDesc(const uint32 Uid)
{
	Substance::List<input_desc_ptr>::TIterator 
		ItIn(InputDescs.itfront());

	for (; ItIn ; ++ItIn)
	{
		if (Uid == (*ItIn)->Uid)
		{
			return *ItIn;
		}
	}

	return input_desc_ptr();
}


graph_inst_t* FGraphDesc::Instantiate(
	USubstanceGraphInstance* Parent, 
	bool bCreateOutputs,
	bool bSubscribeInstance,
	bool bDynamicInstance)
{
	graph_inst_t* NewInstance = new graph_inst_t(this, Parent);
	NewInstance->bIsFreezed = !bDynamicInstance;

	// register the new instance to the desc
	InstanceUids.AddUnique(NewInstance->InstanceGuid);

	if (bCreateOutputs)
	{
		Helpers::CreateTextures(NewInstance);
	}

	if (bSubscribeInstance)
	{
		Subscribe(NewInstance);
	}

	return NewInstance;
}


void FGraphDesc::Subscribe(graph_inst_t* Inst)
{
	int32 DummyIdx = INDEX_NONE;
	check(false == LoadedInstances.FindItem(Inst, DummyIdx));
	check(INDEX_NONE == DummyIdx);

	LoadedInstances.push(Inst);

	if (InstanceUids.FindItem(Inst->InstanceGuid, DummyIdx))
	{
		InstanceUids.AddUnique(Inst->InstanceGuid);
	}

	Parent->InstanceSubscribed();
}


void FGraphDesc::UnSubscribe(graph_inst_t* Inst)
{
	check(this == Inst->Desc);

	LoadedInstances.Remove(Inst);
	Parent->InstanceUnSubscribed();
	Inst->Desc = 0;

	// flag the instance container for deletion if it still exists
	if (Inst->ParentInstance)
	{
		Inst->ParentInstance->ClearFlags(RF_Standalone);
	}
}


struct FCompareUIDS
{
	FORCEINLINE bool operator()(const TKeyValuePair<uint32, uint32> A, const TKeyValuePair<uint32, uint32> B) const
	{
		return A.Key < B.Key;
	}
};


void FGraphDesc::commitOutputs()
{
	check(SortedOutputs.Num() == 0);
	SortedOutputs.Empty(OutputDescs.Num());

	for (auto itOut = OutputDescs.itfrontconst(); itOut; ++itOut)
	{
		SortedOutputs.Add(TKeyValuePair<uint32, uint32>(
			itOut->Uid, SortedOutputs.Num()));
	}

	SortedOutputs.Sort(FCompareUIDS());
}


void FGraphDesc::commitInputs()
{
	check(SortedInputs.Num() == 0);
	SortedInputs.Empty(InputDescs.Num());

	for (auto itIn = InputDescs.itfrontconst(); itIn; ++itIn)
	{
		SortedInputs.Add(TKeyValuePair<uint32, uint32>(
			(*itIn)->Uid, SortedInputs.Num()));
	}

	SortedInputs.Sort(FCompareUIDS());
}


template< typename T > TSharedPtr<input_inst_t> copyInputInstance(
	TSharedPtr<input_inst_t> Input)
{
	TSharedPtr<input_inst_t> Instance = 
		TSharedPtr<input_inst_t>(new FNumericalInputInstance<T>);

	*(FNumericalInputInstance<T>*)Instance.Get() =
		*(FNumericalInputInstance<T>*)Input.Get();

	return Instance;
}


int32 SetImageInputHelper(
	input_desc_t* InputDesc,
	UObject* InValue,
	FGraphInstance* Instance)
{
	int32 ModifiedOuputs = 0;
	input_inst_t* InputInst = NULL;

	for (auto ItInp = Instance->Inputs.itfront(); ItInp; ++ItInp)
	{
		if ((*ItInp)->Uid == InputDesc->Uid)
		{
			InputInst = ItInp->Get();
			break;
		}
	}

	check(InputInst);
	check(false == InputInst->IsNumerical());

	static_cast<img_input_inst_t*>(InputInst)->SetImageInput(InValue, Instance);

	if (!Instance->bHasPendingImageInputRendering)
	{
		return 0;
	}

	for (uint32 Idx = 0; Idx < InputDesc->AlteredOutputUids.size(); ++Idx)
	{
		output_inst_t* OutputModified = Instance->GetOutput(InputDesc->AlteredOutputUids[Idx]);

		if (OutputModified && OutputModified->bIsEnabled)
		{
			OutputModified->flagAsDirty();
			(*OutputModified->Texture)->MarkPackageDirty();
			++ModifiedOuputs;
		}
	}

	if (ModifiedOuputs)
	{
		Instance->ParentInstance->MarkPackageDirty();
		return ModifiedOuputs;
	}

	return 0;
}


FGraphInstance::FGraphInstance(
	FGraphDesc* GraphDesc,
	USubstanceGraphInstance* Parent):
        ParentUrl(GraphDesc->PackageUrl),
		Desc(GraphDesc),
		ParentInstance(Parent),
        bIsFreezed(true),
        bIsBaked(false),
	bHasPendingImageInputRendering(false)
{
	check(ParentInstance);
	check(GraphDesc);
	check(GraphDesc->Parent);
	check(ParentInstance->Instance == 0);

	for (auto itOut = GraphDesc->OutputDescs.itfrontconst(); itOut; ++itOut)
	{
		Outputs.push(itOut->Instantiate());
		Outputs.Last().ParentInstance = ParentInstance;
	}

	for (auto ItIn = GraphDesc->InputDescs.itfront(); ItIn; ++ItIn)
	{
		Inputs.push(
			TSharedPtr<input_inst_t>(
				(*ItIn)->Instantiate()));
		Inputs.Last().Get()->Parent = this;
	}

	InstanceGuid = FGuid::NewGuid();
	ParentInstance->Instance = this;
	ParentInstance->Parent = GraphDesc->Parent->Parent;
}


FGraphInstance::~FGraphInstance()
{
	Outputs.Empty();
	Inputs.Empty();

	Desc = NULL;

	// Notify all renderers that this instance will be deleted
	SBS_VECTOR_FOREACH (Details::States* states,States)
	{
		states->notifyDeleted(InstanceGuid);
	}
}


int32 FGraphInstance::UpdateInput(
	const uint32& Uid,
	class UObject* InValue)
{
	graph_desc_t* ParentGraph = Outputs[0].GetParentGraph();
	int32 ModifiedOuputs = 0;

	// special case for reimported image inputs
	if (Uid == 0)
	{
		for (auto ItIn = Inputs.itfront(); ItIn; ++ItIn)
		{
			if (!(*ItIn)->IsNumerical())
			{
				if (((FImageInputInstance*)ItIn->Get())->ImageSource == InValue)
				{
					ModifiedOuputs += SetImageInputHelper(ItIn->Get()->Desc, InValue, this);
				}
			}
		}

		return ModifiedOuputs;
	}
	
	for (auto ItInDesc = ParentGraph->InputDescs.itfront(); ItInDesc; ++ItInDesc)
	{
		if ((*ItInDesc)->Uid == Uid)
		{
			ModifiedOuputs += SetImageInputHelper((*ItInDesc).Get(), InValue, this);
		}
	}

	return ModifiedOuputs;
}


int32 FGraphInstance::UpdateInput(
	const FString& ParameterName,
	class UObject* InValue)
{
	graph_desc_t* ParentGraph = Outputs[0].GetParentGraph();
	List<input_desc_ptr>::TIterator 
		ItIn(ParentGraph->InputDescs.itfront());

	int32 ModifiedOuputs = 0;

	for ( ; ItIn ; ++ItIn)
	{
		if ((*ItIn)->Identifier == ParameterName)
		{
			ModifiedOuputs += SetImageInputHelper((*ItIn).Get(), InValue, this);
		}
	}

	return ModifiedOuputs;
}

	
output_inst_t* FGraphInstance::GetOutput(const uint32 Uid)
{
	for (int32 Idx=0 ; Idx<Outputs.Num() ; ++Idx)
	{
		if (Uid == Outputs[Idx].Uid)
		{
			return &Outputs[Idx];
		}
	}
	return NULL;
}


input_inst_t* FGraphInstance::GetInput(const FString& Name)
{
	for (int32 Idx=0 ; Idx<Inputs.Num() ; ++Idx)
	{
		if (Name == Inputs[Idx]->Desc->Identifier)
		{
			return Inputs[Idx].Get();
		}
	}
	return NULL;
}


input_inst_t* FGraphInstance::GetInput(const uint32 Uid)
{
	for (int32 Idx=0 ; Idx<Inputs.Num() ; ++Idx)
	{
		if (Uid == Inputs[Idx]->Uid)
		{
			return Inputs[Idx].Get();
		}
	}
	return NULL;
}


void FGraphInstance::plugState(Details::States* states)
{
	check(std::find(States.begin(),States.end(),states)==States.end());
	States.push_back(states);
}


void FGraphInstance::unplugState(Details::States* states)    
{
	if (States.size())
	{
		States_t::iterator ite = std::find(States.begin(),States.end(),states);
		check(ite!=States.end());
		States.erase(ite);
	}
}

} // namespace Substance

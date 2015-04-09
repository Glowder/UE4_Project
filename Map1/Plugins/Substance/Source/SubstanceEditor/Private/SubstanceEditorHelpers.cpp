//! @file SubstanceHelpers.cpp
//! @author Antoine Gonzalez - Allegorithmic
//! @date 20110105
//! @copyright Allegorithmic. All rights reserved.

#include "SubstanceEditorPrivatePCH.h"

#include <UnrealEd.h>
#include <Factories.h>
#include <MainFrame.h>
#include <DesktopPlatformModule.h>

#include "SubstanceEditorHelpers.h"

#include "SubstanceCoreClasses.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceCorePreset.h"
#include "SubstanceFPackage.h"
#include "SubstanceFGraph.h"

#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

using namespace Substance;

namespace SubstanceEditor
{
namespace Helpers
{

void CreateMaterialExpression(
	output_inst_t* OutputInst,
	output_desc_t* OutputDesc,
	UMaterial* UnrealMaterial);


//! @brief Create an Unreal Material for the given graph-instance
UMaterial* CreateMaterial(graph_inst_t* GraphInstance,
	const FString & MaterialName,
	UObject* Outer,
	bool bFocusInObjectBrowser)
{
	// create an unreal material asset
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

	UMaterial* UnrealMaterial = 
		(UMaterial*)MaterialFactory->FactoryCreateNew(
			UMaterial::StaticClass(),
			Substance::Helpers::CreateObjectPackage(Outer, MaterialName),
			*MaterialName, 
			RF_Standalone|RF_Public, NULL, GWarn );

	Substance::List<output_inst_t>::TIterator 
		ItOut(GraphInstance->Outputs.itfront());

	// textures and properties
	for ( ; ItOut ; ++ItOut)
	{
		output_inst_t* OutputInst = &(*ItOut);
		output_desc_t* OutputDesc = GraphInstance->Desc->GetOutputDesc(
			OutputInst->Uid);

		CreateMaterialExpression(
			OutputInst,
			OutputDesc,
			UnrealMaterial);
	}

	//remove any memory copies of shader files, so they will be reloaded from disk
	//this way the material editor can be used for quick shader iteration
	FlushShaderFileCache();

	// let the material update itself if necessary
	UnrealMaterial->PreEditChange(NULL);
	UnrealMaterial->PostEditChange();
	
	return UnrealMaterial;
}


void CreateMaterialExpression(output_inst_t* OutputInst,
							  output_desc_t* OutputDesc,
							  UMaterial* UnrealMaterial)
{
	FExpressionInput * MaterialInput = NULL;

	switch(OutputDesc->Channel)
	{
	case CHAN_BaseColor:
	case CHAN_Diffuse:
		MaterialInput = &UnrealMaterial->BaseColor;
		break;

	case CHAN_Metallic:
		MaterialInput = &UnrealMaterial->Metallic;
		break;

	case CHAN_Specular:
		MaterialInput = &UnrealMaterial->Specular;
		break;

	case CHAN_Roughness:
		MaterialInput = &UnrealMaterial->Roughness;
		break;

	case CHAN_Emissive:
		MaterialInput = &UnrealMaterial->EmissiveColor;
		break;

	case CHAN_Normal:
		MaterialInput = &UnrealMaterial->Normal;
		break;

	case CHAN_Mask:
		MaterialInput = &UnrealMaterial->OpacityMask;
		break;

	case CHAN_Opacity:
		MaterialInput = &UnrealMaterial->Opacity;
		break;

	case CHAN_Refraction:
		MaterialInput = &UnrealMaterial->Refraction;
		break;

	case CHAN_AmbientOcclusion:
		MaterialInput = &UnrealMaterial->AmbientOcclusion;
		break;

	default:
		// nothing relevant to plug, skip it
		return;
		break;
	}

	UTexture* UnrealTexture = *OutputInst->Texture;

	if (UnrealTexture)
	{
		// and link it to the material 
		UMaterialExpressionTextureSampleParameter2D* UnrealTextureExpression =
			ConstructObject<UMaterialExpressionTextureSampleParameter2D>(
			UMaterialExpressionTextureSampleParameter2D::StaticClass(),
			UnrealMaterial );

		UnrealTextureExpression->MaterialExpressionEditorX = -200;
		UnrealTextureExpression->MaterialExpressionEditorY = UnrealMaterial->Expressions.Num() * 180;

		UnrealMaterial->Expressions.Add( UnrealTextureExpression );
		MaterialInput->Expression = UnrealTextureExpression;
		UnrealTextureExpression->Texture = UnrealTexture;
		UnrealTextureExpression->ParameterName = *OutputDesc->Identifier;
		UnrealTextureExpression->SamplerType = UnrealTextureExpression->GetSamplerTypeForTexture( UnrealTexture );
	}

	if (MaterialInput->Expression)
	{
		TArray<FExpressionOutput> Outputs;
		Outputs = MaterialInput->Expression->GetOutputs();
		FExpressionOutput* Output = &Outputs[0];
		MaterialInput->Mask = Output->Mask;
		MaterialInput->MaskR = Output->MaskR;
		MaterialInput->MaskG = Output->MaskG;
		MaterialInput->MaskB = Output->MaskB;
		MaterialInput->MaskA = Output->MaskA;
	}
}


static void* ChooseParentWindowHandle()
{
	void* ParentWindowWindowHandle = NULL;
	/*FMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<FMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
	}*/

	return ParentWindowWindowHandle;
}


/**
	* @param Title                  The title of the dialog
	* @param FileTypes              Filter for which file types are accepted and should be shown
	* @param InOutLastPath          Keep track of the last location from which the user attempted an import
	* @param DefaultFile            Default file name to use for saving.
	* @param OutOpenFilenames       The list of filenames that the user attempted to open
	*
	* @return true if the dialog opened successfully and the user accepted; false otherwise.
	*/
bool SaveFile( const FString& Title, const FString& FileTypes, FString& InOutLastPath, const FString& DefaultFile, FString& OutFilename )
{
	OutFilename = FString();

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bFileChosen = false;
	TArray<FString> OutFilenames;
	if (DesktopPlatform)
	{
		void* ParentWindowWindowHandle = ChooseParentWindowHandle();

		bFileChosen = DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
			Title,
			InOutLastPath,
			DefaultFile,
			FileTypes,
			EFileDialogFlags::None,
			OutFilenames
		);
	}

	bFileChosen = (OutFilenames.Num() > 0);

	if (bFileChosen)
	{
		// User successfully chose a file; remember the path for the next time the dialog opens.
		InOutLastPath = OutFilenames[0];
		OutFilename = OutFilenames[0];
	}

	return bFileChosen;
}

/**
	* @param Title                  The title of the dialog
	* @param FileTypes              Filter for which file types are accepted and should be shown
	* @param InOutLastPath    Keep track of the last location from which the user attempted an import
	* @param DialogMode             Multiple items vs single item.
	* @param OutOpenFilenames       The list of filenames that the user attempted to open
	*
	* @return true if the dialog opened successfully and the user accepted; false otherwise.
	*/
bool OpenFiles( const FString& Title, const FString& FileTypes, FString& InOutLastPath, EFileDialogFlags::Type DialogMode, TArray<FString>& OutOpenFilenames ) 
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bOpened = false;
	if ( DesktopPlatform )
	{
		void* ParentWindowWindowHandle = ChooseParentWindowHandle();

		bOpened = DesktopPlatform->OpenFileDialog(
			ParentWindowWindowHandle,
			Title,
			InOutLastPath,
			TEXT(""),
			FileTypes,
			DialogMode,
			OutOpenFilenames
		);
	}

	bOpened = (OutOpenFilenames.Num() > 0);

	if ( bOpened )
	{
		// User successfully chose a file; remember the path for the next time the dialog opens.
		InOutLastPath = OutOpenFilenames[0];
	}

	return bOpened;
}


void SavePresetFile(preset_t& Preset)
{
	/*WxFileDialog ExportFileDialog( NULL, 
		TEXT("Export Preset"),
		*(GApp->LastDir[LD_GENERIC_EXPORT]),
		TEXT(""),
		TEXT("Export Types (*.sbsprs)|*.sbsprs;|All Files|*.*"),
		wxSAVE | wxOVERWRITE_PROMPT,
		wxDefaultPosition);

	FString PresetContent;

	// Display the Open dialog box.
	if( ExportFileDialog.ShowModal() == wxID_OK )
	{
		FString Path(ExportFileDialog.GetPath());
		GApp->LastDir[LD_GENERIC_EXPORT] = Path;

		FString PresetContent;
		Substance::WritePreset(Preset, PresetContent);
		appSaveStringToFile(PresetContent, *Path);
	}*/
}


bool ExportPresetFromGraph(USubstanceGraphInstance* GraphInstance)
{
	check(GraphInstance != NULL);
	preset_t Preset;
	FString Data;
	
	Preset.ReadFrom(GraphInstance->Instance);
	WritePreset(Preset, Data);
	FString PresetFileName = SubstanceEditor::Helpers::ExportPresetFile(Preset.mLabel);
	if (PresetFileName.Len())
	{
		return FFileHelper::SaveStringToFile(Data, *PresetFileName);
	}

	return false;
}


bool ImportAndApplyPresetForGraph(USubstanceGraphInstance* GraphInstance)
{
	FString Data;
	FString PresetFileName;
	presets_t Presets;
	
	PresetFileName = SubstanceEditor::Helpers::ImportPresetFile();
	FFileHelper::LoadFileToString(Data, *PresetFileName);
	ParsePresets(Presets, Data);
	for (presets_t::TIterator ItPres = Presets.CreateIterator(); ItPres; ++ItPres)
	{
		preset_t Preset = *ItPres;

		// apply it [but which one ? for now that will be the last one]
		if (Preset.Apply(GraphInstance->Instance))
		{
			Substance::Helpers::RenderAsync(GraphInstance->Instance);
			return true;
		}
	}
	
	return false;
}


FString ExportPresetFile(FString SuggestedFilename)
{
        FString OutOpenFilename;
        FString InOutLastPath = ".";
        if ( SaveFile( TEXT("Export Preset"), TEXT("Export Types (*.sbsprs)|*.sbsprs;|All Files|*.*"),
		       InOutLastPath,
			   SuggestedFilename + ".sbsprs",
		       OutOpenFilename )
             )
                 return OutOpenFilename;
        else
                 return FString();
}

FString ImportPresetFile()
{
        TArray<FString> OutOpenFilenames;
        FString InOutLastPath = ".";
        if ( OpenFiles( TEXT("Import Preset"), TEXT("Import Types (*.sbsprs)|*.sbsprs;|All Files|*.*"),
                        InOutLastPath,
                        EFileDialogFlags::None, //single selection
                        OutOpenFilenames )
             )
                 return OutOpenFilenames[0];
        else
                 return FString();
}


void CreateImageInput(UTexture2D* Texture)
{
/*	check(Texture != NULL);

	FString GroupName;
	FString PackageName = Texture->GetOutermost()->GetName();

	//! @todo: look for an unused name
	FString InstanceName = FString::Printf( TEXT("%s_IMG"), *Texture->GetName());*/

}


// map containing a initial values (sbsprs) of each modified graph instance
// used when loading a kismet sequence
TMap< graph_inst_t*, FString > GInitialGraphValues;


void SaveInitialValues(graph_inst_t *Graph)
{
	if (NULL == GInitialGraphValues.Find(Graph))
	{	
		FString PresetValue;
		preset_t InitialValuesPreset;
		InitialValuesPreset.ReadFrom(Graph);
		WritePreset(InitialValuesPreset, PresetValue);
		GInitialGraphValues.Add(Graph, PresetValue);
	}
}


void RestoreGraphInstances()
{
	TMap< graph_inst_t*, FString >::TIterator ItSavedPreset(GInitialGraphValues);

	for (; ItSavedPreset ; ++ItSavedPreset)
	{
		presets_t Presets;
		ParsePresets(Presets, ItSavedPreset.Value());

		if (Presets[0].Apply(ItSavedPreset.Key()))
		{
			Substance::Helpers::PushDelayedRender(ItSavedPreset.Key());
		}
	}
}


void FullyLoadInstance(USubstanceGraphInstance* Instance)
{
	Instance->GetOutermost()->FullyLoad();

	Substance::List<output_inst_t>::TIterator 
		ItOut(Instance->Instance->Outputs.itfront());

	for(; ItOut ; ++ItOut)
	{
		if(*((*ItOut).Texture).get())
		{
			(*((*ItOut).Texture).get())->GetOutermost()->FullyLoad();
		}
	}
}

} // namespace Helpers
} // namespace Substance

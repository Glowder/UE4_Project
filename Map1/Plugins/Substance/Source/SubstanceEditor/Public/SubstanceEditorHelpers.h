//! @file SubstanceEditorHelpers.h
// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SubstanceCoreTypedefs.h"

namespace SubstanceEditor
{
	//! @brief Contains helper functions to call from the main thread
	namespace Helpers
	{
		//! @brief Create an Unreal Material for the given graph-instance
		//! @brief It will be created in the same outer package as the GraphInstance
		UMaterial* CreateMaterial(
			graph_inst_t* GraphInstance, 
			const FString& MatName,
			UObject* Outer = NULL,
			bool bFocusInObjectBrowser = true);

		void CreateImageInput(UTexture2D* Texture);

		bool ExportPresetFromGraph(USubstanceGraphInstance*);
		bool ImportAndApplyPresetForGraph(USubstanceGraphInstance*);

		FString ImportPresetFile();
		FString ExportPresetFile(FString SuggestedFilename);

		void SavePresetFile(preset_t& Preset);

		void SaveInitialValues(graph_inst_t *Graph);

		void RestoreGraphInstances();

		void FullyLoadInstance(USubstanceGraphInstance* Instance);
	} // namespace Helpers
} // namespace Substance

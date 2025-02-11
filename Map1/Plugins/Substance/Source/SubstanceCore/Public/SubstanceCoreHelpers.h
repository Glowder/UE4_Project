// Copyright 2013 Allegorithmic. All Rights Reserved.
#pragma once

#include "framework/imageinput.h"

#include "substance_public.h"

class USubstanceInstanceFactory;
class USubstanceGraphInstance;
class USubstanceImageInput;
class USubstanceTexture2D;
class UObject;

#include "SubstanceFOutput.h"

namespace Substance
{
	struct FImageInputInstance;

	//! @brief Contains helper functions to call from the main thread
	namespace Helpers
	{
		//! @brief Perform an asynchronous (non-blocking) rendering of those instances
		SUBSTANCECORE_API void RenderAsync(Substance::List<graph_inst_t*>&);
		SUBSTANCECORE_API void RenderAsync(graph_inst_t*);

		//! @brief Perform a blocking rendering of those instances
		SUBSTANCECORE_API void RenderSync(Substance::List<graph_inst_t*>&);
		SUBSTANCECORE_API void RenderSync(graph_inst_t*);
		
		void PostLoadRender();
		
		SUBSTANCECORE_API void StartPIE();

		SUBSTANCECORE_API void QueueForRendering(graph_inst_t*);

		//! @brief Subscribe the graph instance for rendering, without rendering
		void RenderPush(graph_inst_t*);

		//! @brief Queue the graph instance for later render
		//! @note Used during seekfreeloading
		SUBSTANCECORE_API void PushDelayedRender(graph_inst_t*);

		//! @brief Queue an image input until its source has been rendered
		//! @note This should be improve to handle recursive image input 
		void PushDelayedImageInput(img_input_inst_t* ImgInput, graph_inst_t* Instance);

		//! @brief Render queued graph instances
		void PerformDelayedRender();

		//! @brief Update Texture Output
		void UpdateTexture(const SubstanceTexture& result, output_inst_t* Output, bool bCacheResults = true);

		//! @brief Perform per frame Substance management
		SUBSTANCECORE_API void Tick();

		//! @brief Cancel pending renders etc.
		void CancelPendingActions();

		//! @brief Create a texture 2D object using an output instance desc.
		SUBSTANCECORE_API void CreateSubstanceTexture2D(FOutputInstance* OutputInstance, bool bTransient = false, FString Name = FString(), UObject* InOuter=NULL);

		SUBSTANCECORE_API void CreatePlaceHolderMips(graph_inst_t* Instance);

		//! @brief used to create a graph instance object
		USubstanceGraphInstance* CreateGraphInstanceContainer(
			UObject* Parent,
			const FString& BaseName, 
			bool CreateSubPackage=true);

		//! @brief Convert a PixelFormat to the closest supported PixelFormat
		//! @note If adding support to a new type, update SubstanceToUe3Format function
		SUBSTANCECORE_API SubstancePixelFormat ValidateFormat(const SubstancePixelFormat Format);

		//! @brief Convert from a Substance Format to UE3 equivalent
		//! @pre The input format is a format supported by the UE3
		//! @return The equivalent in UE3 enum or PF_Unknown if not supported
		EPixelFormat SubstanceToUe3Format(const SubstancePixelFormat Format);

		//! @brief Convert from a UE3 pixel format to a Substance Format
		SubstancePixelFormat Ue3FormatToSubstance(EPixelFormat Fmt);
		
		//! @brief Tell if the UE3 pixel format is supported by Substance
		//! @note Used for image inputs.
		bool IsUe3FormatSupported(EPixelFormat Fmt);

		//! @brief Create textures of empty output instances of the given graph instance
		void CreateTextures(graph_inst_t* GraphInstance);

		//! @brief Update Instances's outputs
		SUBSTANCECORE_API bool UpdateTextures(Substance::List<FGraphInstance*>& Instances);

		//! @brief Compare the values of two input instances.
		//! @todo implement this as a comparison operator ?
		//! @return True is the values are the same.
		bool AreInputValuesEqual(
			TSharedPtr<input_inst_t>&,
			TSharedPtr<input_inst_t>&);

		//! @brief Return the string representation of the Input's value
		SUBSTANCECORE_API FString GetValueString(const TSharedPtr< input_inst_t >& Input);

		TArray< float > GetValueFloat(const TSharedPtr< input_inst_t >& Input);

		TArray< int32 > GetValueInt(const TSharedPtr< input_inst_t >& Input);

		//! @brief Setup the rendering system for commandlet or things like that
		void SetupSubstance();

		//! @brief Opposite of SetupSubstance
		void TearDownSubstance();

		//! @brief Disable an output
		//! @pre It is ok to delete the texture, i.e. it is not used
		//! @port The output's texture is deleted
		SUBSTANCECORE_API void Disable(output_inst_t*);

		//! @brief Clears the texture properly
		void Clear(std::shared_ptr<USubstanceTexture2D*>& otherTexture);

		//! @brief Find which desc an instance belongs to
		graph_desc_t* FindParentGraph(
			Substance::List<graph_desc_t*>& Graphs,
			graph_inst_t*);

		graph_desc_t* FindParentGraph(
			Substance::List<graph_desc_t*>& Graphs,
			const FString& URL);

		//! @brief Decompress a jpeg buffer in RAW RGBA
		SUBSTANCECORE_API void DecompressJpeg(
			const void* Buffer,
			const int32 Length,
			TArray<uint8>& outRawData,
			int32* outW,
			int32* outH, 
			int32 NumComp=4);

		//! @brief Compress a jpeg buffer in RAW RGBA
		SUBSTANCECORE_API void CompressJpeg(
			const void* InBuffer, const int32 InLength, 
			const int32 W, const int32 H, const int32 NumComp,
			TArray<uint8>& CompressedImage);

		std::shared_ptr<ImageInput> PrepareImageInput(
			class UObject* InValue, 
			FImageInputInstance* Input,
			FGraphInstance* Parent);

		bool IsSupportedImageInput(UObject*);

		void LinkImageInput(img_input_inst_t* ImgInputInst,
			USubstanceImageInput* SrcImageInput);
		
		void ClearFromRender(USubstanceGraphInstance* GraphInstance);

		void Cleanup(USubstanceGraphInstance*);

		//! @brief Reset an instance's inputs to their default values
		//! @note Does no trigger rendering of the instance
		SUBSTANCECORE_API void ResetToDefault(graph_inst_t*);

		void RegisterOutputAsImageInput(USubstanceGraphInstance* G, USubstanceTexture2D* T);
		void UnregisterOutputAsImageInput(USubstanceTexture2D* T, bool bUnregisterAll, bool bUpdateGraphInstance);
		void UnregisterOutputAsImageInput(USubstanceGraphInstance* Graph);

		SUBSTANCECORE_API void GetSuitableName(output_inst_t* Instance,
			FString& OutTextureName, 
			FString& OutPackageName);

		//! @brief Create instance of a graph desc
		SUBSTANCECORE_API graph_inst_t* InstantiateGraph(
			graph_desc_t* Graph,
			UObject* Outer,
			FString InstanceName = FString(),
			bool bCreateOutputs = false,
			EObjectFlags Flags = RF_Standalone | RF_Public);

		USubstanceGraphInstance* DuplicateGraphInstance(
			USubstanceGraphInstance* RefInstance, 
			bool bCopyOutputs = true);

		void CopyInstance( 
			graph_inst_t* RefInstance,
			graph_inst_t* NewInstance,
			bool bCopyOutputs = true);

		SUBSTANCECORE_API void RegisterForDeletion( class USubstanceGraphInstance* InstanceContainer );

		SUBSTANCECORE_API void RegisterForDeletion( class USubstanceTexture2D* InstanceContainer );

		SUBSTANCECORE_API void RegisterForDeletion( class USubstanceInstanceFactory* InstanceContainer );

		SUBSTANCECORE_API void PerformDelayedDeletion();

		SUBSTANCECORE_API UPackage* CreateObjectPackage(UObject* Outer, FString ObjectName);

		float GetSubstanceLoadingProgress();

	} // namespace Helpers
} // namespace Substance

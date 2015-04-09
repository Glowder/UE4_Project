//! @file SubstanceCache.cpp
//! @brief Substance Cache Data
//! @date 20140731
//! @copyright Allegorithmic. All rights reserved.
#include "SubstanceCorePrivatePCH.h"
#include "SubstanceCache.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceTexture2D.h"
#include "SubstanceFGraph.h"

#include "Paths.h"

#define SUBSTANCECACHE_VERSION 1

DEFINE_LOG_CATEGORY_STATIC(LogSubstanceCache, Log, All);

using namespace Substance;

TSharedPtr<SubstanceCache> SubstanceCache::SbsCache;

bool SubstanceCache::CanReadFromCache(FGraphInstance* graph)
{
	bool canReadFromCache = false;
	//make sure all enabled outputs have a cached file
	{
		auto iter = graph->Outputs.itfront();
		while (iter)
		{
			if ((*iter).bIsEnabled)
			{
				FString path = GetPathForGuid((*iter).OutputGuid);

				if (IFileManager::Get().FileSize(*path) == INDEX_NONE)
				{
					return false;
				}
				else
				{
					canReadFromCache = true;
				}
			}

			iter++;
		}
	}

	return canReadFromCache;
}

bool SubstanceCache::ReadFromCache(FGraphInstance* graph)
{
	if (!CanReadFromCache(graph))
	{
		return false;
	}

	//read in cached data and upload
	auto iter = graph->Outputs.itfront();
	while (iter)
	{
		if ((*iter).bIsEnabled)
		{
			FOutputInstance& outputInstance = *(iter);
			FString path = GetPathForGuid(outputInstance.OutputGuid);
			FArchive* Ar = IFileManager::Get().CreateFileReader(*path);

			if (Ar)
			{
				SubstanceTexture output;
				FMemory::MemZero(output);

				if (SerializeTexture(*Ar, output))
				{
					delete Ar;

					USubstanceTexture2D* texture = *((outputInstance.Texture).get());
					Substance::Helpers::UpdateTexture(output, &outputInstance, false);

					FMemory::Free(output.buffer);
				}
				else
				{
					delete Ar;
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		iter++;
	}

	return true;
}

void SubstanceCache::CacheOutput(output_inst_t* output, const SubstanceTexture& result)
{
	FString filename = GetPathForGuid(output->OutputGuid);
	FArchive* Ar = IFileManager::Get().CreateFileWriter(*filename, 0);

	if (Ar)
	{
		SerializeTexture(*Ar, const_cast<SubstanceTexture&>(result));
		delete Ar;
	}
}

FString SubstanceCache::GetPathForGuid(const FGuid& guid) const
{
	return FString::Printf(TEXT("%s/Substance/%s.cache"), *FPaths::GameSavedDir(), *(guid.ToString()));
}

bool SubstanceCache::SerializeTexture(FArchive& Ar, SubstanceTexture& result) const
{
	uint32 version = SUBSTANCECACHE_VERSION;

	Ar << version;

	if (Ar.IsLoading())
	{
		if (version != SUBSTANCECACHE_VERSION)
		{
			UE_LOG(LogSubstanceCache, Warning, TEXT("Out of date Substance cache entry, will regenerate"));	
			return false;
		}
	}

	Ar << result.level0Width;
	Ar << result.level0Height;
	Ar << result.pixelFormat;
	Ar << result.channelsOrder;
	Ar << result.mipmapCount;

	EPixelFormat pixelFormat = Substance::Helpers::SubstanceToUe3Format((SubstancePixelFormat)result.pixelFormat);
	SIZE_T bufferSize = CalcTextureSize(result.level0Width, result.level0Height, pixelFormat, result.mipmapCount);

	if (Ar.IsLoading())
	{
		check(result.buffer == NULL);
		result.buffer = FMemory::Malloc(bufferSize);
	}
	
	Ar.Serialize(result.buffer, bufferSize);
	
	return true;
}

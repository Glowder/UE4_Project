// Copyright 2014 Allegorithmic All Rights Reserved.

#include "SubstanceCorePrivatePCH.h"
#include "SubstanceCoreHelpers.h"
#include "SubstanceCoreModule.h"

#include "SubstanceCoreClasses.h"

#include "AssetRegistryModule.h"
#include "ModuleManager.h"
#include "Ticker.h"

namespace
{
	static FWorldDelegates::FWorldInitializationEvent::FDelegate OnWorldInitDelegate;
	static FDelegateHandle OnWorldInitDelegateHandle;
}

void FSubstanceCoreModule::StartupModule()
{
	Substance::Helpers::SetupSubstance();

	// Register tick function.
	TickDelegate = FTickerDelegate::CreateRaw(this, &FSubstanceCoreModule::Tick);
	FTicker::GetCoreTicker().AddTicker( TickDelegate );

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	RegisterSettings();

	::OnWorldInitDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(&FSubstanceCoreModule::OnWorldInitialized);
	::OnWorldInitDelegateHandle = FWorldDelegates::OnPostWorldInitialization.Add(::OnWorldInitDelegate);
}

void FSubstanceCoreModule::ShutdownModule()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(::OnWorldInitDelegateHandle);

	UnregisterSettings();

	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	}

	Substance::Helpers::TearDownSubstance();
}

bool FSubstanceCoreModule::Tick(float DeltaTime)
{
	Substance::Helpers::Tick();

	return true;
}

void FSubstanceCoreModule::OnWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
	Substance::Helpers::PostLoadRender();
}

IMPLEMENT_MODULE( FSubstanceCoreModule, SubstanceCore );

// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"

#include "SubstanceCoreClasses.h"

/*-----------------------------------------------------------------------------
   ISubstanceEditor
-----------------------------------------------------------------------------*/

class ISubstanceEditor : public FAssetEditorToolkit
{
public:
	/** Returns the graph asset being edited */
	virtual USubstanceGraphInstance* GetGraph() const = 0;

};



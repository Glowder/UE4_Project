// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SubstanceEditorPrivatePCH.h"
#include "SubstanceEditorModule.h"
#include "SSubstanceEditorPanel.h"

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "ContentBrowserModule.h"
#include "MouseDeltaTracker.h"
#include "Slate/SceneViewport.h"
#include "SColorPicker.h"
#include "SNumericEntryBox.h"
#include "SExpandableArea.h"
#include "ScopedTransaction.h"

#define LOC_NAMESPACE TEXT("SubstanceEditor")

#define POW2_MIN (5)
#define POW2_MAX (11)

SSubstanceEditorPanel::~SSubstanceEditorPanel()
{
}


TWeakPtr<ISubstanceEditor> SSubstanceEditorPanel::GetSubstanceEditor() const
{
	return SubstanceEditorPtr;
}


void SSubstanceEditorPanel::Construct(const FArguments& InArgs)
{
	SubstanceEditorPtr = InArgs._SubstanceEditor;

	Graph = SubstanceEditorPtr.Pin()->GetGraph();
	if (!Graph->Parent ||
		!Graph->Instance)
	{
		// invalid instance, cannot edit
		// todo: add warning for user
		return;
	}

	ConstructDescription();
	ConstructOutputs();
	ConstructInputs();

	TSharedPtr<SVerticalBox> ChildWidget = 
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0.0f, 3.0f )
		[
			DescArea.ToSharedRef()
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 0.0f, 3.0f )
		[
			OutputsArea.ToSharedRef()
		];

	if (InputsArea.IsValid())
	{
		ChildWidget->AddSlot()
		.AutoHeight()
		.Padding( 0.0f, 3.0f )
		[
			InputsArea.ToSharedRef()
		];
	}

	if (ImageInputsArea.IsValid())
	{
		ChildWidget->AddSlot()
		.AutoHeight()
		.Padding( 0.0f, 3.0f )
		[
			ImageInputsArea.ToSharedRef()
		];
	}

	ChildSlot
	[
		SNew( SScrollBox )
		+ SScrollBox::Slot()
		.Padding( 0.0f )
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 0.0f )
			[
				ChildWidget.ToSharedRef()
			]
		]
	];
}


void SSubstanceEditorPanel::ConstructDescription()
{
	DescArea = SNew(SExpandableArea)
		.AreaTitle(FText::FromString(TEXT("Graph Description:")))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SBorder)
			.Content()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(0.1f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(0.3f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Label:")))
					]
					+SHorizontalBox::Slot()
						.Padding(0.1f)
						[
							SNew(STextBlock)
							.Text(Graph->Instance->Desc->Label)
						]
				]
				+SVerticalBox::Slot()
					.Padding(0.1f)
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(0.3f)				
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Description:")))
						]
						+SHorizontalBox::Slot()
							.Padding(0.1f)
							[
								SNew(STextBlock)
								.AutoWrapText(true)
								.Text(Graph->Instance->Desc->Description.Len() ? Graph->Instance->Desc->Description : TEXT("N/A"))
							]
					]
			]
		];
}


void SSubstanceEditorPanel::ConstructOutputs()
{
	TSharedPtr<SVerticalBox> OutputsBox = SNew(SVerticalBox);

	TArray<output_inst_t>::TIterator ItOut(Graph->Instance->Outputs.getArray());
	for (;ItOut;++ItOut)
	{
		FReferencerInformationList Refs;
		UObject* TextureObject = *ItOut->Texture;

		// determine whether the transaction buffer is the only thing holding a reference to the object
		// and if so, behave like it's not referenced and ask offer the user to clean up undo/redo history when he deletes the output
		GEditor->Trans->DisableObjectSerialization();
		bool bIsReferenced = TextureObject ? IsReferenced(TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, true, &Refs) : false;
		GEditor->Trans->EnableObjectSerialization();

		output_desc_t* OutputDesc = ItOut->GetOutputDesc();

		OutputsBox->AddSlot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(0.3f)
			[
				SNew(STextBlock)
				.Text(OutputDesc ? OutputDesc->Label : TEXT("Invalid output"))
			]
			+SHorizontalBox::Slot()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged( this, &SSubstanceEditorPanel::OnToggleOutput, &(*ItOut) )
				.IsChecked( this, &SSubstanceEditorPanel::GetOutputState, &(*ItOut) )
				.IsEnabled(!bIsReferenced && NULL != OutputDesc)
				.ToolTipText(OutputDesc->Identifier)
			]	
		];
	}

	OutputsArea = SNew(SExpandableArea)
		.AreaTitle(FText::FromString(TEXT("Outputs:")))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SBorder)
			.BorderBackgroundColor( FLinearColor(0.35, 0.35, 0.35) )
			.ForegroundColor( FEditorStyle::GetColor( "TableView.Header", ".Foreground" ) )
			.Content()
			[
				OutputsBox.ToSharedRef()
			]
		];
}


void ActualizeMaterialGraph(graph_inst_t* Instance)
{
	TArray<output_inst_t>::TIterator ItOut(Instance->Outputs.getArray());
	for (;ItOut;++ItOut)
	{
		FReferencerInformationList Refs;
		UObject* TextureObject = *ItOut->Texture;

		// Check and see whether we are referenced by any objects that won't be garbage collected. 
		bool bIsReferenced = TextureObject ? IsReferenced( TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, true, &Refs) : false;

		TArray<UMaterial*> RefMaterials;

		TArray<FReferencerInformation>::TIterator ItRef(Refs.ExternalReferences);
		for (;ItRef;++ItRef)
		{
			UMaterial* Material = Cast<UMaterial>(ItRef->Referencer);

			if (Material && Material->MaterialGraph)
			{
				RefMaterials.AddUnique(Material);
			}
		}

		TArray<UMaterial*>::TIterator ItMat(RefMaterials);
		for (;ItMat;++ItMat)
		{
			(*ItMat)->MaterialGraph->RebuildGraph();
		}
	}
}


void SSubstanceEditorPanel::OnToggleOutput(ECheckBoxState InNewState, output_inst_t* Output)
{
	UTexture* Texture = Output->Texture.get() ? *Output->Texture.get() : NULL;

	if (InNewState == ECheckBoxState::Checked)
	{
		FString TextureName;
		FString PackageName;
		Substance::Helpers::GetSuitableName(Output, TextureName, PackageName);
		UObject* TextureParent = CreatePackage(NULL, *PackageName);

		Substance::Helpers::CreateSubstanceTexture2D(Output, false, TextureName, TextureParent);

		Substance::List<graph_inst_t*> Instances;
		Instances.push(Output->GetParentGraphInstance());
		Substance::Helpers::RenderAsync(Instances);

		TArray<UObject*> NewTexture;
		NewTexture.Add(*(Output->Texture.get()));

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets( NewTexture );

		// should rebuild the graph of any material using a substance output
		ActualizeMaterialGraph(Output->GetParentGraphInstance());
	}
	else if (InNewState == ECheckBoxState::Unchecked && Texture && Texture->IsValidLowLevel())
	{
		FReferencerInformationList Refs;
		UObject* TextureObject = *(Output->Texture.get());

		// Check and see whether we are referenced by any objects that won't be garbage collected. 
		bool bIsReferenced = TextureObject ? IsReferenced( TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, true, &Refs) : false;

		if (bIsReferenced)
		{
			// determine whether the transaction buffer is the only thing holding a reference to the object
			// and if so, behave like it's not referenced and ask offer the user to clean up undo/redo history when he deletes the output
			GEditor->Trans->DisableObjectSerialization();
			bIsReferenced = IsReferenced(TextureObject, GARBAGE_COLLECTION_KEEPFLAGS, true, &Refs);
			GEditor->Trans->EnableObjectSerialization();			
		
			// only ref to this object is the transaction buffer - let the user choose whether to clear the undo buffer
			if ( !bIsReferenced )
			{
				if ( EAppReturnType::Yes == FMessageDialog::Open( 
					EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "ResetUndoBufferForObjectDeletionPrompt", 
						"The only reference to this object is the undo history.  In order to delete this object, you must clear all undo history - would you like to clear undo history?")) )
				{
					GEditor->Trans->Reset( NSLOCTEXT("UnrealEd", "DeleteSelectedItem", "Delete Selected Item") );
				}
				else
				{
					bIsReferenced = true;
				}
			}
		}

		if (Output->Texture && !bIsReferenced)
		{
			Substance::Helpers::RegisterForDeletion( *(Output->Texture.get()) );
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "ObjectStillReferenced", "The texture is in use, it cannot be deleted." ));
		}

		Substance::Helpers::Tick(); // need to make sure the texture has been deleted completely
		ActualizeMaterialGraph(Output->GetParentGraphInstance());
	}
}


ECheckBoxState SSubstanceEditorPanel::GetOutputState(output_inst_t* Output) const
{
	if (Output->bIsEnabled)
	{
		return ECheckBoxState::Checked;
	}

	return ECheckBoxState::Unchecked;
}


void SSubstanceEditorPanel::ConstructInputs()
{
	TSharedPtr<SScrollBox> InputsBox = TSharedPtr<SScrollBox>(NULL);
	TSharedPtr<SScrollBox> ImageInputsBox = TSharedPtr<SScrollBox>(NULL);

	InputsArea = TSharedPtr<SExpandableArea>(NULL);
	ImageInputsArea = TSharedPtr<SExpandableArea>(NULL);

	TMap< FString, TSharedPtr< SVerticalBox > > Groups;

	// make a copy of the list of inputs to sort them by index
	TArray<TSharedPtr<input_inst_t>> IdxSortedInputs = Graph->Instance->Inputs.getArray();

	struct FCompareInputIdx
	{
		FORCEINLINE bool operator()(const TSharedPtr<input_inst_t>& A, const TSharedPtr<input_inst_t>& B) const
		{
			return (*A).Desc->Index < (*B).Desc->Index;
		}
	};

	IdxSortedInputs.Sort(FCompareInputIdx());

	for (auto ItIn = IdxSortedInputs.CreateIterator(); ItIn; ++ItIn)
	{
		if (NULL == (*ItIn)->Desc)
		{
			check(0);
			//! @todo find out how an input can loose its desc
			continue;
		}

		if ((*ItIn)->Desc->Identifier == TEXT("$pixelsize") ||
			(*ItIn)->Desc->Identifier == TEXT("$time") ||
			(*ItIn)->Desc->Identifier == TEXT("$normalformat"))
		{
			continue;
		}

		if ((*ItIn)->Desc->IsNumerical())
		{
			if (false == InputsBox.IsValid())
			{
				InputsBox = SNew(SScrollBox);

				InputsArea = SNew(SExpandableArea)
					.AreaTitle(FString(TEXT("Inputs:")))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SBorder)
						.Content()
						[
							InputsBox.ToSharedRef()
						]
					];
			}

			FString GroupName = (*ItIn)->Desc->Group;

			if (GroupName.Len())
			{
				TSharedPtr<SVerticalBox> GroupBox;

				if (Groups.Contains(GroupName))
				{
					GroupBox = *Groups.Find(GroupName);
				}
				else
				{
					GroupBox = SNew(SVerticalBox);
					Groups.Add( GroupName, GroupBox );
				}

				GroupBox->AddSlot()
				[
					SNew( SSeparator )
						.Orientation( EOrientation::Orient_Horizontal )
				];

				GroupBox->AddSlot()
					.AutoHeight()
					.Padding(10.0f, 0.0f)
					[
						GetInputWidget(*ItIn)					
					];
			}
			else
			{
				InputsBox->AddSlot()
				[
					SNew( SSeparator )
						.Orientation( EOrientation::Orient_Horizontal )
				];

				InputsBox->AddSlot()
				[
					GetInputWidget(*ItIn)
				];
			}			
		}
		else
		{
			if (false == ImageInputsBox.IsValid())
			{
				ImageInputsBox = SNew(SScrollBox);

				ImageInputsArea = SNew(SExpandableArea)
					.AreaTitle(FString(TEXT("Image Inputs:")))
					.InitiallyCollapsed(false)
					.BodyContent()
					[
						SNew(SBorder)
							.Content()
							[
								ImageInputsBox.ToSharedRef()
							]
					];

				ThumbnailPool = MakeShareable( new FAssetThumbnailPool( 10 /*arbitrary number*/, true, 0.5f ) );
			}

			ImageInputsBox->AddSlot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				[
					GetImageInputWidget(*ItIn)
				]
			];
		}
	}

	TMap< FString, TSharedPtr< SVerticalBox >>::TIterator ItGroups(Groups);

	FLinearColor OddColor( .35,.35,.35, 0.0f );
	FLinearColor EvenColor( .4,.4,.4, 0.0f );

	int32 Idx = 0;

	for (;ItGroups;++ItGroups)
	{
		InputsBox->AddSlot()
		[
			SNew(SExpandableArea)
				.AreaTitle(ItGroups.Key())
				.InitiallyCollapsed(false)
				.BodyContent()
				[
					ItGroups.Value().ToSharedRef()
				]
		];

		InputsBox->AddSlot()
		[
			SNew( SSeparator )
			.Orientation( EOrientation::Orient_Horizontal )
		];
	}
}

template< typename T> void SSubstanceEditorPanel::SetValue(T NewValue, TSharedPtr<input_inst_t> Input, int32 Index)
{
	FInputValue<T> value = { NewValue, Input, Index };
	SetValues(1, &value);
}

template< typename T > void SSubstanceEditorPanel::SetValues(uint32 NumInputValues, FInputValue<T>* Inputs)
{
	bool renderAsync = false;

	for (uint32 i = 0; i < NumInputValues; i++)
	{
		FInputValue<T>& currentInput = Inputs[i];

		Substance::FNumericalInputInstanceBase* InputInst =
			(Substance::FNumericalInputInstanceBase*)(currentInput.Input.Get());

		TArray< T > Values;
		InputInst->GetValue(Values);

		if (Values.Num() == 0)
		{
			return;
		}

		if (Values[currentInput.Index] == currentInput.NewValue)
		{
			continue;
		}

		Values[currentInput.Index] = currentInput.NewValue;

		if (Graph->Instance->UpdateInput< T >(currentInput.Input->Uid, Values))
		{
			renderAsync = true;
		}
	}

	if (renderAsync)
	{
		Substance::List< graph_inst_t* > Graphs;
		Graphs.AddUnique(Graph->Instance);
		Substance::Helpers::RenderAsync(Graphs);
		Graph->MarkPackageDirty();
	}
}


template< typename T > TOptional< T > SSubstanceEditorPanel::GetInputValue(TSharedPtr<input_inst_t> Input, int32 Index) const
{
	Substance::FNumericalInputInstance<T>* TypedInst =
		(Substance::FNumericalInputInstance<T>*)Input.Get();
	T InputValue = 0;
	TArray< T > Values;

	TypedInst->GetValue(Values);
	
	InputValue = Values[Index];
	
	return InputValue;
}


FString SSubstanceEditorPanel::GetInputValue(TSharedPtr<input_inst_t> Input) const
{
	Substance::FNumericalInputInstance<int32>* TypedInst =
		(Substance::FNumericalInputInstance<int32>*)Input.Get();

	Substance::FNumericalInputDescComboBox* TypedDesc =
		(Substance::FNumericalInputDescComboBox*)TypedInst->Desc;

	TArray< int32 > Values;
	TypedInst->GetValue(Values);

	if (TypedDesc->ValueText.Num() >= 1)
	{
		return *TypedDesc->ValueText.Find(Values[0]);
	}
	else
	{
		return FString();
	}
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetCombobox(TSharedPtr< input_inst_t > Input)
{
	Substance::FNumericalInputDescComboBox* TypedDesc =
		(Substance::FNumericalInputDescComboBox*)Input.Get()->Desc;

	TArray< FString > Items;
	TypedDesc->ValueText.GenerateValueArray(Items);
	
	SharedFStringArray* ItemsForWidget = new SharedFStringArray();
	for (TMap< int32, FString > ::TIterator it(TypedDesc->ValueText); it ; ++it)
	{
		ItemsForWidget->Add(MakeShareable( new FString(it.Value()) ) );
	}

	ComboBoxLabels.Add( MakeShareable( ItemsForWidget ) );

	TSharedPtr< FString > CurrentValue = MakeShareable( new FString( GetInputValue(Input) ));

	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(Input->Desc->Label)
			.ToolTipText(Input->Desc->Identifier)
			.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
		+SHorizontalBox::Slot()
		[
			SNew(SComboBox< TSharedPtr<FString> >)
				.OptionsSource(ComboBoxLabels.Last().Get())
				.OnGenerateWidget(this, &SSubstanceEditorPanel::MakeInputComboWidget)
				.OnSelectionChanged(this, &SSubstanceEditorPanel::OnComboboxSelectionChanged, Input)
				.InitiallySelectedItem(CurrentValue)
				.ContentPadding(0)
				.Content()
				[
					SNew(STextBlock)
					.Text(this, &SSubstanceEditorPanel::GetInputValue, Input)
				]
		];
}


TSharedRef<SWidget> SSubstanceEditorPanel::MakeInputComboWidget( TSharedPtr<FString> InItem )
{
	return SNew(STextBlock).Text(*InItem);
}


void SSubstanceEditorPanel::OnComboboxSelectionChanged( TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo, TSharedPtr< input_inst_t > Input/*, TSharedRef<STextBlock> ComboText*/)
{
	Substance::FNumericalInputDescComboBox* TypedDesc =
		(Substance::FNumericalInputDescComboBox*)Input.Get()->Desc;

	if (!Item.IsValid() || TypedDesc->ValueText.Num() == 0)
	{
		return;
	}

	float ItemValue((float)*TypedDesc->ValueText.FindKey(*Item) + 0.1);

	FInputValue<float> value = { ItemValue, Input, 0 };
	SetValues<float>(1, &value);
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetTogglebutton(TSharedPtr< input_inst_t > Input)
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(Input->Desc->Label)
			.ToolTipText(Input->Desc->Identifier)
			.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
		+SHorizontalBox::Slot()
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SSubstanceEditorPanel::OnToggleValueChanged, Input)
			.IsChecked(this, &SSubstanceEditorPanel::GetToggleValue, Input)
		];
}


void SSubstanceEditorPanel::OnToggleValueChanged(ECheckBoxState InNewState, TSharedPtr< input_inst_t > Input)
{
	FInputValue<int32> value = { 1, Input, 0 };

	if (InNewState == ECheckBoxState::Checked)
	{
		value.NewValue = 1;
		SetValues<int32>(1, &value);
	}
	else
	{
		value.NewValue = 0;
		SetValues<int32>(1, &value);
	}
}


ECheckBoxState SSubstanceEditorPanel::GetToggleValue(TSharedPtr< input_inst_t > Input) const
{
	if (GetInputValue<int32>(Input, 0).GetValue() > 0.0f)
	{
		return ECheckBoxState::Checked;
	}

	return ECheckBoxState::Unchecked;
}


void SSubstanceEditorPanel::UpdateColor(FLinearColor NewColor, TSharedPtr< input_inst_t > Input)
{
	if (Substance_IType_Float == Input->Desc->Type)
	{
		FInputValue<float> value = { NewColor.Desaturate(1.0f).R, Input, 0 };
		SetValues<float>(1, &value);
	}
	else
	{
		FInputValue<float> values[] = 
		{
			{ NewColor.R, Input, 0 },
			{ NewColor.G, Input, 1 },
			{ NewColor.B, Input, 2 },
			{ NewColor.A, Input, 3 },
		};

		if (Substance_IType_Float4 == Input->Desc->Type)
		{
			SetValues<float>(4, values);
		}
		else
		{
			SetValues<float>(3, values);
		}
	}
}


void SSubstanceEditorPanel::CancelColor(FLinearColor OldColor, TSharedPtr< input_inst_t > Input)
{
	UpdateColor(OldColor.HSVToLinearRGB(), Input);
}


FReply SSubstanceEditorPanel::PickColor(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSharedPtr< input_inst_t > Input)
{
	FColorPickerArgs PickerArgs;

	FLinearColor InputColor = GetColor(Input);

	if (Input->Desc->Type == Substance_IType_Float4)
	{
		PickerArgs.bUseAlpha = true;
	}
	else
	{
		PickerArgs.bUseAlpha = false;
	}

	PickerArgs.ParentWidget = this->AsShared();
	PickerArgs.bOnlyRefreshOnOk = false;
	PickerArgs.bOnlyRefreshOnMouseUp = false;
	PickerArgs.InitialColorOverride = InputColor;
	PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP( this, &SSubstanceEditorPanel::UpdateColor, Input);
	PickerArgs.OnColorPickerCancelled = FOnColorPickerCancelled::CreateSP( this, &SSubstanceEditorPanel::CancelColor, Input);

	OpenColorPicker(PickerArgs);

	return FReply::Handled();
}


FString SSubstanceEditorPanel::GetOutputSizeValue(TSharedPtr<input_inst_t> Input, int32 Idx) const
{
	Substance::FNumericalInputInstance<vec2int_t>* TypedInst =
		(Substance::FNumericalInputInstance<vec2int_t>*)Input.Get();
	
	TArray< int32 > Values;
	TypedInst->GetValue(Values);

	int32 SizeValue = FMath::RoundToInt(FMath::Pow(2.0f, (float)Values[Idx]));

	return FString::Printf(TEXT("%i"), SizeValue);
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetSizePow2(TSharedPtr< input_inst_t > Input)
{
	SharedFStringArray* ItemsForWidget = new SharedFStringArray();

	int32 CurrentPow2 = POW2_MIN;
	while (CurrentPow2 <= POW2_MAX)
	{
		int32 SizeValue = FMath::Pow(2.0f, (float)CurrentPow2);
		CurrentPow2++;
		ItemsForWidget->Add(MakeShareable( new FString( FString::Printf(TEXT("%i"), SizeValue) )));
	}

	ComboBoxLabels.Add( MakeShareable( ItemsForWidget ) );

	TSharedPtr< FString > CurrentX = MakeShareable( new FString(GetOutputSizeValue(Input, 0)));
	TSharedPtr< FString > CurrentY = MakeShareable( new FString(GetOutputSizeValue(Input, 1)));

	TSharedPtr< SComboBox< TSharedPtr< FString > > > WidgetSizeX = SNew(SComboBox< TSharedPtr<FString> >)
		.OptionsSource(ComboBoxLabels.Last().Get())
		.OnGenerateWidget(this, &SSubstanceEditorPanel::MakeInputSizeComboWidget)
		.OnSelectionChanged(this, &SSubstanceEditorPanel::OnSizeComboboxSelectionChanged, Input, 0)
		.ContentPadding(0)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &SSubstanceEditorPanel::GetOutputSizeValue, Input, 0)
		];

	TSharedPtr< SComboBox< TSharedPtr< FString > > > WidgetSizeY = SNew(SComboBox< TSharedPtr<FString> >)
		.OptionsSource(ComboBoxLabels.Last().Get())
		.OnGenerateWidget(this, &SSubstanceEditorPanel::MakeInputSizeComboWidget)
		.OnSelectionChanged(this, &SSubstanceEditorPanel::OnSizeComboboxSelectionChanged, Input, 1)
		.ContentPadding(0)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &SSubstanceEditorPanel::GetOutputSizeValue, Input, 1)
		];

	WidgetSizeX->SetSelectedItem(CurrentX);
	WidgetSizeY->SetSelectedItem(CurrentY);

	RatioLocked.Set(true);
	
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
			.Text(FString(TEXT("Output Size")))
			.ToolTipText(Input->Desc->Identifier)
			.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
		+SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			[
				WidgetSizeX.ToSharedRef()
			]
			+SHorizontalBox::Slot()
			[
				WidgetSizeY.ToSharedRef()
			]
			+SHorizontalBox::Slot()
			[
				SNew(SCheckBox)
					.OnCheckStateChanged( this, &SSubstanceEditorPanel::OnLockRatioValueChanged )
					.IsChecked( this, &SSubstanceEditorPanel::GetLockRatioValue )
					.Content()
					[
						SNew(STextBlock)
							.Text(FString(TEXT("Lock Ratio")))
					]
			]
		];
}


void SSubstanceEditorPanel::OnLockRatioValueChanged(ECheckBoxState InNewState)
{
	RatioLocked.Set(InNewState == ECheckBoxState::Checked ? true : false);
}


ECheckBoxState SSubstanceEditorPanel::GetLockRatioValue() const
{
	return RatioLocked.Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}


TSharedRef<SWidget> SSubstanceEditorPanel::MakeInputSizeComboWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(*InItem);
}


void SSubstanceEditorPanel::OnSizeComboboxSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo, TSharedPtr< input_inst_t > Input, int Idx)
{
	if (!Item.IsValid())
	{
		return;
	}

	Substance::FNumericalInputInstance<vec2int_t>* TypedInst =
		(Substance::FNumericalInputInstance<vec2int_t>*)Input.Get();
	TArray< int32 > Values;
	TypedInst->GetValue(Values);
	float PrevValue = (float)Values[Idx];

	int SizeValue = FCString::Atoi(**Item);
	float NewValue = FMath::Log2((float)SizeValue);

	FInputValue<int32> values[2];

	values[0].NewValue = NewValue;
	values[0].Input = Input;
	values[0].Index = Idx;

	if (RatioLocked.Get())
	{
 		const float DeltaValue = NewValue - PrevValue;
		const int OtherIdx = Idx == 0 ? 1 : 0;

		values[1].NewValue = FMath::Clamp((float)Values[OtherIdx] + DeltaValue, (float)POW2_MIN, (float)POW2_MAX);;
		values[1].Input = Input;
		values[1].Index = OtherIdx;

		SetValues<int32>(2, values);
	}
	else
	{
		SetValues<int32>(1, values);
	}
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetRandomSeed(TSharedPtr< input_inst_t > Input)
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding(1.0f, 1.0f)
		.FillWidth(0.3f)
		[
			SNew(STextBlock)
				.Text(FString(TEXT("Random Seed")))
				.ToolTipText(Input->Desc->Identifier)
				.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
		+SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
						.Text(FString(TEXT("Randomize Seed")))
						.OnClicked( this, &SSubstanceEditorPanel::RandomizeSeed, Input )
				]
				+SHorizontalBox::Slot()
				[
					GetInputWidgetSlider_internal<int32>(Input, 0)
				]
		];
}


FReply SSubstanceEditorPanel::RandomizeSeed(TSharedPtr< input_inst_t > Input)
{
	const int32 rand = FMath::Rand();

	FInputValue<int32> value = { rand, Input, 0 };
	SetValues<int32>(1, &value);

	return FReply::Handled();
}


void SSubstanceEditorPanel::OnGetClassesForAssetPicker( TArray<const UClass*>& OutClasses )
{
	OutClasses.AddUnique(USubstanceImageInput::StaticClass());
	
	// disable substance output as input feature for now
	//OutClasses.AddUnique(USubstanceTexture2D::StaticClass());
}


void SSubstanceEditorPanel::OnAssetSelected( const FAssetData& AssetData, TSharedPtr<input_inst_t> Input )
{
	OnSetImageInput(AssetData.GetAsset(), Input);
}


void SSubstanceEditorPanel::OnUseSelectedImageInput(TSharedPtr<input_inst_t> Input)
{
	// Load selected assets
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();

	USelection* Selection = GEditor->GetSelectedObjects();
	if (Selection)
	{
		USubstanceImageInput* ImageInput = Selection->GetTop<USubstanceImageInput>();
		if (ImageInput)
		{
			OnSetImageInput(ImageInput, Input);
		}
	}
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetImageInputWidget(TSharedPtr< input_inst_t > Input)
{
	Substance::FImageInputDesc* ImgInputDesc = (Substance::FImageInputDesc*)Input->Desc;

	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(0.3f)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(Input->Desc->Label)
					.Font(FEditorStyle::GetFontStyle("BoldFont"))
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(ImgInputDesc->Desc)
				]
			]
		]
		+SHorizontalBox::Slot()
		[
			SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 1.0f)
					[
						PropertyCustomizationHelpers::MakeAssetPickerAnchorButton( 
							FOnGetAllowedClasses::CreateSP(this, &SSubstanceEditorPanel::OnGetClassesForAssetPicker),
							FOnAssetSelected::CreateSP(this, &SSubstanceEditorPanel::OnAssetSelected, Input))
					]
				+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 1.0f)
					[
						PropertyCustomizationHelpers::MakeUseSelectedButton( FSimpleDelegate::CreateSP( this, &SSubstanceEditorPanel::OnUseSelectedImageInput, Input ) )
					]
				+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(2.0f, 1.0f)
					[
						SNew(SButton)
							.ToolTipText(NSLOCTEXT("PropertyEditor", "ResetToDefaultToolTip", "Reset to Default").ToString())
							.ButtonStyle(FEditorStyle::Get(), "NoBorder")
							.OnClicked(this, &SSubstanceEditorPanel::OnResetImageInput, Input)
							.Content()
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
							]
					]
				+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SSpacer)
					]
		];
}


FString SSubstanceEditorPanel::GetImageInputPath(TSharedPtr< input_inst_t > Input)
{
	Substance::FImageInputInstance* TypedInst = 
		(Substance::FImageInputInstance*)(Input.Get());

	FString PathName;

	if (TypedInst->ImageSource)
	{
		PathName = TypedInst->ImageSource->GetPathName();
	}

	return PathName;
}


FReply SSubstanceEditorPanel::OnResetImageInput(TSharedPtr< input_inst_t > Input)
{
	OnSetImageInput(NULL, Input);
	return FReply::Handled();
}


void SSubstanceEditorPanel::OnSetImageInput(const UObject* InObject, TSharedPtr< input_inst_t > Input)
{
	Substance::FImageInputInstance* InputInst =
		(Substance::FImageInputInstance*)(Input.Get());

	UObject* NewInput = Cast< UObject >(const_cast< UObject* >(InObject));

	if (Graph->Instance->UpdateInput(Input->Uid, NewInput))
	{
		Substance::List< graph_inst_t* > Graphs;
		Graphs.AddUnique(Graph->Instance);
		Substance::Helpers::RenderAsync(Graphs);
	}

	ThumbnailPool->Tick(0.1f);
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidgetAngle(TSharedPtr<input_inst_t> Input)
{
	return GetInputWidgetSlider<float>(Input);
}


TSharedRef<SWidget> SSubstanceEditorPanel::GetInputWidget(TSharedPtr< input_inst_t > Input)
{
	// special widget for random seed
	if (Input->Desc->Identifier == TEXT("$randomseed"))
	{
		return GetInputWidgetRandomSeed(Input);
	}
	if (Input->Desc->Identifier == TEXT("$outputsize"))
	{
		return GetInputWidgetSizePow2(Input);
	}

	switch (Input->Desc->Widget)
	{
	default:
	case Substance::Input_NoWidget:
	case Substance::Input_Slider:
	case Substance::Input_Color:

		switch(Input->Desc->Type)
		{
		case Substance_IType_Float:
		case Substance_IType_Float2:
		case Substance_IType_Float3:
		case Substance_IType_Float4:
			return GetInputWidgetSlider< float >(Input);

		default:
		case Substance_IType_Integer:
		case Substance_IType_Integer2:
		case Substance_IType_Integer3:
		case Substance_IType_Integer4:
			return GetInputWidgetSlider< int32 >(Input);
		}
		
	case Substance::Input_Angle:
		return GetInputWidgetAngle(Input);

	case Substance::Input_Combobox:
		return GetInputWidgetCombobox(Input);

	case Substance::Input_Togglebutton:
		return GetInputWidgetTogglebutton(Input);

	case Substance::Input_SizePow2:
		return GetInputWidgetSizePow2(Input);
	}
}


template< typename T > TSharedRef<SHorizontalBox> SSubstanceEditorPanel::GetInputWidgetSlider(TSharedPtr<input_inst_t> Input)
{
	check(Input.IsValid());
	check(Input->Desc->Type != Substance_IType_Image);

	int32 SliderCount = 1;

	switch(Input->Desc->Type)
	{
	case Substance_IType_Integer2:
	case Substance_IType_Float2:
		SliderCount = 2;
		break;
	case Substance_IType_Integer3:
	case Substance_IType_Float3:
		SliderCount = 3;
		break;
	case Substance_IType_Integer4:
	case Substance_IType_Float4:
		SliderCount = 4;
		break;
	}

	TSharedPtr<SHorizontalBox> InputContainer = SNew(SHorizontalBox);

	TSharedPtr<SVerticalBox> SlidersBox = SNew(SVerticalBox);
	for (int32 i = 0 ; i < SliderCount ; ++i)
	{
		SlidersBox->AddSlot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(0.0f, 1.0f)
			[
				GetInputWidgetSlider_internal<T>(Input, i)
			]
		];
	}

	InputContainer->AddSlot()
	[
		SlidersBox.ToSharedRef()
	];

	if (Input->Desc->Widget == Substance::Input_Color)
	{
		InputContainer->AddSlot()
		.FillWidth(0.1f)
		[
			SNew(SColorBlock)
				.Color(this, &SSubstanceEditorPanel::GetColor, Input)
				.ColorIsHSV(false)
				.ShowBackgroundForAlpha(true)
				.ToolTipText(FString(TEXT("Pick Color")))
				.OnMouseButtonDown( this, &SSubstanceEditorPanel::PickColor, Input)
				.UseSRGB(false)
		];
	}

	TSharedPtr<SHorizontalBox> WidgetsBox = 
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(0.3f)
		.VAlign(VAlign_Top)
		[
			SNew(STextBlock)
				.Text(Input->Desc->Label)
				.ToolTipText(Input->Desc->Identifier)
				.Font(FEditorStyle::GetFontStyle("BoldFont"))
		]
		+SHorizontalBox::Slot()
		[
			InputContainer.ToSharedRef()
		];

	return WidgetsBox.ToSharedRef();
}


FLinearColor SSubstanceEditorPanel::GetColor(TSharedPtr<input_inst_t> Input) const
{
	TArray< float > Values;	
	FLinearColor InputColor;

	Substance::FNumericalInputInstanceBase* InputInst =
		(Substance::FNumericalInputInstanceBase*)(Input.Get());

	InputInst->GetValue(Values);

	if (Values.Num() == 1)
	{
		InputColor = FLinearColor(Values[0],Values[0],Values[0],1.0f);
	}
	else if (Values.Num() == 3)
	{
		InputColor = FLinearColor(Values[0],Values[1],Values[2],1.0f);
	}
	else if (Values.Num() == 4)
	{
		InputColor = FLinearColor(Values[0],Values[1],Values[2],Values[3]);
	}

	return InputColor;
}


void SSubstanceEditorPanel::BeginSliderMovement(TSharedPtr<input_inst_t> Input)
{
	FScopedTransaction Transaction( NSLOCTEXT("SubstanceEditor", "SubstanceSetInput", "Substance set input"));
	Graph->Modify();
}


template< typename T, typename U> const U GetMinComponent(input_desc_t* Desc, int Idx, bool& Clamped)
{
	Substance::FNumericalInputDesc<T>* TypedDesc = 
		(Substance::FNumericalInputDesc<T>*)Desc;

	const U val = TypedDesc->Min[Idx];

	return val;
}


template< typename T, typename U> const U GetMaxComponent(input_desc_t* Desc, int Idx, bool& Clamped)
{
	Substance::FNumericalInputDesc<T>* TypedDesc = 
		(Substance::FNumericalInputDesc<T>*)Desc;

	const U val = TypedDesc->Max[Idx];

	return val;
}


template< typename T> void GetMinMaxValues(input_desc_t* Desc, const int32 i, T& Min, T& Max, bool& Clamped)
{
	switch(Desc->Type)
	{
	case Substance_IType_Integer:
		{
			Substance::FNumericalInputDesc<int32>* TypedDesc =
				(Substance::FNumericalInputDesc<int32>*)Desc;

			Min = TypedDesc->Min;
			Max = TypedDesc->Max;
			Clamped = TypedDesc->IsClamped;
		}
		break;
	case Substance_IType_Float:
		{
			Substance::FNumericalInputDesc<float>* TypedDesc =
				(Substance::FNumericalInputDesc<float>*)Desc;

			Min = TypedDesc->Min;
			Max = TypedDesc->Max;
			Clamped = TypedDesc->IsClamped;
		}
		break;
	case Substance_IType_Integer2:
		{
			Min = GetMinComponent<vec2int_t, int32>(Desc,i, Clamped);
			Max = GetMaxComponent<vec2int_t, int32>(Desc,i, Clamped);
		}
		break;
	case Substance_IType_Float2:
		{
			Min = GetMinComponent<vec2float_t, float>(Desc,i, Clamped);
			Max = GetMaxComponent<vec2float_t, float>(Desc,i, Clamped);
		}
		break;
	case Substance_IType_Integer3:
		{
			Min = GetMinComponent<vec3int_t, int>(Desc,i, Clamped);
			Max = GetMaxComponent<vec3int_t, int>(Desc,i, Clamped);
		}
		break;
	case Substance_IType_Float3:
		{
			Min = GetMinComponent<vec3float_t, float>(Desc,i, Clamped);
			Max = GetMaxComponent<vec3float_t, float>(Desc,i, Clamped);
		}
		break;
	case Substance_IType_Integer4:
		{
			Min = GetMinComponent<vec4int_t, int>(Desc,i, Clamped);
			Max = GetMaxComponent<vec4int_t, int>(Desc,i, Clamped);
		}
		break;
	case Substance_IType_Float4:
		{
			Min = GetMinComponent<vec4float_t, float>(Desc,i, Clamped);
			Max = GetMaxComponent<vec4float_t, float>(Desc,i, Clamped);
		}
		break;
	}
}


template< > TSharedRef< SNumericEntryBox< float > > SSubstanceEditorPanel::GetInputWidgetSlider_internal<float>(TSharedPtr<input_inst_t> Input, const int32 SliderIndex)
{
	float delta = 0;
	float SliderMin = 0;
	float SliderMax = 0;
	bool Clamped = false;

	GetMinMaxValues(Input.Get()->Desc, SliderIndex, SliderMin, SliderMax, Clamped);

	float MinValue = SliderMin;
	float MaxValue = SliderMax;

	if (Clamped == false)
	{
		if (Input.Get()->Desc->Widget == Substance::Input_Color)
		{
			MinValue = 0.0f;
			MaxValue = 1.0f;
		}
		else
		{
			MinValue = TNumericLimits<float>::Lowest();
			MaxValue = TNumericLimits<float>::Max();
		}
	}

	if (SliderMin == SliderMax || SliderMin > SliderMax)
	{
		if (Input.Get()->Desc->Widget == Substance::Input_Color)
		{
			SliderMin = 0.0f;
			SliderMax = 1.0f;
		}
		else
		{
			SliderMin = TNumericLimits<float>::Lowest();
			SliderMax = TNumericLimits<float>::Max();
		}
	}

	return SNew( SNumericEntryBox<float> )
		.Value(this, &SSubstanceEditorPanel::GetInputValue<float>, Input, SliderIndex)
		.OnValueChanged(this, &SSubstanceEditorPanel::SetValue<float>, Input, SliderIndex)
		.OnBeginSliderMovement(this, &SSubstanceEditorPanel::BeginSliderMovement, Input)
		.ToolTipText(Input->Desc->Identifier)
		.AllowSpin(true)
		.Delta(0.001f)
		.LabelPadding(FMargin(0.0f, 1.1f))
		.MinValue(MinValue)
		.MaxValue(MaxValue)
		.MinSliderValue(SliderMin)
		.MaxSliderValue(SliderMax);
}


template< > TSharedRef< SNumericEntryBox< int32 > > SSubstanceEditorPanel::GetInputWidgetSlider_internal<int32>(TSharedPtr<input_inst_t> Input, const int32 SliderIndex)
{
	float delta = 0;
	int32 SliderMin = 0;
	int32 SliderMax = 0;
	bool Clamped = false;

	GetMinMaxValues(Input.Get()->Desc, SliderIndex, SliderMin, SliderMax, Clamped);

	int32 MinValue = SliderMin;
	int32 MaxValue = SliderMax;

	if (Clamped == false)
	{
		MinValue = TNumericLimits<int32>::Lowest();
		MaxValue = TNumericLimits<int32>::Max();
	}

	if (SliderMin == SliderMax || SliderMin > SliderMax)
	{
		SliderMin = TNumericLimits<int32>::Lowest();
		SliderMax = TNumericLimits<int32>::Max();
	}

	return SNew( SNumericEntryBox<int32> )
		.Value(this, &SSubstanceEditorPanel::GetInputValue<int32>, Input, SliderIndex)
		.OnValueChanged(this, &SSubstanceEditorPanel::SetValue<int32>, Input, SliderIndex)
		.OnBeginSliderMovement(this, &SSubstanceEditorPanel::BeginSliderMovement, Input)
		.ToolTipText(Input->Desc->Identifier)
		.AllowSpin(true)
		.Delta(0)
		.LabelPadding(FMargin(0.0f, 1.1f))
		.MinValue(MinValue)
		.MaxValue(MaxValue)
		.MinSliderValue(SliderMin)
		.MaxSliderValue(SliderMax);
}

#undef LOC_NAMESPACE

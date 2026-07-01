// Copyright (c) 2026 Wz4h. All Rights Reserved.
#include "UI/SSmartAssetCreateWindow.h"

#include "AssetRegistry/AssetData.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "DataTableEditorUtils.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Animation/Skeleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInterface.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Rule/SmartAssetRuleResolver.h"
#include "StructViewerFilter.h"
#include "StructViewerModule.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

namespace
{
	class FSmartAssetClassFilter : public IClassViewerFilter
	{
	public:
		explicit FSmartAssetClassFilter(UClass* InBaseClass, bool bInAllowDerivedClasses)
			: BaseClass(InBaseClass)
			, bAllowDerivedClasses(bInAllowDerivedClasses)
		{
		}

		virtual bool IsClassAllowed(
			const FClassViewerInitializationOptions& InInitOptions,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			if (!BaseClass || !InClass)
			{
				return false;
			}

			const bool bMatchesBaseClass = bAllowDerivedClasses ? InClass->IsChildOf(BaseClass) : InClass == BaseClass;
			const bool bIsObjectRootClass = BaseClass == UObject::StaticClass() && InClass == UObject::StaticClass();
			const bool bHasDisallowedFlags = InClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists)
				|| (InClass->HasAnyClassFlags(CLASS_Abstract) && !bIsObjectRootClass);
			return bMatchesBaseClass && !bHasDisallowedFlags;
		}

		virtual bool IsUnloadedClassAllowed(
			const FClassViewerInitializationOptions& InInitOptions,
			const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			if (!BaseClass)
			{
				return false;
			}

			if (!bAllowDerivedClasses)
			{
				return false;
			}

			TSet<const UClass*> AllowedClasses;
			AllowedClasses.Add(BaseClass);
			return InFilterFuncs->IfInChildOfClassesSet(AllowedClasses, InUnloadedClassData) != EFilterReturn::Failed;
		}

	private:
		UClass* BaseClass = nullptr;
		bool bAllowDerivedClasses = true;
	};

	class FSmartDataTableStructFilter : public IStructViewerFilter
	{
	public:
		virtual bool IsStructAllowed(
			const FStructViewerInitializationOptions& InInitOptions,
			const UScriptStruct* InStruct,
			TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
		{
			return InStruct && FDataTableEditorUtils::IsValidTableStruct(InStruct);
		}

		virtual bool IsUnloadedStructAllowed(
			const FStructViewerInitializationOptions& InInitOptions,
			const FSoftObjectPath& InStructPath,
			TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
		{
			// Blueprint structs must be loaded before DataTable validity can be checked.
			return true;
		}
	};

	int32 ComputeClassDistance(const UClass* ChildClass, const UClass* AncestorClass)
	{
		if (!ChildClass || !AncestorClass)
		{
			return INDEX_NONE;
		}

		int32 Distance = 0;
		for (const UClass* Current = ChildClass; Current; Current = Current->GetSuperClass(), ++Distance)
		{
			if (Current == AncestorClass)
			{
				return Distance;
			}
		}

		return INDEX_NONE;
	}
}

void SSmartAssetCreateWindow::Construct(const FArguments& InArgs)
{
	ParentWindowWeak = InArgs._ParentWindow;
	TargetFolder = InArgs._TargetFolder;
	InitialUnderlyingKind = InArgs._InitialUnderlyingKind;
	bLockCreationOption = InArgs._LockCreationOption;
	InitialParentBlueprint = InArgs._InitialParentBlueprint;
	OnCreateRequested = InArgs._OnCreateRequested;
	OnPreviewRequested = InArgs._OnPreviewRequested;
	OnValidationRequested = InArgs._OnValidationRequested;
	OnOpenSettingsRequested = InArgs._OnOpenSettingsRequested;

	RebuildCreationOptions();

	RefreshDefaultsForCreationOption();
	InvalidatePreview();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SmartAssetCreator", "CreateWindowTitle", "Create Smart Asset"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ContentPadding(2.0f)
					.ToolTipText(NSLOCTEXT("SmartAssetCreator", "SettingsButtonTooltip", "Open Smart Asset Creator settings."))
					.OnClicked(this, &SSmartAssetCreateWindow::HandleOpenSettingsClicked)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Settings"))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("Target Folder: %s"), *TargetFolder));
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f)
			[
				BuildCreationOptionPicker()
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 6.0f)
			[
				BuildDynamicOptions()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SSeparator)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return FText::FromString(FString::Printf(TEXT("Name Preview: %s"), *GetPreviewText()));
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				BuildFooter()
			]
		]
	];

	RefreshClassViewer();
}

void SSmartAssetCreateWindow::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(InitialParentBlueprint);
	Collector.AddReferencedObject(SelectedParentClass);
	Collector.AddReferencedObject(SelectedRowStruct);
	Collector.AddReferencedObject(SelectedSkeleton);
	Collector.AddReferencedObject(SelectedParentMaterial);

	for (const TSharedPtr<FSmartCreationOption>& Option : CreationOptions)
	{
		if (Option.IsValid())
		{
			Collector.AddReferencedObject(Option->ParentClass);
		}
	}
}

void SSmartAssetCreateWindow::UnbindModuleCallbacks()
{
	OnCreateRequested.Unbind();
	OnPreviewRequested.Unbind();
	OnValidationRequested.Unbind();
	OnOpenSettingsRequested.Unbind();
}

FString SSmartAssetCreateWindow::GetReferencerName() const
{
	return TEXT("SSmartAssetCreateWindow");
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildCreationOptionPicker()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SmartAssetCreator", "CreationOptionLabel", "Creation Template"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SAssignNew(CreationOptionComboBox, SComboBox<TSharedPtr<FSmartCreationOption>>)
			.OptionsSource(&CreationOptions)
			.IsEnabled(!bLockCreationOption)
			.OnGenerateWidget_Lambda([](TSharedPtr<FSmartCreationOption> Item)
			{
				return SNew(STextBlock).Text(Item.IsValid() ? Item->DisplayName : FText::GetEmpty());
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<FSmartCreationOption> Item, ESelectInfo::Type)
			{
				if (Item.IsValid())
				{
					SelectedCreationOption = Item;
					SelectedParentClass = nullptr;
					SelectedRowStruct = nullptr;
					SelectedSkeleton = nullptr;
					SelectedParentMaterial = nullptr;
					bTemplateAnimBlueprint = false;
					RefreshDefaultsForCreationOption();
					InvalidatePreview();
				}
			})
			.InitiallySelectedItem(SelectedCreationOption)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return SelectedCreationOptionDisplayName;
				})
			]
		];
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildDynamicOptions()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return InitialParentBlueprint
						? FText::FromString(FString::Printf(TEXT("Source Blueprint: %s"), *InitialParentBlueprint->GetName()))
						: FText::GetEmpty();
				})
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]()
				{
					return NeedsParentClass() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					BuildClassPicker(
						NSLOCTEXT("SmartAssetCreator", "ParentClassLabel", "Parent Class")
					)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]()
				{
					return IsAnimBlueprintOption() && !InitialParentBlueprint
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildObjectPicker(
							NSLOCTEXT("SmartAssetCreator", "SkeletonLabel", "Target Skeleton"),
							USkeleton::StaticClass(),
							[this]() { return SelectedSkeleton ? SelectedSkeleton->GetPathName() : FString(); },
								[this](const FAssetData& AssetData)
								{
									SelectedSkeleton = Cast<USkeleton>(AssetData.GetAsset());
									InvalidatePreview();
								})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bTemplateAnimBlueprint ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
							{
								bTemplateAnimBlueprint = NewState == ECheckBoxState::Checked;
								InvalidatePreview();
							})
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SmartAssetCreator", "TemplateAnimBlueprint", "Create as template Anim"))
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]()
				{
					return IsDataTableOption() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					BuildDataTableStructPicker()
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]()
				{
					return IsMaterialInstanceOption() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					BuildObjectPicker(
						NSLOCTEXT("SmartAssetCreator", "ParentMaterialLabel", "Parent Material"),
						UMaterialInterface::StaticClass(),
						[this]() { return SelectedParentMaterial ? SelectedParentMaterial->GetPathName() : FString(); },
							[this](const FAssetData& AssetData)
							{
								SelectedParentMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
								InvalidatePreview();
							})
				]
			]
		];
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildFooter()
{
	return SNew(SUniformGridPanel)
		.SlotPadding(FMargin(6.0f, 0.0f))
		+ SUniformGridPanel::Slot(0, 0)
		[
				SNew(SButton)
				.Text(NSLOCTEXT("SmartAssetCreator", "CreateButton", "Create"))
				.IsEnabled_Lambda([this]() { return CanCreate(); })
				.ToolTipText_Lambda([this]()
				{
					const FString ValidationError = GetValidationError();
					return ValidationError.IsEmpty()
						? NSLOCTEXT("SmartAssetCreator", "CreateButtonTooltip", "Create the selected asset.")
						: FText::FromString(ValidationError);
				})
				.OnClicked(this, &SSmartAssetCreateWindow::HandleCreateClicked)
		]
		+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("SmartAssetCreator", "CancelButton", "Cancel"))
			.OnClicked(this, &SSmartAssetCreateWindow::HandleCancelClicked)
		];
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildClassPicker(const FText& Label)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock).Text(Label)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SAssignNew(ClassViewerContainer, SBox)
			.HeightOverride(260.0f)
		];
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildObjectPicker(const FText& Label, UClass* AllowedClass, TFunction<FString()> GetterText, TFunction<void(const FAssetData&)> Setter)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock).Text(Label)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(AllowedClass)
			.ObjectPath_Lambda([GetterText]() { return GetterText(); })
			.OnObjectChanged_Lambda([Setter](const FAssetData& AssetData) { Setter(AssetData); })
		];
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildDataTableStructPicker()
{
	FStructViewerInitializationOptions Options;
	Options.Mode = EStructViewerMode::StructPicker;
	Options.StructFilter = MakeShared<FSmartDataTableStructFilter>();

	FStructViewerModule& StructViewerModule = FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer");
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SmartAssetCreator", "RowStructLabel", "Row Struct"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.HeightOverride(260.0f)
				[
					StructViewerModule.CreateStructViewer(
						Options,
						FOnStructPicked::CreateLambda([this](const UScriptStruct* PickedStruct)
						{
								SelectedRowStruct = const_cast<UScriptStruct*>(PickedStruct);
								InvalidatePreview();
						}))
				]
			];
}

void SSmartAssetCreateWindow::RebuildCreationOptions()
{
	CreationOptions.Reset();
	SelectedCreationOption.Reset();

	for (const FSmartCreationOption& Option : CreationOptionBuilder.BuildOptions())
	{
			if (InitialParentBlueprint
				&& (!InitialParentBlueprint->GeneratedClass
					|| Option.UnderlyingKind != ESmartUnderlyingAssetKind::Blueprint
					|| !FSmartCreationOptionBuilder::DoesOptionMatchClass(Option, InitialParentBlueprint->GeneratedClass)))
		{
			continue;
		}

		TSharedPtr<FSmartCreationOption> SharedOption = MakeShared<FSmartCreationOption>(Option);
		CreationOptions.Add(SharedOption);
		if (!InitialParentBlueprint && !SelectedCreationOption.IsValid() && Option.UnderlyingKind == InitialUnderlyingKind)
		{
			SelectedCreationOption = SharedOption;
		}
	}

	if (InitialParentBlueprint && InitialParentBlueprint->GeneratedClass)
	{
		int32 BestDistance = TNumericLimits<int32>::Max();
		for (const TSharedPtr<FSmartCreationOption>& Option : CreationOptions)
		{
			if (!Option.IsValid() || Option->UnderlyingKind != ESmartUnderlyingAssetKind::Blueprint || !Option->ParentClass)
			{
				continue;
			}

			const int32 Distance = ComputeClassDistance(InitialParentBlueprint->GeneratedClass, Option->ParentClass);
			if (Distance != INDEX_NONE && Distance < BestDistance)
			{
				BestDistance = Distance;
				SelectedCreationOption = Option;
			}
		}
	}

	if (!SelectedCreationOption.IsValid() && CreationOptions.Num() > 0)
	{
		SelectedCreationOption = CreationOptions[0];
	}
}

void SSmartAssetCreateWindow::RefreshForSettingsChange()
{
	const TSharedPtr<FSmartCreationOption> PreviousOption = SelectedCreationOption;
	UClass* PreviousParentClass = SelectedParentClass;

	CreationOptionBuilder.ResetCache();
	FSmartAssetRuleResolver::LoadConfiguredRuleClasses();
	RebuildCreationOptions();

	if (PreviousOption.IsValid())
	{
		const TSharedPtr<FSmartCreationOption>* ExactMatch = CreationOptions.FindByPredicate([&PreviousOption](const TSharedPtr<FSmartCreationOption>& Option)
		{
			return Option.IsValid()
				&& Option->UnderlyingKind == PreviousOption->UnderlyingKind
				&& Option->BlueprintTemplateKind == PreviousOption->BlueprintTemplateKind
				&& Option->ParentClass == PreviousOption->ParentClass
				&& Option->Prefix == PreviousOption->Prefix
				&& Option->bIsUserDefined == PreviousOption->bIsUserDefined
				&& Option->bAllowDerivedClasses == PreviousOption->bAllowDerivedClasses;
		});
		const TSharedPtr<FSmartCreationOption>* StructuralMatch = ExactMatch ? nullptr : CreationOptions.FindByPredicate([&PreviousOption](const TSharedPtr<FSmartCreationOption>& Option)
		{
			return Option.IsValid()
				&& Option->UnderlyingKind == PreviousOption->UnderlyingKind
				&& Option->BlueprintTemplateKind == PreviousOption->BlueprintTemplateKind
				&& Option->ParentClass == PreviousOption->ParentClass
				&& Option->bIsUserDefined == PreviousOption->bIsUserDefined
				&& Option->bAllowDerivedClasses == PreviousOption->bAllowDerivedClasses;
		});

		if (ExactMatch)
		{
			SelectedCreationOption = *ExactMatch;
		}
		else if (StructuralMatch)
		{
			SelectedCreationOption = *StructuralMatch;
		}
	}

	if (PreviousParentClass
		&& SelectedCreationOption.IsValid()
		&& FSmartCreationOptionBuilder::DoesOptionMatchClass(*SelectedCreationOption, PreviousParentClass))
	{
		SelectedParentClass = PreviousParentClass;
	}
	else
	{
		RefreshDefaultsForCreationOption();
	}

	if (CreationOptionComboBox.IsValid())
	{
		CreationOptionComboBox->RefreshOptions();
		CreationOptionComboBox->SetSelectedItem(SelectedCreationOption);
	}

	RefreshClassViewer();
	InvalidatePreview();
}

void SSmartAssetCreateWindow::RefreshDefaultsForCreationOption()
{
	if (InitialParentBlueprint)
	{
		SelectedParentClass = InitialParentBlueprint->GeneratedClass;
		if (const UAnimBlueprint* ParentAnimBlueprint = Cast<UAnimBlueprint>(InitialParentBlueprint.Get()))
		{
			SelectedSkeleton = ParentAnimBlueprint->TargetSkeleton;
			bTemplateAnimBlueprint = ParentAnimBlueprint->bIsTemplate;
		}
		RefreshClassViewer();
		return;
	}

	if (SelectedCreationOption.IsValid() && SelectedCreationOption->ParentClass)
	{
		SelectedParentClass = SelectedCreationOption->ParentClass;
		RefreshClassViewer();
		return;
	}

	RefreshClassViewer();
}

void SSmartAssetCreateWindow::RefreshClassViewer()
{
	if (ClassViewerContainer.IsValid())
	{
		ClassViewerContainer->SetContent(CreateClassViewerWidget());
	}
}

void SSmartAssetCreateWindow::InvalidatePreview()
{
	bPreviewDirty = true;
	if (!SelectedCreationOption.IsValid())
	{
		SelectedCreationOptionDisplayName = FText::GetEmpty();
		return;
	}

	SelectedCreationOptionDisplayName = FSmartCreationOptionBuilder::MakeEffectiveDisplayName(*SelectedCreationOption, BuildRequest());
}

FString SSmartAssetCreateWindow::GetPreviewText() const
{
	const double CurrentTime = FPlatformTime::Seconds();
	if (bPreviewDirty || CurrentTime - CachedPreviewTime >= 0.5)
	{
		CachedPreviewText = OnPreviewRequested.IsBound() ? OnPreviewRequested.Execute(BuildRequest()) : FString();
		bPreviewDirty = false;
		CachedPreviewTime = CurrentTime;
	}
	return CachedPreviewText;
}

FString SSmartAssetCreateWindow::GetValidationError() const
{
	return OnValidationRequested.IsBound() ? OnValidationRequested.Execute(BuildRequest()) : FString();
}

bool SSmartAssetCreateWindow::CanCreate() const
{
	return OnCreateRequested.IsBound() && GetValidationError().IsEmpty();
}

FReply SSmartAssetCreateWindow::HandleCreateClicked()
{
	if (!OnCreateRequested.IsBound())
	{
		return FReply::Handled();
	}

	const FSmartAssetCreateResult Result = OnCreateRequested.Execute(BuildRequest());
	if (Result.bSucceeded)
	{
		CloseWindow();
	}
	else
	{
		const FText Message = Result.ErrorMessage.IsEmpty()
			? NSLOCTEXT("SmartAssetCreator", "UnknownCreateFailure", "The asset could not be created.")
			: FText::FromString(Result.ErrorMessage);
		FMessageDialog::Open(EAppMsgType::Ok, Message, NSLOCTEXT("SmartAssetCreator", "CreateFailureTitle", "Smart Asset Creation Failed"));
	}

	return FReply::Handled();
}

FReply SSmartAssetCreateWindow::HandleCancelClicked()
{
	CloseWindow();
	return FReply::Handled();
}

FReply SSmartAssetCreateWindow::HandleOpenSettingsClicked()
{
	if (OnOpenSettingsRequested.IsBound())
	{
		OnOpenSettingsRequested.Execute();
	}
	return FReply::Handled();
}

void SSmartAssetCreateWindow::CloseWindow()
{
	if (ParentWindowWeak.IsValid())
	{
		ParentWindowWeak.Pin()->RequestDestroyWindow();
	}
}

bool SSmartAssetCreateWindow::NeedsParentClass() const
{
	return !InitialParentBlueprint
		&& SelectedCreationOption.IsValid()
		&& (SelectedCreationOption->UnderlyingKind == ESmartUnderlyingAssetKind::Blueprint
			|| SelectedCreationOption->UnderlyingKind == ESmartUnderlyingAssetKind::DataAsset)
		&& SelectedCreationOption->BlueprintTemplateKind != ESmartBlueprintTemplateKind::Interface;
}

bool SSmartAssetCreateWindow::IsAnimBlueprintOption() const
{
	return SelectedCreationOption.IsValid()
		&& SelectedCreationOption->UnderlyingKind == ESmartUnderlyingAssetKind::Blueprint
		&& SelectedCreationOption->BlueprintTemplateKind == ESmartBlueprintTemplateKind::Anim;
}

bool SSmartAssetCreateWindow::IsDataTableOption() const
{
	return SelectedCreationOption.IsValid()
		&& SelectedCreationOption->UnderlyingKind == ESmartUnderlyingAssetKind::DataTable;
}

bool SSmartAssetCreateWindow::IsMaterialInstanceOption() const
{
	return SelectedCreationOption.IsValid()
		&& SelectedCreationOption->UnderlyingKind == ESmartUnderlyingAssetKind::MaterialInstance;
}

FSmartAssetCreateRequest SSmartAssetCreateWindow::BuildRequest() const
{
	FSmartAssetCreateRequest Request;
	if (SelectedCreationOption.IsValid())
	{
		Request.UnderlyingKind = SelectedCreationOption->UnderlyingKind;
		Request.BlueprintTemplateKind = SelectedCreationOption->BlueprintTemplateKind;
		Request.bHasPrefixOverride = SelectedCreationOption->bIsUserDefined;
		Request.PrefixOverride = SelectedCreationOption->Prefix;
		Request.bHasPrefixFallback = true;
		Request.PrefixFallback = SelectedCreationOption->Prefix;
	}
	Request.TargetFolder = TargetFolder;
	Request.ParentClass = SelectedParentClass;
	Request.ParentBlueprint = InitialParentBlueprint;
	Request.RowStruct = SelectedRowStruct;
	Request.TargetSkeleton = SelectedSkeleton;
	Request.ParentMaterial = SelectedParentMaterial;
	Request.bTemplateAnimBlueprint = bTemplateAnimBlueprint;
	return Request;
}

UClass* SSmartAssetCreateWindow::GetRequiredBaseClassForSelection() const
{
	if (SelectedCreationOption.IsValid() && SelectedCreationOption->ParentClass)
	{
		return SelectedCreationOption->ParentClass;
	}

	if (!SelectedCreationOption.IsValid())
	{
		return UObject::StaticClass();
	}

	switch (SelectedCreationOption->UnderlyingKind)
	{
	case ESmartUnderlyingAssetKind::DataAsset:
		return UDataAsset::StaticClass();
	default:
		return UObject::StaticClass();
	}
}

TSharedRef<SWidget> SSmartAssetCreateWindow::CreateClassViewerWidget()
{
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::TreeView;
	Options.bShowNoneOption = false;
	Options.bExpandRootNodes = true;
	Options.bExpandAllNodes = false;
	Options.bShowObjectRootClass = SelectedCreationOption.IsValid()
		&& SelectedCreationOption->UnderlyingKind == ESmartUnderlyingAssetKind::Blueprint
		&& GetRequiredBaseClassForSelection() == UObject::StaticClass();
	Options.bAllowViewOptions = false;
	Options.InitiallySelectedClass = SelectedParentClass.Get();

	const bool bAllowDerivedClasses = !SelectedCreationOption.IsValid() || SelectedCreationOption->bAllowDerivedClasses;
	TSharedPtr<FSmartAssetClassFilter> Filter = MakeShared<FSmartAssetClassFilter>(GetRequiredBaseClassForSelection(), bAllowDerivedClasses);
	Options.ClassFilters.Add(Filter.ToSharedRef());

	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	return ClassViewerModule.CreateClassViewer(
		Options,
		FOnClassPicked::CreateLambda([this](UClass* PickedClass)
		{
				SelectedParentClass = PickedClass;
				InvalidatePreview();
		})
	);
}

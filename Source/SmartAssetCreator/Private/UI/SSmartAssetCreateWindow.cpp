#include "UI/SSmartAssetCreateWindow.h"

#include "AssetRegistry/AssetData.h"
#include "Animation/AnimInstance.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Blueprint.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Animation/Skeleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInterface.h"
#include "PropertyCustomizationHelpers.h"
#include "Settings/SmartAssetSettings.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

namespace
{
	class FSmartAssetClassFilter : public IClassViewerFilter
	{
	public:
		explicit FSmartAssetClassFilter(UClass* InBaseClass)
			: BaseClass(InBaseClass)
		{
		}

		virtual bool IsClassAllowed(
			const FClassViewerInitializationOptions& InInitOptions,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return BaseClass && InClass && InClass->IsChildOf(BaseClass) && !InClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
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

			TSet<const UClass*> AllowedClasses;
			AllowedClasses.Add(BaseClass);
			return InFilterFuncs->IfInChildOfClassesSet(AllowedClasses, InUnloadedClassData) != EFilterReturn::Failed;
		}

	private:
		UClass* BaseClass = nullptr;
	};
}

void SSmartAssetCreateWindow::Construct(const FArguments& InArgs)
{
	ParentWindowWeak = InArgs._ParentWindow;
	TargetFolder = InArgs._TargetFolder;
	SelectedAssetType = InArgs._InitialAssetType;
	bLockAssetType = InArgs._LockAssetType;
	InitialParentBlueprint = InArgs._InitialParentBlueprint;
	OnCreateRequested = InArgs._OnCreateRequested;
	OnPreviewRequested = InArgs._OnPreviewRequested;
	OnDefaultClassRequested = InArgs._OnDefaultClassRequested;

	RebuildAssetTypeOptions();

	RowStructOptions.Reset();
	for (UScriptStruct* Struct : GetAvailableRowStructs())
	{
		RowStructOptions.Add(MakeShared<UScriptStruct*>(Struct));
	}

	RefreshDefaultsForAssetType();

	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SmartAssetCreator", "CreateWindowTitle", "Create Smart Asset"))
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
				BuildAssetTypePicker()
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

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildAssetTypePicker()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SmartAssetCreator", "AssetTypeLabel", "Asset Type"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(SComboBox<TSharedPtr<FSmartAssetTypeOption>>)
			.OptionsSource(&AssetTypeOptions)
			.IsEnabled(!bLockAssetType)
			.OnGenerateWidget_Lambda([](TSharedPtr<FSmartAssetTypeOption> Item)
			{
				return SNew(STextBlock).Text(Item.IsValid() ? Item->Label : FText::GetEmpty());
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<FSmartAssetTypeOption> Item, ESelectInfo::Type)
			{
				if (Item.IsValid())
				{
					SelectedAssetTypeOption = Item;
					SelectedAssetType = Item->AssetType;
					SelectedParentClass = nullptr;
					SelectedRowStruct = nullptr;
					SelectedSkeleton = nullptr;
					SelectedParentMaterial = nullptr;
					bTemplateAnimBlueprint = false;
					RefreshDefaultsForAssetType();
				}
			})
			.InitiallySelectedItem(SelectedAssetTypeOption)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return SelectedAssetTypeOption.IsValid() ? SelectedAssetTypeOption->Label : GetAssetTypeText(SelectedAssetType);
				})
			]
		];
}

TSharedRef<SWidget> SSmartAssetCreateWindow::BuildDynamicOptions()
{
	return SNew(SVerticalBox)
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
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.Visibility_Lambda([this]()
				{
					return (SelectedAssetType == ESmartAssetType::ActorBlueprint
						|| SelectedAssetType == ESmartAssetType::WidgetBlueprint
						|| SelectedAssetType == ESmartAssetType::AnimBlueprint
						|| SelectedAssetType == ESmartAssetType::DataAsset)
						? EVisibility::Visible
						: EVisibility::Collapsed;
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
					return SelectedAssetType == ESmartAssetType::AnimBlueprint ? EVisibility::Visible : EVisibility::Collapsed;
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
							[this](const FAssetData& AssetData) { SelectedSkeleton = Cast<USkeleton>(AssetData.GetAsset()); })
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bTemplateAnimBlueprint ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bTemplateAnimBlueprint = NewState == ECheckBoxState::Checked; })
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
					return SelectedAssetType == ESmartAssetType::DataTable ? EVisibility::Visible : EVisibility::Collapsed;
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
					return SelectedAssetType == ESmartAssetType::MaterialInstance ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					BuildObjectPicker(
						NSLOCTEXT("SmartAssetCreator", "ParentMaterialLabel", "Parent Material"),
						UMaterialInterface::StaticClass(),
						[this]() { return SelectedParentMaterial ? SelectedParentMaterial->GetPathName() : FString(); },
						[this](const FAssetData& AssetData) { SelectedParentMaterial = Cast<UMaterialInterface>(AssetData.GetAsset()); })
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
			SNew(SComboBox<TSharedPtr<UScriptStruct*>>)
			.OptionsSource(&RowStructOptions)
			.OnGenerateWidget_Lambda([](TSharedPtr<UScriptStruct*> Item)
			{
				return SNew(STextBlock).Text(FText::FromString((*Item)->GetName()));
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<UScriptStruct*> Item, ESelectInfo::Type)
			{
				SelectedRowStruct = Item.IsValid() ? *Item.Get() : nullptr;
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					return SelectedRowStruct
						? FText::FromString(SelectedRowStruct->GetName())
						: NSLOCTEXT("SmartAssetCreator", "PickRowStruct", "Pick Row Struct...");
				})
			]
		];
}

void SSmartAssetCreateWindow::RebuildAssetTypeOptions()
{
	AssetTypeOptions.Reset();

	auto AddBuiltInOption = [this](ESmartAssetType AssetType)
	{
		TSharedPtr<FSmartAssetTypeOption> Option = MakeShared<FSmartAssetTypeOption>();
		Option->AssetType = AssetType;
		Option->Label = GetAssetTypeText(AssetType);
		AssetTypeOptions.Add(Option);
		if (!SelectedAssetTypeOption.IsValid() && SelectedAssetType == AssetType)
		{
			SelectedAssetTypeOption = Option;
		}
	};

	AddBuiltInOption(ESmartAssetType::ActorBlueprint);
	AddBuiltInOption(ESmartAssetType::WidgetBlueprint);
	AddBuiltInOption(ESmartAssetType::AnimBlueprint);
	AddBuiltInOption(ESmartAssetType::InterfaceBlueprint);
	AddBuiltInOption(ESmartAssetType::DataAsset);
	AddBuiltInOption(ESmartAssetType::DataTable);
	AddBuiltInOption(ESmartAssetType::Material);
	AddBuiltInOption(ESmartAssetType::MaterialInstance);

	const USmartAssetSettings* Settings = GetDefault<USmartAssetSettings>();
	if (Settings)
	{
		TArray<TSharedPtr<FSmartAssetTypeOption>> CustomOptions;
		for (const FSmartAssetPrefixRule& Rule : Settings->PrefixRules)
		{
			if (!Rule.bEnabled)
			{
				continue;
			}

			UClass* RuleClass = Rule.BaseClass.TryLoadClass<UObject>();
			if (!RuleClass || IsBuiltInClassRule(RuleClass))
			{
				continue;
			}

			TSharedPtr<FSmartAssetTypeOption> Option = MakeShared<FSmartAssetTypeOption>();
			Option->AssetType = ResolveAssetTypeForClass(RuleClass);
			Option->Label = MakeClassOptionLabel(RuleClass);
			Option->PresetClass = RuleClass;
			CustomOptions.Add(Option);
		}

		CustomOptions.Sort([](const TSharedPtr<FSmartAssetTypeOption>& A, const TSharedPtr<FSmartAssetTypeOption>& B)
		{
			return A.IsValid() && B.IsValid() ? A->Label.ToString() < B->Label.ToString() : false;
		});

		AssetTypeOptions.Append(CustomOptions);
	}

	if (!SelectedAssetTypeOption.IsValid() && AssetTypeOptions.Num() > 0)
	{
		SelectedAssetTypeOption = AssetTypeOptions[0];
		SelectedAssetType = SelectedAssetTypeOption->AssetType;
	}
}

void SSmartAssetCreateWindow::RefreshDefaultsForAssetType()
{
	if (InitialParentBlueprint)
	{
		SelectedParentClass = InitialParentBlueprint->GeneratedClass;
		RefreshClassViewer();
		return;
	}

	if (SelectedAssetTypeOption.IsValid() && SelectedAssetTypeOption->PresetClass.IsValid())
	{
		SelectedParentClass = SelectedAssetTypeOption->PresetClass.Get();
		RefreshClassViewer();
		return;
	}

	if (OnDefaultClassRequested.IsBound())
	{
		SelectedParentClass = OnDefaultClassRequested.Execute(SelectedAssetType);
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

FString SSmartAssetCreateWindow::GetPreviewText() const
{
	return OnPreviewRequested.IsBound() ? OnPreviewRequested.Execute(BuildRequest()) : FString();
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

	return FReply::Handled();
}

FReply SSmartAssetCreateWindow::HandleCancelClicked()
{
	CloseWindow();
	return FReply::Handled();
}

void SSmartAssetCreateWindow::CloseWindow()
{
	if (ParentWindowWeak.IsValid())
	{
		ParentWindowWeak.Pin()->RequestDestroyWindow();
	}
}

FText SSmartAssetCreateWindow::GetAssetTypeText(ESmartAssetType AssetType) const
{
	switch (AssetType)
	{
	case ESmartAssetType::ActorBlueprint:
		return NSLOCTEXT("SmartAssetCreator", "ActorBlueprintType", "Object");
	case ESmartAssetType::WidgetBlueprint:
		return NSLOCTEXT("SmartAssetCreator", "WidgetBlueprintType", "Widget");
	case ESmartAssetType::AnimBlueprint:
		return NSLOCTEXT("SmartAssetCreator", "AnimBlueprintType", "Anim");
	case ESmartAssetType::InterfaceBlueprint:
		return NSLOCTEXT("SmartAssetCreator", "InterfaceBlueprintType", "Interface");
	case ESmartAssetType::DataAsset:
		return NSLOCTEXT("SmartAssetCreator", "DataAssetType", "Data Asset");
	case ESmartAssetType::DataTable:
		return NSLOCTEXT("SmartAssetCreator", "DataTableType", "Data Table");
	case ESmartAssetType::Material:
		return NSLOCTEXT("SmartAssetCreator", "MaterialType", "Material");
	case ESmartAssetType::MaterialInstance:
		return NSLOCTEXT("SmartAssetCreator", "MaterialInstanceType", "Material Instance");
	default:
		return FText::GetEmpty();
	}
}

ESmartAssetType SSmartAssetCreateWindow::ResolveAssetTypeForClass(const UClass* InClass) const
{
	if (!InClass)
	{
		return ESmartAssetType::ActorBlueprint;
	}

	if (InClass->IsChildOf(UUserWidget::StaticClass()))
	{
		return ESmartAssetType::WidgetBlueprint;
	}

	if (InClass->IsChildOf(UAnimInstance::StaticClass()))
	{
		return ESmartAssetType::AnimBlueprint;
	}

	if (InClass->IsChildOf(UDataAsset::StaticClass()))
	{
		return ESmartAssetType::DataAsset;
	}

	if (InClass->IsChildOf(UInterface::StaticClass()))
	{
		return ESmartAssetType::InterfaceBlueprint;
	}

	return ESmartAssetType::ActorBlueprint;
}

FText SSmartAssetCreateWindow::MakeClassOptionLabel(const UClass* InClass) const
{
	if (!InClass)
	{
		return FText::GetEmpty();
	}

	const FText DisplayName = InClass->GetDisplayNameText();
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}

	FString ClassName = InClass->GetName();
	if (ClassName.EndsWith(TEXT("_C")))
	{
		ClassName.LeftChopInline(2, EAllowShrinking::No);
	}

	return FText::FromString(ClassName);
}

bool SSmartAssetCreateWindow::IsBuiltInClassRule(const UClass* InClass) const
{
	return InClass == UObject::StaticClass()
		|| InClass == UUserWidget::StaticClass()
		|| InClass == UAnimInstance::StaticClass()
		|| InClass == UInterface::StaticClass()
		|| InClass == UDataAsset::StaticClass();
}

TArray<UScriptStruct*> SSmartAssetCreateWindow::GetAvailableRowStructs() const
{
	TArray<UScriptStruct*> Structs;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		if (Struct
			&& Struct->IsChildOf(FTableRowBase::StaticStruct())
			&& !Struct->HasMetaData(TEXT("Hidden")))
		{
			Structs.Add(Struct);
		}
	}

	Structs.Sort([](const UScriptStruct& A, const UScriptStruct& B)
	{
		return A.GetName() < B.GetName();
	});

	return Structs;
}

FSmartAssetCreateRequest SSmartAssetCreateWindow::BuildRequest() const
{
	FSmartAssetCreateRequest Request;
	Request.AssetType = SelectedAssetType;
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
	if (SelectedAssetTypeOption.IsValid() && SelectedAssetTypeOption->PresetClass.IsValid())
	{
		return SelectedAssetTypeOption->PresetClass.Get();
	}

	switch (SelectedAssetType)
	{
	case ESmartAssetType::ActorBlueprint:
		return UObject::StaticClass();
	case ESmartAssetType::WidgetBlueprint:
		return UUserWidget::StaticClass();
	case ESmartAssetType::AnimBlueprint:
		return UAnimInstance::StaticClass();
	case ESmartAssetType::DataAsset:
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
	Options.bShowObjectRootClass = SelectedAssetType == ESmartAssetType::ActorBlueprint;
	Options.bAllowViewOptions = false;
	Options.InitiallySelectedClass = SelectedParentClass.Get();

	TSharedPtr<FSmartAssetClassFilter> Filter = MakeShared<FSmartAssetClassFilter>(GetRequiredBaseClassForSelection());
	Options.ClassFilters.Add(Filter.ToSharedRef());

	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	return ClassViewerModule.CreateClassViewer(
		Options,
		FOnClassPicked::CreateLambda([this](UClass* PickedClass)
		{
			SelectedParentClass = PickedClass;
		})
	);
}

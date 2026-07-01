// Copyright (c) 2026 Wz4h. All Rights Reserved.
#include "SmartAssetCreatorModule.h"

#include "Blueprint/UserWidget.h"
#include "Brushes/SlateImageBrush.h"
#include "Containers/Ticker.h"
#include "ContentBrowserDataMenuContexts.h"
#include "ContentBrowserDataSubsystem.h"
#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserModule.h"
#include "Core/SmartAssetCreateRequest.h"
#include "Core/SmartAssetCreateResult.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "IContentBrowserSingleton.h"
#include "ISettingsModule.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Service/SmartAssetCreationService.h"
#include "Settings/SmartAssetSettings.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenuEntry.h"
#include "ToolMenus.h"
#include "UI/SSmartAssetCreateWindow.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogSmartAssetCreatorModule, Log, All);

#define LOCTEXT_NAMESPACE "FSmartAssetCreatorModule"

namespace
{
	TSharedPtr<FSlateStyleSet> GSmartAssetCreatorStyleSet;

	FString NormalizeBrowserPath(FString InPath)
	{
		InPath.TrimStartAndEndInline();
		InPath.ReplaceInline(TEXT("\\"), TEXT("/"));

		while (InPath.Len() > 1 && InPath.EndsWith(TEXT("/")))
		{
			InPath.LeftChopInline(1, EAllowShrinking::No);
		}

		if (InPath.IsEmpty())
		{
			return TEXT("/Game");
		}

		if (InPath.StartsWith(TEXT("/All/Plugins/")))
		{
			return FString::Printf(TEXT("/%s"), *InPath.RightChop(13));
		}

		if (InPath.StartsWith(TEXT("/All/Game")))
		{
			return FString::Printf(TEXT("/Game%s"), *InPath.RightChop(9));
		}

		if (InPath.StartsWith(TEXT("/All/Engine")))
		{
			return FString::Printf(TEXT("/Engine%s"), *InPath.RightChop(11));
		}

		if (!InPath.StartsWith(TEXT("/")))
		{
			return FString::Printf(TEXT("/Game/%s"), *InPath);
		}

		return InPath;
	}

	void RegisterSmartAssetCreatorStyle()
	{
		if (GSmartAssetCreatorStyleSet.IsValid())
		{
			return;
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SmartAssetCreator"));
		if (!Plugin.IsValid())
		{
			return;
		}

		GSmartAssetCreatorStyleSet = MakeShared<FSlateStyleSet>(TEXT("SmartAssetCreatorStyle"));
		GSmartAssetCreatorStyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
		GSmartAssetCreatorStyleSet->Set(
			TEXT("SmartAssetCreator.CreateAsset"),
			new FSlateImageBrush(
				GSmartAssetCreatorStyleSet->RootToContentDir(TEXT("Icon128.png")),
				FVector2D(16.0f, 16.0f))
		);

		FSlateStyleRegistry::RegisterSlateStyle(*GSmartAssetCreatorStyleSet);
	}

	void UnregisterSmartAssetCreatorStyle()
	{
		if (!GSmartAssetCreatorStyleSet.IsValid())
		{
			return;
		}

		FSlateStyleRegistry::UnRegisterSlateStyle(*GSmartAssetCreatorStyleSet);
		GSmartAssetCreatorStyleSet.Reset();
	}
}

namespace SmartAssetCreatorMenuNames
{
	static const FName AddNewMenu = "ContentBrowser.AddNewContextMenu";
	static const FName AssetMenu = "ContentBrowser.AssetContextMenu";
	static const FName SectionName = "SmartAssetCreatorSection";
	static const FName IconName = "SmartAssetCreator.CreateAsset";
}

void FSmartAssetCreatorModule::StartupModule()
{
	AssetCreationService = MakeUnique<FSmartAssetCreationService>();
	RegisterSmartAssetCreatorStyle();
	SettingsChangedHandle = USmartAssetSettings::OnSettingsChanged().AddRaw(this, &FSmartAssetCreatorModule::HandleSettingsChanged);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		ToolMenusStartupCallbackHandle = UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSmartAssetCreatorModule::RegisterMenus)
		);
	}
}

void FSmartAssetCreatorModule::ShutdownModule()
{
	USmartAssetSettings::OnSettingsChanged().Remove(SettingsChangedHandle);
	SettingsChangedHandle.Reset();

	for (const TWeakPtr<SSmartAssetCreateWindow>& WidgetWeak : OpenCreateWidgets)
	{
		if (const TSharedPtr<SSmartAssetCreateWindow> Widget = WidgetWeak.Pin())
		{
			Widget->UnbindModuleCallbacks();
		}
	}
	OpenCreateWidgets.Reset();

	for (const TWeakPtr<SWindow>& WindowWeak : OpenCreateWindows)
	{
		if (const TSharedPtr<SWindow> Window = WindowWeak.Pin())
		{
			Window->SetOnWindowClosed(FOnWindowClosed());
			Window->RequestDestroyWindow();
		}
	}
	OpenCreateWindows.Reset();

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupCallbackHandle);
		UToolMenus::UnregisterOwner(this);
	}

	if (PendingRenameTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PendingRenameTickerHandle);
		PendingRenameTickerHandle.Reset();
	}

	AssetCreationService.Reset();
	UnregisterSmartAssetCreatorStyle();
}

void FSmartAssetCreatorModule::HandleSettingsChanged()
{
	OpenCreateWidgets.RemoveAll([](const TWeakPtr<SSmartAssetCreateWindow>& WidgetWeak)
	{
		return !WidgetWeak.IsValid();
	});

	for (const TWeakPtr<SSmartAssetCreateWindow>& WidgetWeak : OpenCreateWidgets)
	{
		if (const TSharedPtr<SSmartAssetCreateWindow> Widget = WidgetWeak.Pin())
		{
			Widget->RefreshForSettingsChange();
		}
	}
}

void FSmartAssetCreatorModule::RegisterMenus()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus)
	{
		return;
	}

	FToolMenuOwnerScoped OwnerScoped(this);

	if (UToolMenu* Menu = ToolMenus->ExtendMenu(SmartAssetCreatorMenuNames::AddNewMenu))
	{
		Menu->AddDynamicSection(
			"SmartAssetCreator",
			FNewToolMenuDelegate::CreateRaw(this, &FSmartAssetCreatorModule::PopulateAddNewMenu)
		);
	}

	if (UToolMenu* AssetMenu = ToolMenus->ExtendMenu(SmartAssetCreatorMenuNames::AssetMenu))
	{
		AssetMenu->AddDynamicSection(
			"SmartAssetCreatorAsset",
			FNewToolMenuDelegate::CreateLambda([this](UToolMenu* InMenu)
			{
				const UContentBrowserAssetContextMenuContext* Context =
					InMenu->FindContext<UContentBrowserAssetContextMenuContext>();
				if (!Context || Context->SelectedAssets.Num() != 1)
				{
					return;
				}

				UBlueprint* ParentBlueprint = Cast<UBlueprint>(Context->SelectedAssets[0].GetAsset());
				if (!ParentBlueprint
					|| ParentBlueprint->BlueprintType == BPTYPE_Interface
					|| !FBlueprintEditorUtils::CanCreateChildBlueprint(ParentBlueprint))
				{
					return;
				}

				FToolMenuSection& Section = InMenu->AddSection(
					SmartAssetCreatorMenuNames::SectionName,
					LOCTEXT("SmartAssetCreatorSection", "Smart Asset Creator"),
					FToolMenuInsert("CommonAssetActions", EToolMenuInsertType::Before)
				);

					const TWeakObjectPtr<UBlueprint> ParentBlueprintWeak = ParentBlueprint;
					Section.AddEntry(
					FToolMenuEntry::InitMenuEntry(
						"SmartAssetCreator_CreateChild",
						LOCTEXT("CreateChildAsset", "Create Child Asset..."),
						LOCTEXT("CreateChildAssetTooltip", "Open Smart Asset Creator with this Blueprint as the parent."),
						FSlateIcon(TEXT("SmartAssetCreatorStyle"), SmartAssetCreatorMenuNames::IconName),
							FUIAction(FExecuteAction::CreateLambda([this, ParentBlueprintWeak]()
							{
								if (UBlueprint* ValidParentBlueprint = ParentBlueprintWeak.Get())
								{
									OpenCreateWindow(
										FPackageName::GetLongPackagePath(ValidParentBlueprint->GetOutermost()->GetName()),
										ValidParentBlueprint);
								}
							}))
					)
				);
			})
		);
	}
}

void FSmartAssetCreatorModule::PopulateAddNewMenu(UToolMenu* InMenu)
{
	const UContentBrowserDataMenuContext_AddNewMenu* Context =
		InMenu->FindContext<UContentBrowserDataMenuContext_AddNewMenu>();

	if (!Context || !Context->bContainsValidPackagePath || Context->SelectedPaths.Num() == 0)
	{
		return;
	}

	const FString TargetPath = ConvertMenuPathToInternalPath(Context->SelectedPaths[0]);

	FToolMenuSection& Section = InMenu->AddSection(
		SmartAssetCreatorMenuNames::SectionName,
		LOCTEXT("SmartAssetCreatorSection", "Smart Asset Creator"),
		FToolMenuInsert("ContentBrowserNewAdvancedAsset", EToolMenuInsertType::Before)
	);

	Section.AddEntry(
		FToolMenuEntry::InitMenuEntry(
			"SmartAssetCreator_Open",
			LOCTEXT("OpenSmartAssetCreator", "Create Smart Asset..."),
			LOCTEXT("OpenSmartAssetCreatorTooltip", "Create a common asset with automatic naming and configurable prefixes."),
			FSlateIcon(TEXT("SmartAssetCreatorStyle"), SmartAssetCreatorMenuNames::IconName),
			FUIAction(FExecuteAction::CreateLambda([this, TargetPath]()
			{
				OpenCreateWindow(TargetPath, nullptr);
			}))
		)
	);

}

void FSmartAssetCreatorModule::OpenCreateWindow(const FString& TargetFolder, UBlueprint* ParentBlueprint)
{
	const ESmartUnderlyingAssetKind InitialUnderlyingKind = ESmartUnderlyingAssetKind::Blueprint;
	constexpr float MinCreateWindowWidth = 620.0f;
	constexpr float MinCreateWindowHeight = 520.0f;

	const USmartAssetSettings* Settings = GetDefault<USmartAssetSettings>();
	const FVector2D SavedWindowSize = Settings ? Settings->CreateWindowSize : FVector2D(MinCreateWindowWidth, MinCreateWindowHeight);
	const FVector2D InitialWindowSize(
		FMath::Max(SavedWindowSize.X, MinCreateWindowWidth),
		FMath::Max(SavedWindowSize.Y, MinCreateWindowHeight)
	);

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SmartAssetCreatorWindowTitle", "Smart Asset Creator"))
		.ClientSize(InitialWindowSize)
		.MinWidth(MinCreateWindowWidth)
		.MinHeight(MinCreateWindowHeight)
		.SizingRule(ESizingRule::UserSized)
		.SupportsMinimize(true)
		.SupportsMaximize(false)
		.HasCloseButton(true);

	TSharedRef<SSmartAssetCreateWindow> CreateWidget =
		SNew(SSmartAssetCreateWindow)
			.ParentWindow(Window)
			.TargetFolder(TargetFolder)
			.InitialUnderlyingKind(InitialUnderlyingKind)
			.LockCreationOption(false)
			.InitialParentBlueprint(ParentBlueprint)
			.OnCreateRequested(FOnSmartAssetCreateRequested::CreateRaw(this, &FSmartAssetCreatorModule::HandleCreateRequest))
			.OnPreviewRequested(FOnSmartAssetPreviewRequested::CreateRaw(this, &FSmartAssetCreatorModule::HandlePreviewRequest))
			.OnValidationRequested(FOnSmartAssetValidationRequested::CreateRaw(this, &FSmartAssetCreatorModule::HandleValidationRequest))
			.OnOpenSettingsRequested(FOnSmartAssetOpenSettingsRequested::CreateRaw(this, &FSmartAssetCreatorModule::HandleOpenSettingsRequest));

	Window->SetContent(CreateWidget);
	const TWeakPtr<SSmartAssetCreateWindow> CreateWidgetWeak = CreateWidget;
	Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this, CreateWidgetWeak, MinCreateWindowWidth, MinCreateWindowHeight](const TSharedRef<SWindow>& ClosedWindow)
	{
		if (USmartAssetSettings* MutableSettings = GetMutableDefault<USmartAssetSettings>())
		{
			const FVector2D ClosedWindowSize = ClosedWindow->GetSizeInScreen();
			MutableSettings->CreateWindowSize = FVector2D(
				FMath::Max(ClosedWindowSize.X, MinCreateWindowWidth),
				FMath::Max(ClosedWindowSize.Y, MinCreateWindowHeight)
			);
			MutableSettings->SaveConfig();
		}

		OpenCreateWindows.RemoveAll([&ClosedWindow](const TWeakPtr<SWindow>& WindowWeak)
		{
			return !WindowWeak.IsValid() || WindowWeak.Pin() == ClosedWindow;
		});
		OpenCreateWidgets.RemoveAll([&CreateWidgetWeak](const TWeakPtr<SSmartAssetCreateWindow>& WidgetWeak)
		{
			return !WidgetWeak.IsValid() || WidgetWeak.Pin() == CreateWidgetWeak.Pin();
		});
	}));

	FSlateApplication::Get().AddWindow(Window);
	OpenCreateWindows.Add(Window);
	OpenCreateWidgets.Add(CreateWidget);
}
FString FSmartAssetCreatorModule::ConvertMenuPathToInternalPath(FName InMenuPath) const
{
	if (InMenuPath.IsNone())
	{
		return TEXT("/Game");
	}

	const FString RawPath = InMenuPath.ToString();
	UContentBrowserDataSubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UContentBrowserDataSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return NormalizeBrowserPath(RawPath);
	}

	FName InternalPath;
	if (Subsystem->TryConvertVirtualPath(RawPath, InternalPath) == EContentBrowserPathType::Internal && !InternalPath.IsNone())
	{
		return NormalizeBrowserPath(InternalPath.ToString());
	}

	return NormalizeBrowserPath(RawPath);
}

FSmartAssetCreateResult FSmartAssetCreatorModule::HandleCreateRequest(const FSmartAssetCreateRequest& Request) const
{
	FSmartAssetCreateResult Result = AssetCreationService->CreateAsset(Request);
	if (Result.bSucceeded && Result.CreatedAsset)
	{
		FocusAndRenameAsset(Result.CreatedAsset);
	}

	return Result;
}

FString FSmartAssetCreatorModule::HandlePreviewRequest(const FSmartAssetCreateRequest& Request) const
{
	return AssetCreationService->BuildAssetNamePreview(Request);
}

FString FSmartAssetCreatorModule::HandleValidationRequest(const FSmartAssetCreateRequest& Request) const
{
	return AssetCreationService->ValidateRequest(Request);
}

void FSmartAssetCreatorModule::HandleOpenSettingsRequest() const
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(
		TEXT("Project"),
		TEXT("Plugins"),
		TEXT("SmartAssetCreator"));
}

void FSmartAssetCreatorModule::FocusAndRenameAsset(UObject* Asset) const
{
	if (!Asset)
	{
		return;
	}

	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	IContentBrowserSingleton& ContentBrowser = ContentBrowserModule.Get();
	TArray<UObject*> Assets;
	Assets.Add(Asset);

	ContentBrowser.SyncBrowserToAssets(Assets);
	ContentBrowser.FocusPrimaryContentBrowser(false);

	if (PendingRenameTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PendingRenameTickerHandle);
		PendingRenameTickerHandle.Reset();
	}

	PendingRenameTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float)
		{
			PendingRenameTickerHandle.Reset();
			if (!FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
			{
				return false;
			}

			auto& Module = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			Module.Get().ExecuteRename(nullptr);
			return false;
		}),
		0.01f
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSmartAssetCreatorModule, SmartAssetCreator)

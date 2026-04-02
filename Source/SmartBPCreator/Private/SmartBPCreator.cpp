#include "SmartBPCreator.h"
#include "Engine/Blueprint.h"
#include "Framework/Application/SlateApplication.h"
#include "SmartBPCreationService.h"
#include "ToolMenus.h"
#include "ContentBrowserDataMenuContexts.h"
#include "ContentBrowserDataSubsystem.h"
#include "SSmartBPClassPicker.h"
#include "Widgets/SWindow.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Containers/Ticker.h"

DEFINE_LOG_CATEGORY_STATIC(LogSmartBPCreatorModule, Log, All);

#define LOCTEXT_NAMESPACE "FSmartBPCreatorModule"

namespace SmartBPCreatorMenuNames
{
	static const FName ContentBrowserAddNewMenu = "ContentBrowser.AddNewContextMenu";
}

void FSmartBPCreatorModule::StartupModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		ToolMenusStartupCallbackHandle = UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSmartBPCreatorModule::RegisterMenus)
		);
	}
}

void FSmartBPCreatorModule::ShutdownModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupCallbackHandle);
		UToolMenus::UnregisterOwner(this);
	}
}

void FSmartBPCreatorModule::RegisterMenus()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus)
	{
		return;
	}

	UToolMenu* Menu = ToolMenus->ExtendMenu(SmartBPCreatorMenuNames::ContentBrowserAddNewMenu);
	if (!Menu)
	{
		return;
	}

	Menu->AddDynamicSection(
		"SmartBPCreator",
		FNewToolMenuDelegate::CreateRaw(this, &FSmartBPCreatorModule::PopulateAddNewMenu)
	);
}

void FSmartBPCreatorModule::PopulateAddNewMenu(UToolMenu* InMenu)
{
	const UContentBrowserDataMenuContext_AddNewMenu* Context =
		InMenu->FindContext<UContentBrowserDataMenuContext_AddNewMenu>();

	if (!Context || !Context->bContainsValidPackagePath || Context->SelectedPaths.Num() == 0)
	{
		return;
	}

	const FName Path = Context->SelectedPaths[0];

	FToolMenuSection& Section = InMenu->FindOrAddSection(
		"SmartBPCreator",
		FText::GetEmpty(),
		FToolMenuInsert("ContentBrowserNewAdvancedAsset", EToolMenuInsertType::Before)
	);

	Section.AddMenuEntry(
		"SmartBP_Create",
		LOCTEXT("CreateSmartBP", "Create Blueprint (Auto Name)"),
		LOCTEXT("CreateSmartBP_Tooltip", "Create BP_XXX automatically"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FSmartBPCreatorModule::OnCreateSmartBlueprintClicked, Path)
		)
	);
}

FString FSmartBPCreatorModule::ConvertMenuPathToInternalPath(FName InMenuPath) const
{
	if (InMenuPath.IsNone())
	{
		return TEXT("/Game");
	}

	const FString RawPath = InMenuPath.ToString();

	UContentBrowserDataSubsystem* Subsystem =
		GEditor ? GEditor->GetEditorSubsystem<UContentBrowserDataSubsystem>() : nullptr;

	if (!Subsystem)
	{
		return RawPath;
	}

	FName InternalPath;
	Subsystem->TryConvertVirtualPath(RawPath, InternalPath);

	return InternalPath.IsNone() ? RawPath : InternalPath.ToString();
}

void FSmartBPCreatorModule::OnCreateSmartBlueprintClicked(FName TargetPath)
{
	const FString InternalPath = ConvertMenuPathToInternalPath(TargetPath);

	OpenClassPickerWindow(InternalPath);
}

void FSmartBPCreatorModule::OpenClassPickerWindow(const FString& TargetPath)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("PickClass", "Pick Parent Class"))
		.ClientSize(FVector2D(640, 520));

	Window->SetContent(
		SNew(SSmartBPClassPicker)
		.ParentWindow(Window)
		.OnClassPicked(FOnBPClassPicked::CreateRaw(
			this,
			&FSmartBPCreatorModule::HandleClassPicked,
			TargetPath
		))
	);

	FSlateApplication::Get().AddWindow(Window);
}

void FSmartBPCreatorModule::HandleClassPicked(UClass* PickedClass, FString TargetPath)
{
	if (!PickedClass)
	{
		return;
	}

	UBlueprint* BP = FSmartBPCreationService::CreateSmartBlueprint(PickedClass, TargetPath);
	if (!BP)
	{
		return;
	}

	// 选中
	FContentBrowserModule& CBModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	IContentBrowserSingleton& CB = CBModule.Get();

	TArray<UObject*> Assets;
	Assets.Add(BP);

	CB.SyncBrowserToAssets(Assets);
	CB.FocusPrimaryContentBrowser(false);

	// 下一帧 rename
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([](float)
		{
			if (!FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
			{
				return false;
			}

			auto& CBModule =
				FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

			CBModule.Get().ExecuteRename(nullptr);

			return false;
		}),
		0.01f
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSmartBPCreatorModule, SmartBPCreator)
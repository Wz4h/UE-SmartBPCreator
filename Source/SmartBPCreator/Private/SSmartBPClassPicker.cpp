#include "SSmartBPClassPicker.h"

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "SmartBPClassPicker"

DEFINE_LOG_CATEGORY_STATIC(LogSmartBPClassPicker, Log, All);

/**
 * Class Viewer 过滤器
 *
 * 作用：
 * 1. 只显示可以创建 Blueprint 的类
 * 2. V1 直接过滤掉不能作为 Blueprint 父类的类型
 */
class FSmartBPClassViewerFilter : public IClassViewerFilter
{
public:
	virtual bool IsClassAllowed(
		const FClassViewerInitializationOptions& InInitOptions,
		const UClass* InClass,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		if (!InClass)
		{
			return false;
		}

		// 只显示可创建 Blueprint 的类
		return FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass);
	}

	virtual bool IsUnloadedClassAllowed(
		const FClassViewerInitializationOptions& InInitOptions,
		const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		// V1 直接不考虑未加载蓝图数据，避免引入额外复杂度
		return false;
	}
};

void SSmartBPClassPicker::Construct(const FArguments& InArgs)
{
	OnClassPickedDelegate = InArgs._OnClassPicked;
	ParentWindowWeak = InArgs._ParentWindow;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			// 标题
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PickerTitle", "Pick a parent class for Blueprint"))
			]

			// 类选择器
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBox)
				.MinDesiredWidth(500.0f)
				.MinDesiredHeight(420.0f)
				[
					BuildClassViewer()
				]
			]

			// 分割线
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f)
			[
				SNew(SSeparator)
			]

			// 按钮区
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f, 0.0f))

				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ConfirmButton", "Confirm"))
					.IsEnabled_Lambda([this]()
					{
						return SelectedClass != nullptr;
					})
					.OnClicked(this, &SSmartBPClassPicker::OnConfirmClicked)
				]

				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CancelButton", "Cancel"))
					.OnClicked(this, &SSmartBPClassPicker::OnCancelClicked)
				]
			]
		]
	];
}

TSharedRef<SWidget> SSmartBPClassPicker::BuildClassViewer()
{
	FClassViewerInitializationOptions InitOptions;
	InitOptions.Mode = EClassViewerMode::ClassPicker;
	InitOptions.DisplayMode = EClassViewerDisplayMode::TreeView;
	InitOptions.bShowBackgroundBorder = true;
	InitOptions.bShowUnloadedBlueprints = false;
	InitOptions.bShowNoneOption = false;
	InitOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::ClassName;

	TSharedPtr<FSmartBPClassViewerFilter> Filter = MakeShared<FSmartBPClassViewerFilter>();
	InitOptions.ClassFilters.Add(Filter.ToSharedRef());

	FClassViewerModule& ClassViewerModule =
		FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	return ClassViewerModule.CreateClassViewer(
		InitOptions,
		FOnClassPicked::CreateSP(this, &SSmartBPClassPicker::HandleClassPicked)
	);
}

void SSmartBPClassPicker::HandleClassPicked(UClass* InClass)
{
	SelectedClass = InClass;

	if (SelectedClass)
	{
		UE_LOG(LogSmartBPClassPicker, Log, TEXT("[SmartBPCreator] Selected parent class: %s"),
			*SelectedClass->GetName());
	}
	else
	{
		UE_LOG(LogSmartBPClassPicker, Warning, TEXT("[SmartBPCreator] Selected parent class is null."));
	}
}

FReply SSmartBPClassPicker::OnConfirmClicked()
{
	if (!SelectedClass)
	{
		UE_LOG(LogSmartBPClassPicker, Warning, TEXT("[SmartBPCreator] Confirm clicked, but no class selected."));
		return FReply::Handled();
	}

	OnClassPickedDelegate.ExecuteIfBound(SelectedClass);

	CloseOwnerWindow();
	return FReply::Handled();
}

FReply SSmartBPClassPicker::OnCancelClicked()
{
	CloseOwnerWindow();
	return FReply::Handled();
}

void SSmartBPClassPicker::CloseOwnerWindow()
{
	if (ParentWindowWeak.IsValid())
	{
		ParentWindowWeak.Pin()->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
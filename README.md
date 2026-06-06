# SmartAssetCreator

Unreal Engine editor plugin for creating common assets with automatic naming and configurable prefix rules.

## Features

- Create common assets directly from Content Browser.
- Built-in support for:
  - Actor Blueprint -> `BP_`
  - Widget Blueprint -> `WBP_`
  - Anim Blueprint -> `ABP_`
  - Interface Blueprint -> `BPI_`
  - Data Asset -> `DA_`
  - Data Table -> `DT_`
  - Material -> `M_`
  - Material Instance -> `MI_`
- Create child assets from supported Blueprint assets. Interface and Function Library Blueprints are excluded.
- Show a live name preview before creation.
- Automatically select the new asset and enter rename mode.
- Do not auto-open the created asset editor.
- Resolve name collisions with `_01`, `_02`, and so on.
- Support user-defined prefix rules matched by inheritance distance.

## Entry Points

- Content Browser empty area or `Add New` menu:
  `Create Smart Asset...`
- Blueprint asset right-click menu:
  `Create Child Asset...`

## Settings

Open:

`Project Settings -> Plugins -> SmartAssetCreator`

You can configure:

- Fixed asset-type prefixes for `Data Table`, `Material`, and `Material Instance`
- Class prefix rules for `Actor Blueprint`, `Widget Blueprint`, `Anim Blueprint`, `Interface Blueprint`, and `Data Asset`
- Whether child blueprints reuse the parent blueprint asset name
- Whether known prefixes are stripped from parent blueprint names
- Child blueprint suffix

## Workflow

1. Open Content Browser.
2. Choose `Create Smart Asset...`.
3. Select the asset type.
4. Select the parent class or required parameters.
5. Check the live name preview.
6. Click `Create`.
7. Rename the created asset immediately if needed.

## Architecture

- `Core`: request and result models
- `Rule`: prefix rules and resolver
- `Settings`: persistent user configuration
- `Creator`: asset-type-specific creation logic
- `Service`: unified creation orchestration
- `UI`: create window
- `Module`: menu registration, windows, icon style, and Content Browser integration

## Installation

Project plugin:

```text
YourProject/Plugins/SmartAssetCreator/
```

Engine plugin:

```text
UE_5.x/Engine/Plugins/Marketplace/SmartAssetCreator/
```

Then:

1. Enable the plugin in Unreal Editor.
2. Restart the editor if prompted.

## Technical Notes

- Uses `ToolMenus` to inject Content Browser entries.
- Converts virtual Content Browser paths to internal package paths.
- Uses a creator-dispatch architecture so new asset types can be added cleanly.
- Keeps naming rules and creation logic separate for easier extension.
- Class-based assets resolve prefixes from class rules only; fixed asset types use asset-type prefixes.

## Current Status

This repository now runs on the new `SmartAssetCreator` module and supports the M1 to M4 migration scope:

- module and architecture migration
- configurable prefix rules
- creator dispatch for multiple asset types
- unified asset creation window and settings window

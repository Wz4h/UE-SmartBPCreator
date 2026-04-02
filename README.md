# SmartBPCreator

Unreal Engine Editor Plugin

---

## ✨ Features

* Create Blueprint with automatic naming (`BP_XXX`)
* Works directly in Content Browser **right-click (empty area / Add New menu)**
* Automatically selects the new asset
* Instantly enters rename mode
* No need to manually open Blueprint editor

---

## 📦 Usage

1. Open Content Browser
2. Right-click in an empty area (or use Add New menu)
3. Click **Create Blueprint (Auto Name)**
4. Select a parent class
5. Blueprint is created and ready to rename

---

## 🎯 Naming Rule

```
BP_ + ClassName
```

### Examples

```
Actor → BP_Actor
Character → BP_Character
UserWidget → BP_UserWidget
```

---

## 🧠 Why this plugin?

### Default UE workflow

```
Create Blueprint
→ Rename
→ Open Blueprint Editor
→ Close
```

### With SmartBPCreator

```
Create Blueprint
→ Rename (Done)
```

👉 Faster, cleaner, and closer to real usage habits

---

## 🔧 Technical Notes

* Uses `ContentBrowser ToolMenus` to inject menu entry
* Converts virtual path → internal package path (fixes invisible asset issue)
* Handles name conflicts safely (no crash on duplicate names)
* Uses `FTSTicker` to trigger rename in next frame (ensures UI sync)

---

## 🛠 Requirements

* Unreal Engine 5.x
* Editor module plugin

---

## 📁 Installation

1. Download or clone this repository
2. Put the plugin into:

```
YourProject/Plugins/SmartBPCreator/
```

3. Open Unreal Engine
4. Enable plugin in **Edit → Plugins**
5. Restart Editor

---

## 📌 Current Status

**V1.0.0**

* Core workflow complete
* Stable for daily use

---

## 🚀 Future Plans

* Widget Blueprint support
* Anim Blueprint support
* Custom naming rules
* Folder auto-classification (UI / Gameplay / etc.)
* Batch creation support

---

## 📷 Preview

(Add screenshot here)

---

## 📄 License

MIT (or your choice)

---

## 💡 Author Notes

This plugin focuses on **high-frequency workflow optimization** rather than feature complexity.

The goal is simple:

> Remove friction from Blueprint creation.

---

## ⭐ If you find this useful

Give it a star on GitHub 👍

---

## 📬 Feedback

Feel free to open issues or suggestions.

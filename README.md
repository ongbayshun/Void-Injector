# ⚡ Void Injector

<p align="center">
  <b>A modern Windows DLL loading utility built with C++ and Win32</b>
</p>

<p align="center">

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Windows-0078D6)
![API](https://img.shields.io/badge/API-Win32-purple)
![Version](https://img.shields.io/badge/Version-1.0.0-green)

</p>

---

## 📌 Overview

**Void Injector** is a lightweight Windows utility created to explore native **C++ development**, **Windows APIs**, **process management**, and **custom desktop interfaces**.

The project features a custom-built graphical interface, configurable settings, process monitoring, and DLL loading functionality.

Built from the ground up using native Windows technologies without external UI frameworks.

---

## ✨ Features

## 🎨 Custom Interface

- 🌙 Modern dark-themed UI
- 🖥️ Custom Win32 graphics rendering
- 🎨 Customizable color themes
- 📊 Live status indicators
- 📌 System tray support
- ⚡ Lightweight performance

---

## ⚙️ Configuration

- 💾 Persistent settings
- 🎨 Theme saving
- 🪟 Window preferences
- ⏱️ Configurable options
- 🔧 User-controlled behaviour

---

## 🔍 Process Management

- 🔎 Application detection
- 📈 Process monitoring
- 📝 Status tracking
- 🖥️ Process information display

---

## 🧩 DLL Management

- 📂 File browser integration
- 📁 DLL path management
- ⚙️ Loading workflow
- 👤 User-selected files

---

# 🛠️ Built With

```
C++
Win32 API
Windows GDI
Windows Registry API
Visual Studio Toolchain
Windows SDK
```

---

# 🚀 Building

## Requirements

Before building, make sure you have:

```
✔ Windows 10 / Windows 11
✔ Visual Studio 2022
✔ Windows SDK
```

---

## Compile

```bash
rc resource.rc

cl /EHsc /DUNICODE /D_UNICODE /Fe:Void.exe main.cpp resource.res ^
user32.lib gdi32.lib comdlg32.lib shell32.lib comctl32.lib advapi32.lib
```

---

# 📂 Project Structure

```
VoidInjector
│
├── main.cpp
├── resource.rc
├── resource.h
├── README.md
│
└── assets
    └── icons
```

---

# 📸 Screenshots

Add screenshots here:

```
/screenshots

<img width="615" height="521" alt="image" src="https://github.com/user-attachments/assets/796c64c6-5e6c-41b6-9f91-d0a76cbf44a9" />

<img width="238" height="198" alt="image" src="https://github.com/user-attachments/assets/1c19b704-54f7-456a-a6dc-3a604e7b8414" />

```

---

# 🔒 Disclaimer

⚠️ **Important**

This project is created for **educational and development purposes**.

The goal of this project is to explore:

- Native C++ programming
- Windows API development
- GUI engineering
- Process interaction concepts

Only use this software with applications and systems you own or have permission to modify.

The author is not responsible for misuse of this project.

---

# 🤝 Contributing

Contributions, suggestions, and improvements are welcome.

To contribute:

```
1. Fork this repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request
```

---

# ⭐ Support

If you find this project interesting:

⭐ Consider starring the repository

It helps support future improvements and development.


---

<p align="center">
  Made with ❤️ using C++
</p>

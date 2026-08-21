# Void Injector

<p align="center">
  <b>A lightweight Windows utility built with C++ and Win32 API</b>
  <br>
  Exploring native Windows development, GUI design, and process management.
</p>

<p align="center">

![C++](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Windows](https://img.shields.io/badge/Platform-Windows-0078D6)
![Win32](https://img.shields.io/badge/API-Win32-purple)
![Version](https://img.shields.io/badge/Version-1.0.0-green)

</p>

---

## Overview

**Void Injector** is a native Windows application built in **C++** using the **Win32 API**.

The project focuses on exploring:

- Native C++ development
- Windows API interaction
- Custom desktop application design
- GUI development
- Process management concepts

The application features a custom-built interface, configurable settings, theme customization, and a lightweight native design.

---

## Features

### Custom GUI

- Custom Win32 interface
- Dark themed design
- Custom color themes
- Status indicators
- Interactive UI elements
- System tray support
- Lightweight rendering system

---

### File Management

- Built-in file browser
- File selection system
- Saved preferences
- Clear/reset options
- Live operation status

---

### Process Monitoring

- Application detection
- Real-time process checking
- Process ID display
- Automatic status updates
- Launch monitoring

---

### Automation & Settings

- Configurable startup options
- Adjustable delay settings
- Start with Windows option
- Start minimized option
- Always-on-top mode
- Automatic monitoring options

---

### Theme System

- Custom background colors
- Custom accent colors
- Custom panel styling
- Saved theme preferences
- Reset to default theme option

---

### Status System

- Ready state tracking
- Active operation indicators
- Error reporting
- Detailed status messages
- Live interface updates

---

## Built With

```
C++
Win32 API
Windows GDI
Windows Registry API
Visual Studio
Windows SDK
```

---

## Building

### Requirements

```
Windows 10 / Windows 11
Visual Studio 2022
Windows SDK
```

---

### Compile

```bash
rc resource.rc

cl /EHsc /DUNICODE /D_UNICODE /Fe:Void.exe main.cpp resource.res ^
user32.lib gdi32.lib comdlg32.lib shellapi.lib comctl32.lib advapi32.lib
```

---

## Project Structure

```
VoidInjector
│
├── main.cpp
├── resource.rc
├── resource.h
├── README.md
└── .ico
```

---

## Screenshots

<img width="615" height="521" alt="image" src="https://github.com/user-attachments/assets/8f962b42-44b0-4f0c-9460-176badeeed14" />

<img width="238" height="198" alt="image" src="https://github.com/user-attachments/assets/9ba43fc7-d9c2-4580-8825-1927e00206a4" />

---

## Disclaimer

This project was created for educational and development purposes.

The goal of this project is to explore:

- Native Windows programming
- C++ application development
- GUI engineering
- Windows API concepts

Only use this software with applications and systems you own or have permission to modify.

The author is not responsible for misuse of this project.

---

## Contributing

Contributions, suggestions, and improvements are welcome.

To contribute:

```
1. Fork the repository
2. Create a new branch
3. Make your changes
4. Submit a pull request
```

---

## Support

If you find this project useful, consider starring the repository.


---

<p align="center">
Made with C++ on Windows
</p>

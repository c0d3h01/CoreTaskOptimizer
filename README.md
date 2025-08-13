# Task Optimizer – Magisk Module

**Boost system responsiveness, improve gaming performance, and smooth out UI interactions — automatically.**
Task Optimizer intelligently adjusts CPU affinity, priority, and I/O scheduling for critical Android system processes.

[![Android Build Test](https://github.com/c0d3h01/CoreTaskOptimizer/actions/workflows/checks.yml/badge.svg?branch=main)](https://github.com/c0d3h01/CoreTaskOptimizer/actions/workflows/checks.yml)

---

## Features

* **High-priority tuning** for essential Android services (`system_server`, `zygote`, `surfaceflinger`, etc.)
* **Real-time performance boost** for graphics and touch input threads
* **Background process throttling** to free up resources for active tasks
* **Launcher optimization** for smoother home screen animations
* **Touch & display IRQ tuning** for faster input response
* **Automatic logging** for transparency and troubleshooting

---

## Installation

1. Download the latest release ZIP.
2. Open **Magisk Manager**.
3. Tap **Modules** → **Install from storage**.
4. Select the downloaded ZIP and install.
5. Reboot your device.

---

## How It Works

* Detects running processes and threads using `/proc`
* Matches them against a curated list of **high**, **real-time**, and **low priority** patterns
* Applies:

  * **CPU affinity masks** (performance vs. efficiency cores)
  * **Nice values** (process scheduling priority)
  * **Real-time scheduling** for critical rendering/touch threads
  * **I/O scheduling classes** for background tasks
* Logs all actions to:

    * `/data/adb/modules/task_optimizer/logs/main.log`
    * `/data/adb/modules/task_optimizer/logs/error.log`

---

## Requirements

* Root access via **Magisk**
* Android 8.0+ recommended
* BusyBox - No need (Magisk has its inbuilt)

---

> [!CAUTION]    
> I am not responsible for any kind damage of software or hardware      
> Use at your own risk!     

---

> [!NOTE]      
> All changes are **temporary** — they reset after reboot unless the module runs again.     
> Safe to uninstall from Magisk at any time.        
> May have no effect on heavily customized vendor kernels that override CPU settings.       

---

## License

**This project is licensed under the** [MIT License](LICENSE)
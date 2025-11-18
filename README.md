# OFS (Omni File System) - Phase 1

**OFS** is a multi-user, persistent file system implemented from scratch. It features a high-performance C++ backend server that manages data using custom data structures (AVL Trees, N-ary Trees, Bitmaps) and a professional Python PyQt6 graphical user interface.

## 🚀 Features

* **Custom Storage:** All data is stored in a single binary container (`.omni`).
* **Persistence:** Automatic save-on-edit ensures data survives server restarts.
* **Multi-Threading:** Handles multiple concurrent client connections via a TCP socket server.
* **Data Safety:** Uses a FIFO queue architecture to process operations sequentially, preventing race conditions.
* **Security:** Session-based authentication with Admin and Normal user roles.
* **Graphical UI:** A modern, dark-themed desktop application built with Python and PyQt6.

---

## 🛠️ Prerequisites

Before running the project, ensure you have the following installed:

1.  **C++ Compiler:** GCC or Clang with C++17 support.
2.  **CMake:** Version 3.10 or higher.
3.  **Python 3:** For running the client UI.
4.  **PyQt6 Library:** Install it using pip:
    ```bash
    pip install PyQt6
    ```

---

## ⚙️ Installation & Build

### 1. Build the Server
The server is written in C++ and uses CMake for building.

1.  Open a terminal in the project root (`file-verse/`).
2.  Create a build directory and compile:

    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

This will create two executables in the `build/` folder:
* `ofs_server_run`: The main server executable.
* `ofs_client`: A command-line testing client (optional).

---

## 🖥️ How to Run

You will need two separate terminal windows.

### Step 1: Start the Server
In the **first terminal**, run the server from the project root:

```bash
./build/ofs_server_run
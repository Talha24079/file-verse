# OFS User Guide

## 1. System Requirements
* **OS:** Linux (Ubuntu recommended)
* **Build Tools:** `cmake`, `make`, `g++` (C++17)
* **UI Requirements:** Python 3.x, `PyQt6` library (`pip install PyQt6`)

## 2. Building the Server
1.  Navigate to the project root (`file-verse/`).
2.  Create a build directory and compile:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
    This creates the `ofs_server_run` executable.

## 3. Running the System

### Start the Server
In a terminal, run:
```bash
./build/ofs_server_run
```
- On the first run, it will format a new ```my_filesystem.omni``` file.

- It will listen on ```127.0.0.1:8080```

### Start the UI

In a second terminal, run:

```bash 
python3 ofs_gui.py
```
## 4. Using the Application

### Login

Default Admin Credentials:
- Username: ```admin```
- Password: ```admin123```

**Roles**: 
- **Admin**: Has access to "User Management" and "System Stats" tabs.

- **Normal**: Only has access to the "File System" tab.

### File Operations

-    **Create**: Click "New Folder" or "New File" in the toolbar.

-   **Read/Edit**: Double-click a file to open the editor window. Make changes and click "Save".

-    **Delete**: Select a file/folder and click "Delete". (Note: Folders must be empty to be deleted).

-    **Metadata**: Click "Metadata" to view file size, permissions, and block usage.

### Admin Features

- **User Management**: Use the tab to create new users (assigning them "normal" or "admin" roles) or delete existing users.

- **System Stats**: View real-time disk usage and total file counts
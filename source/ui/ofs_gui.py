import sys
import socket
import json
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                             QHBoxLayout, QLabel, QLineEdit, QPushButton, 
                             QTreeWidget, QTreeWidgetItem, QMessageBox, 
                             QInputDialog, QTabWidget, QFrame, QHeaderView,
                             QStatusBar, QMenu, QTextEdit, QDialog, QStyleFactory,
                             QStyle, QFormLayout, QGroupBox, QComboBox, QProgressBar)
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QFont, QIcon, QPalette, QColor, QAction

SERVER_IP = '127.0.0.1'
SERVER_PORT = 8080

class ThemeManager:
    @staticmethod
    def set_dark_theme(app):
        app.setStyle("Fusion")
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(53, 53, 53))
        palette.setColor(QPalette.ColorRole.WindowText, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.Base, QColor(25, 25, 25))
        palette.setColor(QPalette.ColorRole.AlternateBase, QColor(53, 53, 53))
        palette.setColor(QPalette.ColorRole.ToolTipBase, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.ToolTipText, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.Text, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.Button, QColor(53, 53, 53))
        palette.setColor(QPalette.ColorRole.ButtonText, Qt.GlobalColor.white)
        palette.setColor(QPalette.ColorRole.BrightText, Qt.GlobalColor.red)
        palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))
        palette.setColor(QPalette.ColorRole.Highlight, QColor(42, 130, 218))
        palette.setColor(QPalette.ColorRole.HighlightedText, Qt.GlobalColor.black)
        app.setPalette(palette)

    @staticmethod
    def set_light_theme(app):
        app.setStyle("Fusion")
        app.setPalette(QApplication.style().standardPalette())

class OFSClient:
    def __init__(self):
        self.session_id = ""
        self.username = ""
        self.role = ""

    def send_request(self, operation, params=None):
        if params is None: params = {}
        req = {"operation": operation, "parameters": params}
        if self.session_id: req["session_id"] = self.session_id

        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((SERVER_IP, SERVER_PORT))
            msg = json.dumps(req) + "\n"
            sock.sendall(msg.encode('utf-8'))
            f = sock.makefile('r', encoding='utf-8')
            response_line = f.readline()
            sock.close()
            if not response_line: return {"status": "error", "error_message": "Empty response"}
            return json.loads(response_line)
        except Exception as e:
            return {"status": "error", "error_message": str(e)}

class LoginWindow(QWidget):
    def __init__(self, client, on_success):
        super().__init__()
        self.client = client
        self.on_success = on_success
        self.initUI()

    def initUI(self):
        self.setWindowTitle("OFS Login")
        self.setFixedSize(400, 300)
        
        layout = QVBoxLayout()
        layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        
        title = QLabel("OFS Secure Login")
        title.setFont(QFont("Segoe UI", 20, QFont.Weight.Bold))
        layout.addWidget(title, alignment=Qt.AlignmentFlag.AlignCenter)
        
        layout.addSpacing(20)
        
        self.user_input = QLineEdit()
        self.user_input.setPlaceholderText("Username")
        self.user_input.setText("admin")
        layout.addWidget(self.user_input)
        
        self.pass_input = QLineEdit()
        self.pass_input.setPlaceholderText("Password")
        self.pass_input.setEchoMode(QLineEdit.EchoMode.Password)
        self.pass_input.setText("admin123")
        self.pass_input.returnPressed.connect(self.do_login)
        layout.addWidget(self.pass_input)
        
        layout.addSpacing(20)
        
        btn = QPushButton("Login")
        btn.setFixedHeight(40)
        btn.setStyleSheet("background-color: #0078d4; color: white; font-weight: bold; border-radius: 5px;")
        btn.clicked.connect(self.do_login)
        layout.addWidget(btn)
        
        self.setLayout(layout)

    def do_login(self):
        user = self.user_input.text()
        pw = self.pass_input.text()
        
        if not user or not pw:
            QMessageBox.warning(self, "Input Error", "Please enter username and password")
            return

        resp = self.client.send_request("user_login", {"username": user, "password": pw})
        
        if resp and resp.get("status") == "success":
            self.client.session_id = resp["data"]["session_id"]
            self.client.username = user
            
            self.client.role = resp["data"].get("role", "normal")
            
            self.on_success()
        else:
            msg = resp.get("error_message", "Unknown error") if resp else "Connection failed"
            QMessageBox.critical(self, "Login Failed", f"Error: {msg}")

class MainWindow(QMainWindow):
    def __init__(self, client, on_logout):
        super().__init__()
        self.client = client
        self.on_logout = on_logout
        self.current_path = "/"
        self.is_dark = True
        self.initUI()

    def initUI(self):
        self.setWindowTitle(f"OFS Dashboard - {self.client.username}")
        self.resize(1000, 700)
        
        central = QWidget()
        self.setCentralWidget(central)
        main_layout = QVBoxLayout(central)
        
        header = QHBoxLayout()
        user_label = QLabel(f"User: {self.client.username} ({self.client.role})")
        user_label.setFont(QFont("Segoe UI", 12, QFont.Weight.Bold))
        header.addWidget(user_label)
        
        header.addStretch()
        
        theme_btn = QPushButton("Toggle Theme")
        theme_btn.clicked.connect(self.toggle_theme)
        header.addWidget(theme_btn)
        
        logout_btn = QPushButton("Logout")
        logout_btn.setStyleSheet("background-color: #d9534f; color: white; font-weight: bold;")
        logout_btn.clicked.connect(self.do_logout)
        header.addWidget(logout_btn)
        
        main_layout.addLayout(header)
        
        self.tabs = QTabWidget()
        
        self.fs_tab = QWidget()
        self.setup_fs_tab()
        self.tabs.addTab(self.fs_tab, "File System")
    
        if self.client.role == "admin":
            self.admin_tab = QWidget()
            self.setup_admin_tab()
            self.tabs.addTab(self.admin_tab, "User Management")
            
            self.stats_tab = QWidget()
            self.setup_stats_tab()
            self.tabs.addTab(self.stats_tab, "System Stats")
            
        main_layout.addWidget(self.tabs)
        
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")

        self.refresh_files()
        if self.client.role == "admin":
            self.refresh_users()
            self.refresh_stats()

    def setup_fs_tab(self):
        layout = QVBoxLayout(self.fs_tab)
        
        toolbar = QHBoxLayout()
        
        self.path_input = QLineEdit("/")
        self.path_input.setReadOnly(True)
        toolbar.addWidget(QLabel("Path:"))
        toolbar.addWidget(self.path_input)
        
        up_btn = QPushButton("Up")
        up_btn.clicked.connect(self.go_up)
        toolbar.addWidget(up_btn)
        
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(self.refresh_files)
        toolbar.addWidget(refresh_btn)
        
        layout.addLayout(toolbar)
        
        actions = QHBoxLayout()
        btn_mkdir = QPushButton("New Folder")
        btn_mkdir.clicked.connect(self.create_dir)
        actions.addWidget(btn_mkdir)
        
        btn_touch = QPushButton("New File")
        btn_touch.clicked.connect(self.create_file)
        actions.addWidget(btn_touch)
        
        actions.addStretch()
        
        layout.addLayout(actions)

        self.file_tree = QTreeWidget()
        self.file_tree.setHeaderLabels(["Name", "Type", "Size"])
        self.file_tree.setColumnWidth(0, 400)
        self.file_tree.itemDoubleClicked.connect(self.on_file_double_click)
        self.file_tree.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.file_tree.customContextMenuRequested.connect(self.open_fs_context)
        layout.addWidget(self.file_tree)

    def setup_admin_tab(self):
        layout = QVBoxLayout(self.admin_tab)
        
        toolbar = QHBoxLayout()
        btn_refresh = QPushButton("Refresh List")
        btn_refresh.clicked.connect(self.refresh_users)
        toolbar.addWidget(btn_refresh)
        
        btn_add = QPushButton("Add User")
        btn_add.clicked.connect(self.add_user)
        toolbar.addWidget(btn_add)
        
        toolbar.addStretch()
        layout.addLayout(toolbar)
        
        self.user_tree = QTreeWidget()
        self.user_tree.setHeaderLabels(["Username", "Role", "Active"])
        self.user_tree.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.user_tree.customContextMenuRequested.connect(self.open_user_context)
        layout.addWidget(self.user_tree)

    def setup_stats_tab(self):
        layout = QFormLayout(self.stats_tab)
        
        self.lbl_total = QLabel("Loading...")
        self.lbl_used = QLabel("Loading...")
        self.lbl_free = QLabel("Loading...")
        self.lbl_files = QLabel("Loading...")
        self.lbl_users = QLabel("Loading...")
        self.bar = QProgressBar()
        
        layout.addRow("Total Size:", self.lbl_total)
        layout.addRow("Used Space:", self.lbl_used)
        layout.addRow("Free Space:", self.lbl_free)
        layout.addRow("Total Files:", self.lbl_files)
        layout.addRow("Total Users:", self.lbl_users)
        layout.addRow("Usage:", self.bar)
        
        refresh_btn = QPushButton("Refresh Stats")
        refresh_btn.clicked.connect(self.refresh_stats)
        layout.addRow(refresh_btn)

    def refresh_files(self):
        self.file_tree.clear()
        self.path_input.setText(self.current_path)
        self.status_bar.showMessage("Loading files...")
        
        resp = self.client.send_request("dir_list", {"path": self.current_path})
        if resp.get("status") == "success":
            items = sorted(resp["data"], key=lambda x: (x["type"] != "directory", x["name"]))
            for item in items:
                node = QTreeWidgetItem(self.file_tree)
                node.setText(0, item["name"])
                node.setText(1, item["type"])
                node.setText(2, str(item["size"]) + " B")
                
                if item["type"] == "directory":
                    icon = QApplication.style().standardIcon(QStyle.StandardPixmap.SP_DirIcon)
                else:
                    icon = QApplication.style().standardIcon(QStyle.StandardPixmap.SP_FileIcon)
                node.setIcon(0, icon)
            self.status_bar.showMessage(f"Loaded {len(items)} items.")
        else:
            self.status_bar.showMessage(f"Error: {resp.get('error_message')}")

    def go_up(self):
        if self.current_path == "/": return
        self.current_path = "/".join(self.current_path.rstrip('/').split('/')[:-1])
        if not self.current_path: self.current_path = "/"
        self.refresh_files()

    def on_file_double_click(self, item):
        name = item.text(0)
        ftype = item.text(1)
        if ftype == "directory":
            self.current_path = f"{self.current_path.rstrip('/')}/{name}"
            self.refresh_files()
        else:
            self.read_file(name)

    def create_dir(self):
        name, ok = QInputDialog.getText(self, "New Directory", "Name:")
        if ok and name:
            path = f"{self.current_path.rstrip('/')}/{name}"
            self.client.send_request("dir_create", {"path": path})
            self.refresh_files()

    def create_file(self):
        name, ok = QInputDialog.getText(self, "New File", "Name:")
        if ok and name:
            content, ok2 = QInputDialog.getMultiLineText(self, "File Content", "Initial Content:")
            if not ok2: content = ""
            
            path = f"{self.current_path.rstrip('/')}/{name}"
            size = len(content)
            
            resp = self.client.send_request("file_create", {"path": path, "size": int(size)})
            if resp.get("status") == "success":
                if size > 0:
                    self.client.send_request("file_edit", {"path": path, "data": content})
                self.refresh_files()
            else:
                QMessageBox.warning(self, "Error", resp.get("error_message", "Failed"))

    def read_file(self, name):
        path = f"{self.current_path.rstrip('/')}/{name}"
        resp = self.client.send_request("file_read", {"path": path})
        if resp.get("status") == "success":
            content = resp["data"].get("content", "")
            self.show_editor(path, content)
        else:
            QMessageBox.warning(self, "Error", resp.get("error_message", "Read failed"))

    def show_editor(self, path, content):
        dlg = QDialog(self)
        dlg.setWindowTitle(f"Edit: {path}")
        dlg.resize(600, 400)
        layout = QVBoxLayout(dlg)
        
        txt = QTextEdit()
        txt.setText(content)
        txt.setFont(QFont("Consolas", 11))
        layout.addWidget(txt)
        
        btn = QPushButton("Save")
        def save():
            data = txt.toPlainText()
            resp = self.client.send_request("file_edit", {"path": path, "data": data})
            if resp.get("status") == "success":
                dlg.accept()
                self.refresh_files()
            else:
                QMessageBox.warning(dlg, "Error", resp.get("error_message", "Save failed"))
        btn.clicked.connect(save)
        layout.addWidget(btn)
        dlg.exec()

    def delete_file_item(self, item):
        name = item.text(0)
        ftype = item.text(1)
        path = f"{self.current_path.rstrip('/')}/{name}"
        op = "dir_delete" if ftype == "directory" else "file_delete"
        
        if QMessageBox.question(self, "Delete", f"Delete {name}?") == QMessageBox.StandardButton.Yes:
            resp = self.client.send_request(op, {"path": path})
            if resp.get("status") == "success":
                self.refresh_files()
            else:
                QMessageBox.warning(self, "Error", resp.get("error_message"))

    def show_metadata(self):
        item = self.file_tree.currentItem()
        if not item: return
        name = item.text(0)
        path = f"{self.current_path.rstrip('/')}/{name}"
        
        resp = self.client.send_request("get_metadata", {"path": path})
        if resp.get("status") == "success":
            d = resp["data"]
            info = f"""
            Path: {d['path']}
            Size: {d['entry']['size']} bytes
            Permissions: {d['entry']['permissions']}
            Blocks Used: {d['blocks_used']}
            """
            QMessageBox.information(self, "File Metadata", info)
        else:
             QMessageBox.warning(self, "Error", resp.get("error_message", "Failed"))

    def open_fs_context(self, pos):
        item = self.file_tree.itemAt(pos)
        if not item: return
        menu = QMenu()
        act_open = QAction("Open/Edit", self)
        act_open.triggered.connect(lambda: self.on_file_double_click(item))
        
        act_meta = QAction("Properties", self)
        act_meta.triggered.connect(self.show_metadata)

        act_del = QAction("Delete", self)
        act_del.triggered.connect(lambda: self.delete_file_item(item))
        
        menu.addAction(act_open)
        menu.addAction(act_meta)
        menu.addSeparator()
        menu.addAction(act_del)
        menu.exec(self.file_tree.viewport().mapToGlobal(pos))

    def refresh_users(self):
        self.user_tree.clear()
        resp = self.client.send_request("user_list")
        if resp.get("status") == "success":
            for u in resp["data"]:
                item = QTreeWidgetItem(self.user_tree)
                item.setText(0, u["username"])
                item.setText(1, u["role"])
                item.setText(2, "Active" if u["is_active"] else "Inactive")

    def add_user(self):
        dialog = QDialog(self)
        dialog.setWindowTitle("Add User")
        layout = QFormLayout(dialog)
        u_in = QLineEdit()
        p_in = QLineEdit()
        p_in.setEchoMode(QLineEdit.EchoMode.Password)
        r_combo = QComboBox()
        r_combo.addItems(["normal", "admin"])
        
        layout.addRow("Username:", u_in)
        layout.addRow("Password:", p_in)
        layout.addRow("Role:", r_combo)
        
        btn = QPushButton("Create")
        def create():
            self.client.send_request("user_create", {
                "username": u_in.text(), "password": p_in.text(), "role": r_combo.currentText()
            })
            self.refresh_users()
            dialog.accept()
        btn.clicked.connect(create)
        layout.addWidget(btn)
        dialog.exec()

    def delete_user_item(self, item):
        name = item.text(0)
        if name == "admin": 
            QMessageBox.warning(self, "Error", "Cannot delete root admin")
            return
        if QMessageBox.question(self, "Delete", f"Delete user {name}?") == QMessageBox.StandardButton.Yes:
            self.client.send_request("user_delete", {"username": name})
            self.refresh_users()

    def open_user_context(self, pos):
        item = self.user_tree.itemAt(pos)
        if not item: return
        menu = QMenu()
        act_del = QAction("Delete User", self)
        act_del.triggered.connect(lambda: self.delete_user_item(item))
        menu.addAction(act_del)
        menu.exec(self.user_tree.viewport().mapToGlobal(pos))

    def refresh_stats(self):
        resp = self.client.send_request("get_stats")
        if resp.get("status") == "success":
            d = resp["data"]
            self.lbl_total.setText(f"{d['total_size']} bytes")
            self.lbl_used.setText(f"{d['used_space']} bytes")
            self.lbl_free.setText(f"{d['free_space']} bytes")
            self.lbl_files.setText(str(d['total_files']))
            self.lbl_users.setText(str(d['total_users']))
            
            percent = int((d['used_space'] / d['total_size']) * 100) if d['total_size'] > 0 else 0
            self.bar.setValue(percent)

    def toggle_theme(self):
        self.is_dark = not self.is_dark
        if self.is_dark:
            ThemeManager.set_dark_theme(QApplication.instance())
        else:
            ThemeManager.set_light_theme(QApplication.instance())

    def do_logout(self):
        self.client.send_request("user_logout")
        self.client.session_id = ""
        self.on_logout()

class AppController:
    def __init__(self):
        self.app = QApplication(sys.argv)
        ThemeManager.set_dark_theme(self.app)
        self.client = OFSClient()
        self.show_login()

    def show_login(self):
        self.login_window = LoginWindow(self.client, self.show_main)
        self.login_window.show()
        if hasattr(self, 'main_window'): self.main_window.close()

    def show_main(self):
        self.main_window = MainWindow(self.client, self.show_login)
        self.main_window.show()
        if hasattr(self, 'login_window'): self.login_window.close()

    def run(self):
        sys.exit(self.app.exec())

if __name__ == "__main__":
    controller = AppController()
    controller.run()
# ui/client_ui.py
# Student-style Tkinter UI for OFS (Phase 01 Part C)
# Uses base64 for file uploads (data_base64) and sends JSON over TCP.
# Usage:
#   python3 ui/client_ui.py
#
# Notes:
# - Server must be running on localhost:8080 (or change host/port in UI)
# - Messages are newline-terminated JSON strings
# - Responses are collected line-by-line and shown in Responses tab

import tkinter as tk
from tkinter import ttk, filedialog, scrolledtext, messagebox
import socket
import threading
import json
import time
import base64
import os
import uuid

HOST_DEFAULT = "127.0.0.1"
PORT_DEFAULT = 8080

# -----------------------
# Helper utilities
# -----------------------
def make_request_id(prefix="r"):
    return prefix + "_" + str(int(time.time())) + "_" + uuid.uuid4().hex[:6]

def pretty_json(s):
    try:
        obj = json.loads(s)
        return json.dumps(obj, indent=2)
    except Exception:
        return s

def build_json(cmd, params, session_id=None, request_id=None):
    req = {}
    req["cmd"] = cmd
    if session_id:
        req["session_id"] = session_id
    if request_id:
        req["request_id"] = request_id
    else:
        req["request_id"] = make_request_id(cmd)
    # parameters: merge into top-level or into "parameters"? PDF suggests operation/parameters or cmd with params.
    # We'll put params at top-level (simple student-style)
    for k, v in params.items():
        req[k] = v
    return json.dumps(req)

# -----------------------
# Networking thread safe socket wrapper
# -----------------------
class SocketClient:
    def __init__(self, host, port, on_response):
        self.host = host
        self.port = port
        self.sock = None
        self.on_response = on_response
        self.running = False
        self.lock = threading.Lock()

    def connect(self):
        with self.lock:
            if self.sock:
                return True
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect((self.host, int(self.port)))
                s.settimeout(0.5)
                self.sock = s
                self.running = True
                threading.Thread(target=self.recv_loop, daemon=True).start()
                return True
            except Exception as e:
                self.sock = None
                self.running = False
                raise e

    def close(self):
        with self.lock:
            self.running = False
            if self.sock:
                try:
                    self.sock.shutdown(socket.SHUT_RDWR)
                except:
                    pass
                try:
                    self.sock.close()
                except:
                    pass
                self.sock = None

    def send(self, text):
        with self.lock:
            if not self.sock:
                raise Exception("Not connected")
            # ensure newline termination
            if not text.endswith("\n"):
                text = text + "\n"
            self.sock.sendall(text.encode())

    def recv_loop(self):
        buf = ""
        while self.running:
            try:
                data = self.sock.recv(4096)
                if not data:
                    # socket closed by server
                    self.running = False
                    self.on_response({"_meta":"connection_closed"})
                    break
                try:
                    chunk = data.decode()
                except:
                    chunk = data.decode('latin1')
                buf += chunk
                while "\n" in buf:
                    line, buf = buf.split("\n", 1)
                    line = line.strip()
                    if line == "":
                        continue
                    # call callback on each JSON line
                    try:
                        parsed = json.loads(line)
                    except:
                        parsed = line
                    self.on_response(parsed)
            except socket.timeout:
                continue
            except Exception as e:
                self.running = False
                self.on_response({"_meta":"error", "error": str(e)})
                break

# -----------------------
# Main UI
# -----------------------
class OFSClientUI:
    def __init__(self, root):
        self.root = root
        root.title("OFS Client UI (Student)")
        root.geometry("900x620")

        self.host_var = tk.StringVar(value=HOST_DEFAULT)
        self.port_var = tk.StringVar(value=str(PORT_DEFAULT))
        self.session_id = ""           # stores session after login
        self.client = None

        self._build_top()
        self._build_tabs()
        self._build_responses()

    def _build_top(self):
        topf = tk.Frame(self.root)
        topf.pack(fill=tk.X, padx=8, pady=6)

        tk.Label(topf, text="Host:").pack(side=tk.LEFT)
        tk.Entry(topf, textvariable=self.host_var, width=14).pack(side=tk.LEFT, padx=(0,6))
        tk.Label(topf, text="Port:").pack(side=tk.LEFT)
        tk.Entry(topf, textvariable=self.port_var, width=6).pack(side=tk.LEFT, padx=(0,6))
        self.conn_btn = tk.Button(topf, text="Connect", command=self.connect_clicked)
        self.conn_btn.pack(side=tk.LEFT, padx=(0,6))
        self.disc_btn = tk.Button(topf, text="Disconnect", command=self.disconnect_clicked, state=tk.DISABLED)
        self.disc_btn.pack(side=tk.LEFT, padx=(0,6))
        tk.Button(topf, text="Clear Responses", command=self.clear_responses).pack(side=tk.RIGHT)

    def _build_tabs(self):
        tab_control = ttk.Notebook(self.root)
        tab_control.pack(expand=1, fill="both", padx=8, pady=(0,6))

        # Login tab
        self.tab_login = ttk.Frame(tab_control)
        tab_control.add(self.tab_login, text="Login")
        self._build_tab_login(self.tab_login)

        # File ops tab
        self.tab_file = ttk.Frame(tab_control)
        tab_control.add(self.tab_file, text="File Operations")
        self._build_tab_file(self.tab_file)

        # Dir ops tab
        self.tab_dir = ttk.Frame(tab_control)
        tab_control.add(self.tab_dir, text="Directory Operations")
        self._build_tab_dir(self.tab_dir)

        # System tab
        self.tab_sys = ttk.Frame(tab_control)
        tab_control.add(self.tab_sys, text="System")
        self._build_tab_sys(self.tab_sys)

        # Raw JSON
        self.tab_raw = ttk.Frame(tab_control)
        tab_control.add(self.tab_raw, text="Raw JSON")
        self._build_tab_raw(self.tab_raw)

    def _build_responses(self):
        respf = tk.Frame(self.root)
        respf.pack(fill=tk.BOTH, padx=8, pady=(0,8), expand=True)

        tk.Label(respf, text="Responses & Log:").pack(anchor=tk.W)
        self.resp_area = scrolledtext.ScrolledText(respf, height=12)
        self.resp_area.pack(fill=tk.BOTH, expand=True)

    # -------------------------
    # individual tab builders
    # -------------------------
    def _build_tab_login(self, frame):
        pad = 6
        f = tk.Frame(frame)
        f.pack(padx=8, pady=8, anchor=tk.NW)

        tk.Label(f, text="Username:").grid(row=0, column=0, sticky=tk.W, padx=pad, pady=4)
        self.login_user = tk.Entry(f, width=25)
        self.login_user.grid(row=0, column=1, padx=pad, pady=4)
        tk.Label(f, text="Password:").grid(row=1, column=0, sticky=tk.W, padx=pad, pady=4)
        self.login_pass = tk.Entry(f, width=25, show="*")
        self.login_pass.grid(row=1, column=1, padx=pad, pady=4)

        self.login_msg = tk.Label(f, text="", fg="green")
        self.login_msg.grid(row=2, column=0, columnspan=2, sticky=tk.W, padx=pad, pady=6)

        tk.Button(f, text="Login", command=self.login_clicked).grid(row=0, column=2, padx=pad)
        tk.Button(f, text="Logout", command=self.logout_clicked).grid(row=1, column=2, padx=pad)
        tk.Button(f, text="Who Am I", command=self.whoami_clicked).grid(row=2, column=2, padx=pad)

    def _build_tab_file(self, frame):
        pad = 4
        f = tk.Frame(frame)
        f.pack(fill=tk.X, padx=8, pady=6, anchor=tk.NW)

        # Path
        tk.Label(f, text="Path (e.g. /docs/a.txt):").grid(row=0, column=0, sticky=tk.W, padx=pad, pady=4)
        self.file_path = tk.Entry(f, width=60)
        self.file_path.grid(row=0, column=1, columnspan=3, padx=pad, pady=4)

        # Manual text area
        tk.Label(f, text="Manual data (small):").grid(row=1, column=0, sticky=tk.NW, padx=pad, pady=4)
        self.file_text = scrolledtext.ScrolledText(f, height=6, width=55)
        self.file_text.grid(row=1, column=1, columnspan=2, padx=pad, pady=4)

        # File chooser
        tk.Button(f, text="Load from disk...", command=self.load_file_from_disk).grid(row=1, column=3, padx=pad, pady=4, sticky=tk.N)

        # Actions
        btnf = tk.Frame(frame)
        btnf.pack(padx=8, pady=(0,8), anchor=tk.NW)

        tk.Button(btnf, text="Create File (upload)", command=self.create_file_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Read File", command=self.read_file_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Delete File", command=self.delete_file_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Rename File", command=self.rename_file_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Edit File (overwrite)", command=self.edit_file_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Truncate File", command=self.truncate_file_clicked).pack(side=tk.LEFT, padx=6)

        # rename fields (small)
        rn_frame = tk.Frame(frame)
        rn_frame.pack(padx=8, pady=4, anchor=tk.NW)
        tk.Label(rn_frame, text="New Path for rename:").pack(side=tk.LEFT)
        self.rename_newpath = tk.Entry(rn_frame, width=40)
        self.rename_newpath.pack(side=tk.LEFT, padx=6)

    def _build_tab_dir(self, frame):
        pad = 6
        f = tk.Frame(frame)
        f.pack(padx=8, pady=8, anchor=tk.NW)
        tk.Label(f, text="Directory Path (e.g. /docs):").grid(row=0, column=0, sticky=tk.W, padx=pad, pady=4)
        self.dir_path = tk.Entry(f, width=50)
        self.dir_path.grid(row=0, column=1, padx=pad, pady=4)

        btnf = tk.Frame(frame)
        btnf.pack(padx=8, pady=(0,8), anchor=tk.NW)
        tk.Button(btnf, text="Create Directory", command=self.dir_create_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="List Directory", command=self.dir_list_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Delete Directory", command=self.dir_delete_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(btnf, text="Exists?", command=self.dir_exists_clicked).pack(side=tk.LEFT, padx=6)

    def _build_tab_sys(self, frame):
        pad = 6
        f = tk.Frame(frame)
        f.pack(padx=8, pady=8, anchor=tk.NW)

        tk.Button(f, text="Get Stats", command=self.stats_clicked).pack(side=tk.LEFT, padx=6)
        tk.Button(f, text="Get Free Space", command=self.stats_clicked).pack(side=tk.LEFT, padx=6)  # same for now
        tk.Button(f, text="Exit Server (client send)", command=self.exit_server_clicked).pack(side=tk.LEFT, padx=6)

    def _build_tab_raw(self, frame):
        pad = 6
        tk.Label(frame, text="Raw JSON (edit then Send):").pack(anchor=tk.W, padx=pad, pady=(6,0))
        self.raw_text = scrolledtext.ScrolledText(frame, height=10)
        self.raw_text.pack(fill=tk.BOTH, padx=pad, pady=(0,6))
        self.raw_text.insert(tk.END, '{"cmd":"stats","request_id":"r1"}')
        btnf = tk.Frame(frame)
        btnf.pack(padx=8, pady=(0,8), anchor=tk.NW)
        tk.Button(btnf, text="Send Raw JSON", command=self.send_raw_clicked).pack(side=tk.LEFT, padx=6)

    # -------------------------
    # connection / events
    # -------------------------
    def connect_clicked(self):
        host = self.host_var.get().strip()
        port = self.port_var.get().strip()
        if host == "" or port == "":
            messagebox.showinfo("Info", "Enter host and port")
            return
        try:
            self.client = SocketClient(host, int(port), self.on_response)
            self.client.connect()
            self.log("Connected to %s:%s" % (host, port))
            self.conn_btn.config(state=tk.DISABLED)
            self.disc_btn.config(state=tk.NORMAL)
        except Exception as e:
            messagebox.showerror("Connection error", str(e))
            self.client = None

    def disconnect_clicked(self):
        if self.client:
            self.client.close()
            self.client = None
            self.log("Disconnected")
            self.conn_btn.config(state=tk.NORMAL)
            self.disc_btn.config(state=tk.DISABLED)

    # -------------------------
    # UI actions that send JSON
    # -------------------------
    def login_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Connect to server first")
            return
        username = self.login_user.get().strip()
        password = self.login_pass.get().strip()
        if username == "" or password == "":
            messagebox.showinfo("Info", "Username and password required")
            return
        rid = make_request_id("login")
        js = build_json("login", {"username": username, "password": password}, None, rid)
        try:
            self.client.send(js)
            self.log("SENT login request")
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def logout_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        rid = make_request_id("logout")
        js = build_json("logout", {}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT logout request")
            # drop session locally
            self.session_id = ""
            self.login_msg.config(text="Logged out")
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def whoami_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        rid = make_request_id("whoami")
        js = build_json("whoami", {}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT whoami")
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def stats_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        rid = make_request_id("stats")
        js = build_json("stats", {}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT stats")
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def exit_server_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        rid = make_request_id("exit")
        js = build_json("exit", {}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT exit")
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    # -------------------------
    # File operations
    # -------------------------
    def load_file_from_disk(self):
        path = filedialog.askopenfilename()
        if not path:
            return
        try:
            with open(path, "rb") as f:
                data = f.read()
            # put file content in manual textbox as utf-8 if possible (safe fallback)
            try:
                text = data.decode('utf-8')
                self.file_text.delete("1.0", tk.END)
                self.file_text.insert(tk.END, text)
            except:
                # non-text: show file name only and store base64 in a temp var for upload
                self.file_text.delete("1.0", tk.END)
                self.file_text.insert(tk.END, "[binary file loaded: %s]" % os.path.basename(path))
            # store loaded bytes for create/edit uses
            self._last_loaded_file = data
            self._last_loaded_path = path
            self.log("Loaded file: %s (%d bytes)" % (path, len(data)))
        except Exception as e:
            messagebox.showerror("Load error", str(e))

    def create_file_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.file_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter file path")
            return
        # prefer disk-loaded file if present; else manual text
        data_bytes = None
        if hasattr(self, "_last_loaded_file") and getattr(self, "_last_loaded_path", ""):
            # if loaded file path differs from target path it's fine
            data_bytes = self._last_loaded_file
        else:
            txt = self.file_text.get("1.0", tk.END).encode('utf-8')
            data_bytes = txt
        # base64 encode
        b64 = base64.b64encode(data_bytes).decode('ascii')
        size = len(data_bytes)
        rid = make_request_id("file_create")
        params = {"path": path, "data_base64": b64, "size": size}
        js = build_json("file_create", params, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT file_create %s (%d bytes)" % (path, size))
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def read_file_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.file_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter file path")
            return
        rid = make_request_id("file_read")
        js = build_json("file_read", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT file_read %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def delete_file_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.file_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter file path")
            return
        rid = make_request_id("file_delete")
        js = build_json("file_delete", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT file_delete %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def rename_file_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        old = self.file_path.get().strip()
        newp = self.rename_newpath.get().strip()
        if old == "" or newp == "":
            messagebox.showinfo("Info", "Enter old and new paths")
            return
        rid = make_request_id("file_rename")
        js = build_json("file_rename", {"old_path": old, "new_path": newp}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT file_rename %s -> %s" % (old, newp))
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def edit_file_clicked(self):
        # overwrite with manual or loaded file
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.file_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter file path")
            return
        data_bytes = None
        if hasattr(self, "_last_loaded_file") and getattr(self, "_last_loaded_path", ""):
            data_bytes = self._last_loaded_file
        else:
            data_bytes = self.file_text.get("1.0", tk.END).encode('utf-8')
        b64 = base64.b64encode(data_bytes).decode('ascii')
        size = len(data_bytes)
        # we send file_edit as overwrite with index 0 by default
        rid = make_request_id("file_edit")
        params = {"path": path, "data_base64": b64, "size": size, "index": 0}
        js = build_json("file_edit", params, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT file_edit %s (%d bytes)" % (path, size))
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def truncate_file_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.file_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter file path")
            return
        rid = make_request_id("file_truncate")
        js = build_json("file_truncate", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT file_truncate %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    # -------------------------
    # Directory operations
    # -------------------------
    def dir_create_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.dir_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter dir path")
            return
        rid = make_request_id("dir_create")
        js = build_json("dir_create", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT dir_create %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def dir_list_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.dir_path.get().strip()
        if path == "":
            path = "/"
        rid = make_request_id("dir_list")
        js = build_json("dir_list", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT dir_list %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def dir_delete_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.dir_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter dir path")
            return
        rid = make_request_id("dir_delete")
        js = build_json("dir_delete", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT dir_delete %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    def dir_exists_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Not connected")
            return
        path = self.dir_path.get().strip()
        if path == "":
            messagebox.showinfo("Info", "Enter path")
            return
        rid = make_request_id("dir_exists")
        js = build_json("dir_exists", {"path": path}, self.session_id, rid)
        try:
            self.client.send(js)
            self.log("SENT dir_exists %s" % path)
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    # -------------------------
    # Raw JSON
    # -------------------------
    def send_raw_clicked(self):
        if not self.client:
            messagebox.showinfo("Info", "Connect first")
            return
        text = self.raw_text.get("1.0", tk.END).strip()
        # minimal validation
        try:
            tmp = json.loads(text)
        except Exception as e:
            messagebox.showerror("JSON error", str(e))
            return
        # ensure request_id
        if "request_id" not in tmp:
            tmp["request_id"] = make_request_id("raw")
        # ensure session if present
        if self.session_id and ("session_id" not in tmp):
            tmp["session_id"] = self.session_id
        try:
            self.client.send(json.dumps(tmp))
            self.log("SENT raw json")
        except Exception as e:
            messagebox.showerror("Send error", str(e))

    # -------------------------
    # Response handler
    # -------------------------
    def on_response(self, resp):
        # resp may be a dict or raw string
        if isinstance(resp, dict):
            # check for meta markers
            if "_meta" in resp:
                if resp["_meta"] == "connection_closed":
                    self.log("[notice] server closed connection")
                elif resp["_meta"] == "error":
                    self.log("[socket error] " + str(resp.get("error","")))
                return
            text = json.dumps(resp)
            # auto-capture session id for login success
            try:
                if isinstance(resp, dict) and resp.get("operation","") == "login" and resp.get("status","") == "success":
                    data = resp.get("data", {})
                    sess = data.get("session_id", "")
                    if sess:
                        self.session_id = sess
                        self.login_msg.config(text="Logged in (session: %s)" % sess)
                # if file_read returns base64 data, optionally show decoded text in file_text widget
                if resp.get("operation","") == "file_read" and resp.get("status","") == "success":
                    data = resp.get("data", {})
                    if "data_base64" in data:
                        try:
                            binb = base64.b64decode(data["data_base64"])
                            # try to decode as utf8 for display
                            try:
                                txt = binb.decode('utf-8')
                                self.file_text.delete("1.0", tk.END)
                                self.file_text.insert(tk.END, txt)
                                self.log("file_read: decoded into file text box")
                            except:
                                # binary, show info only
                                self.log("file_read: received binary (%d bytes)" % len(binb))
                        except Exception:
                            self.log("file_read: cannot decode base64")
                # show error_code if present (map is in enum in C++, so just show numeric)
                if "error_code" in resp:
                    self.log("[ERROR_CODE] %s - %s" % (str(resp.get("error_code")), resp.get("error_message","")))
            except Exception:
                pass
            self.log(pretty_json(text))
        else:
            self.log(str(resp))

    # -------------------------
    # logging helper
    # -------------------------
    def log(self, text):
        t = time.strftime("%H:%M:%S")
        self.resp_area.insert(tk.END, "[%s] %s\n" % (t, text))
        self.resp_area.see(tk.END)

    def clear_responses(self):
        self.resp_area.delete("1.0", tk.END)

# -----------------------
# Main
# -----------------------
def main():
    root = tk.Tk()
    app = OFSClientUI(root)
    # connect handler wiring for SocketClient uses the on_response method
    OFSClientUI.on_response = app.on_response
    root.mainloop()

if __name__ == "__main__":
    main()

function serverUrl() {
  return document.getElementById("serverUrl").value;
}

function makeRequestId(prefix) {
  return prefix + "_" + Date.now();
}

function logResponse(txt) {
  const el = document.getElementById("responses");
  el.textContent = (txt + "\n\n") + el.textContent;
}

async function sendJson(obj) {
  try {
    const res = await fetch(serverUrl(), {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(obj)
    });
    const text = await res.text();
    try {
      const parsed = JSON.parse(text);
      logResponse(JSON.stringify(parsed, null, 2));
      return parsed;
    } catch (e) {
      logResponse(text);
      return null;
    }
  } catch (err) {
    logResponse("Network error: " + err.message);
    return null;
  }
}

// tabs
document.querySelectorAll("#tabs button").forEach(btn => {
  btn.addEventListener("click", () => {
    document.querySelectorAll("#tabs button").forEach(b=>b.classList.remove("active"));
    btn.classList.add("active");
    const tab = btn.getAttribute("data-tab");
    document.querySelectorAll("main .tab").forEach(s=>s.classList.remove("active"));
    document.getElementById(tab).classList.add("active");
  });
});

// connect button
document.getElementById("connectBtn").addEventListener("click", async () => {
  logResponse("[info] Using " + serverUrl());
  const r = await sendJson({ cmd: "stats", request_id: makeRequestId("ping") });
  if (r) logResponse("[info] ping OK");
});

// login
document.getElementById("loginBtn").addEventListener("click", async () => {
  const u = document.getElementById("username").value;
  const p = document.getElementById("password").value;
  const rid = makeRequestId("login");
  const obj = { cmd:"login", username: u, password: p, request_id: rid };
  const r = await sendJson(obj);
  if (r && r.status === "success" && r.data && r.data.session_id) {
    document.getElementById("loginStatus").textContent = "Logged in: " + r.data.session_id;
    window.OFS_SESSION = r.data.session_id;
  }
});

// dir create/list/delete
document.getElementById("createDirBtn").addEventListener("click", async () => {
  const path = document.getElementById("dirPath").value || "/";
  const obj = { cmd:"dir_create", path: path, request_id: makeRequestId("dir_create"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});
document.getElementById("listDirBtn").addEventListener("click", async () => {
  const path = document.getElementById("dirPath").value || "/";
  const obj = { cmd:"dir_list", path: path, request_id: makeRequestId("dir_list"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});
document.getElementById("deleteDirBtn").addEventListener("click", async () => {
  const path = document.getElementById("dirPath").value || "/";
  const obj = { cmd:"dir_delete", path: path, request_id: makeRequestId("dir_delete"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});

// stats / whoami
document.getElementById("statsBtn").addEventListener("click", async () => {
  const obj = { cmd:"stats", request_id: makeRequestId("stats"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});
document.getElementById("whoBtn").addEventListener("click", async () => {
  const obj = { cmd:"whoami", request_id: makeRequestId("whoami"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});

// raw send
document.getElementById("sendRawBtn").addEventListener("click", async () => {
  let txt = document.getElementById("rawJson").value;
  try {
    const obj = JSON.parse(txt);
    if (!obj.request_id) obj.request_id = makeRequestId("raw");
    if (!obj.cmd && obj.operation) obj.cmd = obj.operation;
    if (!obj.session_id && window.OFS_SESSION) obj.session_id = window.OFS_SESSION;
    await sendJson(obj);
  } catch (e) {
    logResponse("Invalid JSON in raw box");
  }
});

// file create/upload/read/delete/rename
document.getElementById("createFileBtn").addEventListener("click", async () => {
  const path = document.getElementById("filePath").value;
  const fileInput = document.getElementById("fileInput");
  if (fileInput.files && fileInput.files[0]) {
    const file = fileInput.files[0];
    const reader = new FileReader();
    reader.onload = async function(e) {
      const s = e.target.result;
      const parts = s.split(",");
      const base64 = parts.length>1 ? parts[1] : parts[0];
      const obj = { cmd:"file_create", path: path, data_base64: base64, size: file.size, request_id: makeRequestId("file_create"), session_id: window.OFS_SESSION || "" };
      await sendJson(obj);
    };
    reader.readAsDataURL(file);
  } else {
    const txt = document.getElementById("fileText").value || "";
    const base64 = btoa(unescape(encodeURIComponent(txt)));
    const obj = { cmd:"file_create", path: path, data_base64: base64, size: base64.length, request_id: makeRequestId("file_create"), session_id: window.OFS_SESSION || "" };
    await sendJson(obj);
  }
});

document.getElementById("readFileBtn").addEventListener("click", async () => {
  const path = document.getElementById("filePath").value;
  const obj = { cmd:"file_read", path: path, request_id: makeRequestId("file_read"), session_id: window.OFS_SESSION || "" };
  const r = await sendJson(obj);
  if (r && r.status === "success" && r.data && r.data.data_base64) {
    try {
      const bin = atob(r.data.data_base64);
      const decoder = new TextDecoder("utf-8");
      let arr = new Uint8Array(bin.length);
      for (let i=0;i<bin.length;i++) arr[i] = bin.charCodeAt(i);
      const text = decoder.decode(arr);
      document.getElementById("fileText").value = text;
    } catch (e) {
      logResponse("Binary file received (cannot display)");
    }
  }
});

document.getElementById("deleteFileBtn").addEventListener("click", async () => {
  const path = document.getElementById("filePath").value;
  const obj = { cmd:"file_delete", path: path, request_id: makeRequestId("file_delete"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});

document.getElementById("renameBtn").addEventListener("click", async () => {
  const oldp = document.getElementById("filePath").value;
  const newp = document.getElementById("newPath").value;
  const obj = { cmd:"file_rename", old_path: oldp, new_path: newp, request_id: makeRequestId("file_rename"), session_id: window.OFS_SESSION || "" };
  await sendJson(obj);
});

document.getElementById("loadFileBtn").addEventListener("click", () => {
  const fileInput = document.getElementById("fileInput");
  if (!fileInput.files || !fileInput.files[0]) { logResponse("Choose file first"); return; }
  const reader = new FileReader();
  reader.onload = function(e) {
    const s = e.target.result;
    if (s.startsWith("data:")) {
      const parts = s.split(",");
      const base64 = parts.length>1 ? parts[1] : parts[0];
      try {
        const txt = decodeURIComponent(escape(atob(base64)));
        document.getElementById("fileText").value = txt;
      } catch (err) {
        document.getElementById("fileText").value = "[binary file loaded]";
      }
    } else {
      document.getElementById("fileText").value = s;
    }
  };
  reader.readAsDataURL(fileInput.files[0]);
});
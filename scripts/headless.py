"""Headless launch settings shared by every gate and probe (AGENTS.md rule 13).

No automated or investigative run may show a window, take focus, change the
display mode or touch the desktop gamma:

- FnQL clients start with CLIENT (`+set com_headless 2`): a windowless
  client that still renders, so engine screenshots and captures are real.
- FnQL dedicated servers start with SERVER (`+set com_headless 1`), the
  dedicated default, stated explicitly so a client binary passed as the
  engine stays windowless too.
- Every process starts with `popen_options()`: no console window and, on
  Windows, a hidden show state.
- The retail Steam client is retail code launched by Steam, so it cannot be
  asked to stay windowless (a direct start cannot initialize Steam, and on a
  private desktop its renderer stalls). `RetailWindowHider` hides each of its
  windows on the show event and keeps them hidden, but a window can still be
  on screen for an instant, so gates that launch it run only when
  `retail_client_allowed()` (FNQL_ALLOW_RETAIL_CLIENT=1).
"""
from __future__ import annotations

import os
import subprocess
import sys
import threading
from typing import Any

CLIENT = ["+set", "com_headless", "2"]
SERVER = ["+set", "com_headless", "1"]
CLIENT_SETTINGS = {"com_headless": "2"}
SERVER_SETTINGS = {"com_headless": "1"}


def retail_client_allowed() -> bool:
    """Whether a run may launch the retail Steam client (explicit opt-in)."""
    return os.environ.get("FNQL_ALLOW_RETAIL_CLIENT") == "1"


def popen_options() -> dict[str, Any]:
    """Keyword arguments for subprocess.Popen that never show a window."""
    if sys.platform != "win32":
        return {}
    startup = subprocess.STARTUPINFO()
    startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0  # SW_HIDE
    return {"startupinfo": startup, "creationflags": subprocess.CREATE_NO_WINDOW}


class RetailWindowHider:
    """Hide every window owned by a process with the given image name."""

    def __init__(self, image: str, interval: float = 0.01) -> None:
        self._image = image.lower()
        self._interval = interval
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self.hidden = 0

    def __enter__(self) -> "RetailWindowHider":
        if sys.platform == "win32":
            self._thread = threading.Thread(target=self._run, daemon=True)
            self._thread.start()
        return self

    def __exit__(self, *exc: object) -> None:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=2)

    def _run(self) -> None:
        import ctypes
        from ctypes import wintypes

        user32 = ctypes.WinDLL("user32", use_last_error=True)
        callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        user32.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
        user32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        user32.IsWindowVisible.argtypes = [wintypes.HWND]
        user32.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.OpenProcess.restype = wintypes.HANDLE
        kernel32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        kernel32.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD,
                                                        wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
        kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
        names: dict[int, bool] = {}

        def matches(pid: int) -> bool:
            if pid not in names:
                handle = kernel32.OpenProcess(0x1000, False, pid)  # QUERY_LIMITED_INFORMATION
                path = ""
                if handle:
                    buffer = ctypes.create_unicode_buffer(1024)
                    size = wintypes.DWORD(len(buffer))
                    if kernel32.QueryFullProcessImageNameW(handle, 0, buffer, ctypes.byref(size)):
                        path = buffer.value
                    kernel32.CloseHandle(handle)
                names[pid] = path.replace("/", "\\").rsplit("\\", 1)[-1].lower() == self._image
            return names[pid]

        def hide_if_ours(hwnd) -> None:
            pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            if user32.IsWindowVisible(hwnd) and matches(pid.value):
                user32.ShowWindow(hwnd, 0)  # SW_HIDE
                self.hidden += 1

        @callback_type
        def sweep(hwnd, _):
            hide_if_ours(hwnd)
            return True

        # Hide on the show event itself, so a retail window is on screen for
        # as little as one event dispatch; the sweep below catches anything
        # shown before the hook existed.
        event_type = ctypes.WINFUNCTYPE(None, wintypes.HANDLE, wintypes.DWORD, wintypes.HWND,
                                        wintypes.LONG, wintypes.LONG, wintypes.DWORD, wintypes.DWORD)

        @event_type
        def on_show(_hook, _event, hwnd, id_object, _child, _thread, _time):
            if hwnd and id_object == 0:  # OBJID_WINDOW
                hide_if_ours(hwnd)

        user32.SetWinEventHook.restype = wintypes.HANDLE
        user32.SetWinEventHook.argtypes = [wintypes.DWORD, wintypes.DWORD, wintypes.HMODULE, event_type,
                                           wintypes.DWORD, wintypes.DWORD, wintypes.DWORD]
        user32.UnhookWinEvent.argtypes = [wintypes.HANDLE]
        user32.PeekMessageW.argtypes = [ctypes.POINTER(wintypes.MSG), wintypes.HWND, wintypes.UINT,
                                        wintypes.UINT, wintypes.UINT]
        user32.MsgWaitForMultipleObjects.argtypes = [wintypes.DWORD, ctypes.c_void_p, wintypes.BOOL,
                                                     wintypes.DWORD, wintypes.DWORD]
        # EVENT_OBJECT_SHOW, out of context, skipping our own process.
        hook = user32.SetWinEventHook(0x8002, 0x8002, None, on_show, 0, 0, 0x0000 | 0x0002)
        message = wintypes.MSG()
        try:
            while not self._stop.is_set():
                user32.EnumWindows(sweep, 0)
                user32.MsgWaitForMultipleObjects(0, None, False, int(self._interval * 1000), 0x04FF)  # QS_ALLINPUT
                while user32.PeekMessageW(ctypes.byref(message), None, 0, 0, 1):  # PM_REMOVE
                    user32.TranslateMessage(ctypes.byref(message))
                    user32.DispatchMessageW(ctypes.byref(message))
        finally:
            if hook:
                user32.UnhookWinEvent(hook)

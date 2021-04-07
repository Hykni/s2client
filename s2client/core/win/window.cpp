#include "window.hpp"

#include <core/io/logger.hpp>

namespace core {
	map<window::HandleType, window&> window::HandleToWindow;

	LRESULT CALLBACK window::DispatchWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
		if (Msg == WM_CREATE || Msg == WM_NCCREATE)
			HandleToWindow.emplace(hWnd, *(window*)((CREATESTRUCT*)lParam)->lpCreateParams);
		
		auto it = HandleToWindow.find(hWnd);
		if (it != HandleToWindow.end())
			return it->second.wndproc(hWnd, Msg, wParam, lParam);
		else {
			//assert(false);
			core::warning("Unhandled window message %Xh (%Xh, %Xh)\n", Msg, wParam, lParam);
			return DefWindowProcA(hWnd, Msg, wParam, lParam);
		}
	}

	LRESULT window::wndproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
		if (Msg == WM_NCHITTEST)
			return HTCAPTION;
		return DefWindowProcA(hWnd, Msg, wParam, lParam);
	}

	bool window::create() {
#if defined(_WIN64) || defined(_WIN32)
		WNDCLASSA wndClass = { 0 };
		constexpr auto classname = "core::window";
		wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wndClass.lpszClassName = classname;
		wndClass.lpfnWndProc = &window::DispatchWndProc;
		RegisterClassA(&wndClass);
		mHandle = CreateWindowExA(WS_EX_APPWINDOW, classname, "window",
			WS_POPUPWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			mWidth, mHeight, mParent, NULL, (HINSTANCE)GetModuleHandle(NULL), this);
		if (!mHandle) {
			core::error("Window creation failed (%Xh).", GetLastError());
		}
#endif
		return mHandle != NULL;
	}
	int window::width() const {
		return mWidth;
	}
	int window::height() const {
		return mHeight;
	}
	window::window(int width, int height, HandleType parent)
		: mWidth(width), mHeight(height), mHandle(0), mParent(parent) {
	}
	void window::pump() {
		MSG msg = { 0 };
		if (PeekMessageA(&msg, mHandle, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}
	void window::show(bool en) {
		ShowWindow(mHandle, en ? SW_SHOW : SW_HIDE);
		UpdateWindow(mHandle);
	}
	window::HandleType window::handle() const {
		return mHandle;
	}
}

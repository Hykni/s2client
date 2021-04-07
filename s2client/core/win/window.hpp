#pragma once

#include <core/prerequisites.hpp>

namespace core {
	class window {
		using HandleType = HWND;
		int mWidth, mHeight;
		HandleType mHandle;
		HandleType mParent;

		static map<HandleType, window&> HandleToWindow;
		static LRESULT CALLBACK DispatchWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
		LRESULT wndproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	public:
		window(int width, int height, HandleType parent=0);
		bool create();

		int width()const;
		int height()const;
		
		void pump();
		void show(bool en = true);
		HandleType handle()const;
	};
}

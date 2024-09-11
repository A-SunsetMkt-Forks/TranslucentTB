#include "xamlthread.hpp"
#include <wil/resource.h>

#include "../windows/window.hpp"
#include "uwp.hpp"
#include "../../ProgramLog/error/win32.hpp"

DWORD WINAPI XamlThread::ThreadProc(LPVOID param)
{
	const auto that = static_cast<XamlThread *>(param);
	that->ThreadInit();
	that->m_Ready.SetEvent();

	BOOL ret;
	MSG msg;
	while ((ret = GetMessage(&msg, Window::NullWindow, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			if (!that->PreTranslateMessage(msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			LastErrorHandle(spdlog::level::critical, L"Failed to get message");
		}
	}

	delete that;
	winrt::uninit_apartment();
	return static_cast<DWORD>(msg.wParam);
}

void XamlThread::DeletedCallback(void *data)
{
	const auto that = static_cast<XamlThread *>(data);

	{
		std::scoped_lock guard(that->m_CurrentWindowLock);
		that->m_CurrentWindow.reset();
	}

	that->m_Source = nullptr;
}

void XamlThread::ThreadInit()
{
	try
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}
	HresultErrorCatch(spdlog::level::critical, L"Failed to initialize thread apartment");

	m_Manager = UWP::CreateXamlManager();
	m_Dispatcher = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
}

bool XamlThread::PreTranslateMessage(const MSG &msg)
{
	// prevent XAML islands from capturing ALT-{F4,SPACE} because of
	// https://github.com/microsoft/microsoft-ui-xaml/issues/2408
	if (msg.message == WM_SYSKEYDOWN && (msg.wParam == VK_F4 || msg.wParam == VK_SPACE)) [[unlikely]]
	{
		Window(msg.hwnd).ancestor(GA_ROOT).send_message(msg.message, msg.wParam, msg.lParam);
		return true;
	}

	if (m_Source)
	{
		BOOL result;
		const HRESULT hr = m_Source->PreTranslateMessage(&msg, &result);
		if (SUCCEEDED(hr))
		{
			if (result)
			{
				return result;
			}
		}
		else
		{
			HresultHandle(hr, spdlog::level::warn, L"Failed to pre-translate message");
		}
	}

	return false;
}

winrt::fire_and_forget XamlThread::ThreadDeinit()
{
	co_await wil::resume_foreground(m_Dispatcher, winrt::Windows::System::DispatcherQueuePriority::Low);

	// only called during destruction of thread pool, so no locking needed.
	m_CurrentWindow.reset();

	m_Source = nullptr;
	m_Manager.Close();
	m_Manager = nullptr;

	PostQuitMessage(0);
}

XamlThread::XamlThread() :
	m_Dispatcher(nullptr),
	m_Manager(nullptr)
{
	m_Thread.reset(CreateThread(nullptr, 0, XamlThread::ThreadProc, this, 0, nullptr));
	if (!m_Thread)
	{
		LastErrorHandle(spdlog::level::critical, L"Failed to create XAML thread");
	}

#ifdef _DEBUG
	HresultVerify(SetThreadDescription(m_Thread.get(), APP_NAME L" XAML Island Thread"), spdlog::level::info, L"Failed to set thread description");
#endif

	m_Ready.wait();
}

wil::unique_handle XamlThread::Delete()
{
	ThreadDeinit();
	return std::move(m_Thread);
}

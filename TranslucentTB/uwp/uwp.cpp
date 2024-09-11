#include "uwp.hpp"
#include <Windows.h>
#include <appmodel.h>
#include <CoreWindow.h>
#include <ShlObj_core.h>
#include <wil/resource.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Core.h>

#include "../ProgramLog/error/win32.hpp"
#include "../ProgramLog/error/winrt.hpp"

std::optional<std::wstring> UWP::GetPackageFamilyName()
{
	static constexpr std::wstring_view FAILED_TO_GET = L"Failed to get package family name";

	UINT32 length = 0;
	LONG result = GetCurrentPackageFamilyName(&length, nullptr);
	if (result != ERROR_INSUFFICIENT_BUFFER) [[unlikely]]
	{
		if (result == APPMODEL_ERROR_NO_PACKAGE)
		{
			return std::nullopt;
		}
		else
		{
			HresultHandle(HRESULT_FROM_WIN32(result), spdlog::level::critical, FAILED_TO_GET);
		}
	}

	std::wstring familyName;
	familyName.resize_and_overwrite(length - 1, [](wchar_t* data, std::size_t count)
	{
		UINT32 length = static_cast<UINT32>(count) + 1;
		const LONG result = GetCurrentPackageFamilyName(&length, data);
		if (result != ERROR_SUCCESS) [[unlikely]]
		{
			HresultHandle(HRESULT_FROM_WIN32(result), spdlog::level::critical, FAILED_TO_GET);
		}

		return length - 1;
	});

	return familyName;
}

std::optional<std::filesystem::path> UWP::GetAppStorageFolder()
{
	if (const auto familyName = UWP::GetPackageFamilyName())
	{
		wil::unique_cotaskmem_string appdata;
		HresultVerify(
			SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_NO_PACKAGE_REDIRECTION, nullptr, appdata.put()),
			spdlog::level::critical,
			L"Failed to get local app data folder"
		);

		std::filesystem::path storage = appdata.get();
		storage /= L"Packages";
		storage /= *familyName;
		return std::move(storage);
	}
	else
	{
		return std::nullopt;
	}
}

winrt::fire_and_forget UWP::OpenUri(const wf::Uri &uri)
{
	bool opened;
	try
	{
		opened = co_await winrt::Windows::System::Launcher::LaunchUriAsync(uri);
	}
	catch (const winrt::hresult_error &err)
	{
		HresultErrorHandle(err, spdlog::level::err, L"Failed to launch uri.");
		co_return;
	}

	if (!opened)
	{
		MessagePrint(spdlog::level::err, L"Uri was not launched.");
	}
}

Window UWP::GetCoreWindow()
{
	Window coreWin;
	try
	{
		const auto coreInterop = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().as<ICoreWindowInterop>();
		winrt::check_hresult(coreInterop->get_WindowHandle(coreWin.put()));
	}
	HresultErrorCatch(spdlog::level::warn, L"Failed to get core window handle");

	return coreWin;
}

void UWP::HideCoreWindow()
{
	if (auto coreWin = GetCoreWindow())
	{
		coreWin.show(SW_HIDE);
	}
}

wuxh::WindowsXamlManager UWP::CreateXamlManager() try
{
	const auto manager = wuxh::WindowsXamlManager::InitializeForCurrentThread();
	HideCoreWindow();
	return manager;
}
HresultErrorCatch(spdlog::level::critical, L"Failed to create Xaml manager");

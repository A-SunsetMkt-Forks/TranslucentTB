#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include "winrt.hpp"
#include <winrt/Windows.System.h>
#include "undefgetcurrenttime.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "redefgetcurrenttime.h"

#include "../windows/window.hpp"

namespace UWP {
	std::optional<std::wstring> GetPackageFamilyName();
	std::optional<std::filesystem::path> GetAppStorageFolder();
	winrt::fire_and_forget OpenUri(const wf::Uri &uri);
	Window GetCoreWindow();
	void HideCoreWindow();
	wuxh::WindowsXamlManager CreateXamlManager();
};

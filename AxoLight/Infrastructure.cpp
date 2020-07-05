#include "pch.h"
#include "Infrastructure.h"

using namespace std;
using namespace winrt;

namespace AxoLight::Infrastructure
{
  std::filesystem::path get_root()
  {
    wchar_t executablePath[MAX_PATH];
    GetModuleFileNameW(NULL, executablePath, MAX_PATH);

    return filesystem::path(executablePath).remove_filename();
  }

  std::vector<uint8_t> load_file(const std::filesystem::path& path)
  {
    FILE* file = nullptr;
    _wfopen_s(&file, path.c_str(), L"rb");
    fseek(file, 0, SEEK_END);
    auto length = ftell(file);

    std::vector<uint8_t> buffer(length);

    fseek(file, 0, SEEK_SET);
    fread_s(buffer.data(), length, length, 1, file);
    fclose(file);

    return buffer;
  }

  LRESULT CALLBACK debug_message_handler(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
  {
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(windowHandle, message, wParam, lParam);
        break;
    }

    return 0;
  }

  handle create_debug_window()
  {
    handle windowCreatedEvent = handle(CreateEvent(0, false, false, nullptr));

    auto hEvent = windowCreatedEvent.get();
    handle hWnd;
    thread([&hWnd, hEvent] {
      auto hInstance = GetModuleHandle(NULL);

      WNDCLASSEXW classDescription = { 0 };
      classDescription.cbSize = sizeof(WNDCLASSEX);
      classDescription.hInstance = hInstance;
      classDescription.lpfnWndProc = debug_message_handler;
      classDescription.lpszClassName = L"DebugWindow";
      classDescription.style = CS_HREDRAW | CS_VREDRAW;
      classDescription.hCursor = LoadCursor(NULL, IDC_ARROW);
      classDescription.hbrBackground = (HBRUSH)COLOR_WINDOW;

      RegisterClassExW(&classDescription);

      hWnd = handle(CreateWindowExW(NULL, L"DebugWindow", L"Debug Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 300, 300, NULL, NULL, hInstance, nullptr));

      ShowWindow((HWND)hWnd.get(), SW_SHOW);
      SetEvent(hEvent);

      MSG wMessage;

      while (GetMessage(&wMessage, NULL, 0, 0))
      {
          TranslateMessage(&wMessage);
          DispatchMessage(&wMessage);
      }
    }).detach();

    WaitForSingleObject(hEvent, INFINITE);
    return hWnd;
  }
}
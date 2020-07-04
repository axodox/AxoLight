#include "pch.h"
#include "AdaLightController.h"

using namespace std::chrono_literals;
using namespace AxoLight::Lighting;

using namespace winrt;
using namespace Windows::Foundation;

int main()
{
  init_apartment();
  Uri uri(L"http://aka.ms/cppwinrt");
  printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

  AdaLightController controller;
  if (!controller.IsConnected()) return 0;

  while (true)
  {
    controller.Push({
      {255, 0, 0},
      {0, 255, 0},
      {0, 0, 255}
      });

  }

  return 0;
}

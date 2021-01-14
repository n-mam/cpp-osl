#include <osl.hpp>

int main(void)
{
  auto list = EnumerateVolumes();

  for(auto& names : list)
  {
    for(auto& name : names)
    {
      std::cout << name << "\n";
    }
    std::cout << "\n";
  }

  Json json;

  OSL::GetVolumeList(json);

  std::cout << json.Stringify() << "\n";

  return 0;
}
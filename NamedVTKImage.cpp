#include "NamedVTKImage.h"

#include "Helpers.h"

NamedVTKImage FindImageByName(const std::vector<NamedVTKImage>& namedImages, const std::string& imageName)
{
  for(unsigned int i = 0; i < namedImages.size(); ++i)
    {
    if(Helpers::StringsMatch(namedImages[i].Name, imageName))
      {
      return namedImages[i];
      }
    }
  std::cerr << "No image named " << imageName << " found!" << std::endl;
  exit(-1);
}
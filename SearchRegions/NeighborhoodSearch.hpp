/*=========================================================================
 *
 *  Copyright David Doria 2012 daviddoria@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef NeighborhoodSearch_HPP
#define NeighborhoodSearch_HPP

// Custom
#include "Helpers/Helpers.h"

// ITK
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkImage.h"
/**

 */
template <typename TVertexDescriptorType>
struct NeighborhoodSearch
{
  itk::ImageRegion<2> FullRegion;

  typedef std::vector<TVertexDescriptorType> VectorType;

  unsigned int Radius;
  
  NeighborhoodSearch(const itk::ImageRegion<2>& fullRegion, const unsigned int radius) : FullRegion(fullRegion), Radius(radius)
  {

  };

  VectorType operator()(const TVertexDescriptorType& center)
  {
    itk::Index<2> centerIndex = Helpers::ConvertFrom<itk::Index<2>, TVertexDescriptorType>(center);
    
    itk::Index<2> corner = centerIndex;
    corner[0] -= Radius;
    corner[1] -= Radius;

    itk::Size<2> size = {{ Radius*2 + 1, Radius*2 + 1 }};
    itk::ImageRegion<2> region(corner, size);
    region.Crop(FullRegion);
    
    // We have to create a dummy image with the specified region in order to use the region iterator
    typedef itk::Image<int, 2> ImageType;
    ImageType::Pointer image = ImageType::New();
    image->SetRegions(region);
    image->Allocate();

    VectorType vertices(region.GetNumberOfPixels());
    unsigned int counter = 0;
    itk::ImageRegionConstIteratorWithIndex<ImageType> imageIterator(image, region);
    while(!imageIterator.IsAtEnd())
      {
      TVertexDescriptorType vert = Helpers::ConvertFrom<TVertexDescriptorType, itk::Index<2> > (imageIterator.GetIndex());
      vertices[counter] = vert;
      counter++;
      ++imageIterator;
      }
    return vertices;
  }

};

#endif

/*=========================================================================
 *
 *  Copyright David Doria 2011 daviddoria@gmail.com
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

#include "ClusterColors.h"

// ITK
#include "itkImageRegionConstIterator.h"
#include "itkVector.h"

ClusterColors::ClusterColors()
{
  this->MaxIterations = 10;
  // this->ColorBinMembershipImage = IntImageType::New(); // This is done in CreateMembershipImage()
}

void ClusterColors::SetMaxIterations(const unsigned int maxIterations)
{
  this->MaxIterations = maxIterations;
}

std::vector<ColorMeasurementVectorType> ClusterColors::GetColors()
{
  return this->Colors;
}

void ClusterColors::Construct(const FloatVectorImageType::Pointer image)
{
  this->Image = image;
  GenerateColors();
  
  CreateMembershipImage();
}

IntImageType::Pointer ClusterColors::GetColorBinMembershipImage()
{
  return this->ColorBinMembershipImage;
}


std::vector<float> ClusterColors::HistogramRegion(const FloatVectorImageType::Pointer image, const itk::ImageRegion<2>& imageRegion,
                                                  Mask::Pointer mask, const itk::ImageRegion<2>& maskRegion)
{
  std::vector<float> histogram(this->Colors.size(), 0.0f);

  itk::ImageRegionConstIterator<FloatVectorImageType> imageIterator(image, imageRegion);
  itk::ImageRegionConstIterator<Mask> maskIterator(mask, maskRegion);
    
  while(!imageIterator.IsAtEnd())
    {
    if(mask->IsHole(maskIterator.GetIndex()))
      {
      ++imageIterator;
      ++maskIterator;
      continue;
      }
    FloatVectorImageType::PixelType pixel = imageIterator.Get();
    ColorMeasurementVectorType measurement;
    measurement[0] = pixel[0];
    measurement[1] = pixel[1];
    measurement[2] = pixel[2];
    
    TreeType::InstanceIdentifierVectorType neighbors;
    this->KDTree->Search( measurement, 1u, neighbors );
  
    histogram[neighbors[0]] += 1.0f;
    
    ++imageIterator;
    ++maskIterator;
    }
    
  return histogram;
}


std::vector<float> ClusterColors::HistogramRegion(const IntImageType::Pointer image, const itk::ImageRegion<2>& imageRegion,
                                                  const Mask::Pointer mask, const itk::ImageRegion<2>& maskRegion)
{
  EnterFunction("ClusterColors::HistogramRegion(IntImageType)");
  std::vector<float> histogram(this->Colors.size(), 0.0f);
  //std::cout << "histogram.size() " << histogram.size() << std::endl;
  itk::ImageRegionConstIterator<IntImageType> imageIterator(image, imageRegion);
  itk::ImageRegionConstIterator<Mask> maskIterator(mask, maskRegion);
    
  while(!imageIterator.IsAtEnd())
    {
    if(mask->IsHole(maskIterator.GetIndex()))
      {
      ++imageIterator;
      ++maskIterator;
      continue;
      }
    //std::cout << "Attempting to increment bin " << imageIterator.Get() << std::endl;
    histogram[imageIterator.Get()] += 1.0f;
    
    ++imageIterator;
    ++maskIterator;
    }
  LeaveFunction("ClusterColors::HistogramRegion(IntImageType)");
  return histogram;
}

void ClusterColors::CreateKDTreeFromColors()
{
  EnterFunction("CreateSamplesFromColors");
  this->Sample = SampleType::New();
  this->Sample->SetMeasurementVectorSize( ColorMeasurementVectorType::Dimension );

  for(unsigned int i = 0; i < this->Colors.size(); ++i)
    {
    this->Sample->PushBack(this->Colors[i]);
    }

  // Create a KDTree

  this->TreeGenerator = TreeGeneratorType::New();
  this->TreeGenerator->SetSample( this->Sample );
  this->TreeGenerator->SetBucketSize( 16 );
  this->TreeGenerator->Update();

  typedef TreeType::NearestNeighbors NeighborsType;
  typedef TreeType::KdTreeNodeType NodeType;

  this->KDTree = this->TreeGenerator->GetOutput();
  LeaveFunction("CreateSamplesFromColors");
}

void ClusterColors::CreateMembershipImage()
{
  EnterFunction("CreateMembershipImage");
  this->ColorBinMembershipImage = IntImageType::New();
  this->ColorBinMembershipImage->SetRegions(this->Image->GetLargestPossibleRegion());
  this->ColorBinMembershipImage->Allocate();
  this->ColorBinMembershipImage->FillBuffer(0);
  
  ColorMeasurementVectorType queryPoint;
  itk::ImageRegionConstIterator<FloatVectorImageType> imageIterator(this->Image, this->Image->GetLargestPossibleRegion());

  while(!imageIterator.IsAtEnd())
    {
    // Get the value of the current pixel
    FloatVectorImageType::PixelType pixel = imageIterator.Get();

    queryPoint[0] = pixel[0];
    queryPoint[1] = pixel[1];
    queryPoint[2] = pixel[2];

    TreeType::InstanceIdentifierVectorType neighbors;
    this->KDTree->Search( queryPoint, 1u, neighbors );
    
    this->ColorBinMembershipImage->SetPixel(imageIterator.GetIndex(), neighbors[0]);
    ++imageIterator;
    }
  LeaveFunction("CreateMembershipImage");
}

ClusterColors::TreeType::Pointer ClusterColors::GetKDTree()
{
  return this->KDTree;
}

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

#include "itkCastImageFilter.h"

// STL
#include <iomanip> // for setfill()

// VTK
#include <vtkImageData.h>

// ITK
#include "itkGaussianOperator.h"
#include "itkImageFileWriter.h"
#include "itkVectorMagnitudeImageFilter.h"

// Qt
#include <QColor>

// Custom
#include "Mask.h"

namespace Helpers
{

/** Copy the input to the output*/
template<typename TImage>
void DeepCopy(const typename TImage::Pointer input, typename TImage::Pointer output)
{
  output->SetRegions(input->GetLargestPossibleRegion());
  output->Allocate();

  itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
  itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());

  while(!inputIterator.IsAtEnd())
    {
    outputIterator.Set(inputIterator.Get());
    ++inputIterator;
    ++outputIterator;
    }
}

/** Copy the input to the output*/
template<typename TImage>
void DeepCopyVectorImage(const typename TImage::Pointer input, typename TImage::Pointer output)
{
  output->SetRegions(input->GetLargestPossibleRegion());
  output->SetNumberOfComponentsPerPixel(input->GetNumberOfComponentsPerPixel());
  output->Allocate();

  itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
  itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());

  while(!inputIterator.IsAtEnd())
    {
    outputIterator.Set(inputIterator.Get());
    ++inputIterator;
    ++outputIterator;
    }
    
}

// template <typename TPixelType>
// float PixelSquaredDifference(const TPixelType& pixel1, const TPixelType& pixel2)
// {
//   
// //   std::cout << "pixel1: " << pixel1 << " pixel2: " << pixel2
// //             << " pixel1-pixel2: " << pixel1-pixel2
// //             << " squared norm: " << (pixel1-pixel2).GetSquaredNorm() << std::endl;
//   
//   //return (pixel1-pixel2).GetSquaredNorm();
//   
//   float difference = 0;
//   unsigned int componentsPerPixel = pixel1.GetSize();
//   for(unsigned int i = 0; i < componentsPerPixel; ++i)
//     {
//     difference += (pixel1[i] - pixel2[i]) * 
// 		  (pixel1[i] - pixel2[i]);
//     }
//   return difference;
// }

template<typename T>
void ReplaceValue(typename T::Pointer image, const typename T::PixelType queryValue, const typename T::PixelType replacementValue)
{
  // This function replaces all pixels in 'image' equal to 'queryValue' with 'replacementValue'
  itk::ImageRegionIterator<T> imageIterator(image, image->GetLargestPossibleRegion());
  imageIterator.GoToBegin();
  while(!imageIterator.IsAtEnd())
    {
    if(imageIterator.Get() == queryValue)
      {
      imageIterator.Set(replacementValue);
      }
    ++imageIterator;
    }
}

template<typename T>
void WriteImage(const typename T::Pointer image, const std::string& filename)
{
  // This is a convenience function so that images can be written in 1 line instead of 4.
  typename itk::ImageFileWriter<T>::Pointer writer = itk::ImageFileWriter<T>::New();
  writer->SetFileName(filename);
  writer->SetInput(image);
  writer->Update();
}


template<typename T>
void WriteRGBImage(const typename T::Pointer input, const std::string& filename)
{
  typedef itk::Image<itk::CovariantVector<unsigned char, 3>, 2> RGBImageType;

  RGBImageType::Pointer output = RGBImageType::New();
  output->SetRegions(input->GetLargestPossibleRegion());
  output->Allocate();

  itk::ImageRegionConstIterator<T> inputIterator(input, input->GetLargestPossibleRegion());
  itk::ImageRegionIterator<RGBImageType> outputIterator(output, output->GetLargestPossibleRegion());

  while(!inputIterator.IsAtEnd())
    {
    itk::CovariantVector<unsigned char, 3> pixel;
    for(unsigned int i = 0; i < 3; ++i)
      {
      pixel[i] = inputIterator.Get()[i];
      }
    outputIterator.Set(pixel);
    ++inputIterator;
    ++outputIterator;
    }

  typename itk::ImageFileWriter<RGBImageType>::Pointer writer = itk::ImageFileWriter<RGBImageType>::New();
  writer->SetFileName(filename);
  writer->SetInput(output);
  writer->Update();

}

template <class T>
void CreateBlankPatch(typename T::Pointer patch, const unsigned int radius)
{
  CreateConstantPatch<T>(patch, itk::NumericTraits< typename T::PixelType >::Zero, radius);
}

template <class T>
void CreateConstantPatch(typename T::Pointer patch, const typename T::PixelType value, const unsigned int radius)
{
  try
  {
  typename T::IndexType start;
  start.Fill(0);

  typename T::SizeType size;
  size.Fill(radius*2 + 1);

  typename T::RegionType region(start, size);

  patch->SetRegions(region);
  patch->Allocate();

  itk::ImageRegionIterator<T> imageIterator(patch, patch->GetLargestPossibleRegion());

  while(!imageIterator.IsAtEnd())
    {
    imageIterator.Set(value);
    ++imageIterator;
    }

  }
  catch( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught in CreateConstantPatch!" << std::endl;
    std::cerr << err << std::endl;
    exit(-1);
  }
}

template <class T>
float MaxValue(const typename T::Pointer image)
{
  typedef typename itk::MinimumMaximumImageCalculator<T>
          ImageCalculatorFilterType;

  typename ImageCalculatorFilterType::Pointer imageCalculatorFilter
          = ImageCalculatorFilterType::New ();
  imageCalculatorFilter->SetImage(image);
  imageCalculatorFilter->Compute();

  return imageCalculatorFilter->GetMaximum();
}

template <class T>
float MaxValueLocation(const typename T::Pointer image)
{
  typedef typename itk::MinimumMaximumImageCalculator<T>
          ImageCalculatorFilterType;

  typename ImageCalculatorFilterType::Pointer imageCalculatorFilter
          = ImageCalculatorFilterType::New ();
  imageCalculatorFilter->SetImage(image);
  imageCalculatorFilter->Compute();

  return imageCalculatorFilter->GetIndexOfMaximum();
}

template <class T>
float MinValue(const typename T::Pointer image)
{
  typedef typename itk::MinimumMaximumImageCalculator<T>
          ImageCalculatorFilterType;

  typename ImageCalculatorFilterType::Pointer imageCalculatorFilter
          = ImageCalculatorFilterType::New ();
  imageCalculatorFilter->SetImage(image);
  imageCalculatorFilter->Compute();

  return imageCalculatorFilter->GetMinimum();
}

template <class T>
itk::Index<2> MinValueLocation(const typename T::Pointer image)
{
  typedef typename itk::MinimumMaximumImageCalculator<T>
          ImageCalculatorFilterType;

  typename ImageCalculatorFilterType::Pointer imageCalculatorFilter
          = ImageCalculatorFilterType::New ();
  imageCalculatorFilter->SetImage(image);
  imageCalculatorFilter->Compute();

  return imageCalculatorFilter->GetIndexOfMinimum();
}

template <class T>
void CopyPatchIntoImage(const typename T::Pointer patch, typename T::Pointer image, const Mask::Pointer mask, const itk::Index<2>& position)
{
  try
  {
  // This function copies 'patch' into 'image' centered at 'position' only where the 'mask' is non-zero

  // 'Mask' must be the same size as 'image'
  if(mask->GetLargestPossibleRegion().GetSize() != image->GetLargestPossibleRegion().GetSize())
    {
    std::cerr << "mask and image must be the same size!" << std::endl;
    exit(-1);
    }

  // The PasteFilter expects the lower left corner of the destination position, but we have passed the center pixel.
  position[0] -= patch->GetLargestPossibleRegion().GetSize()[0]/2;
  position[1] -= patch->GetLargestPossibleRegion().GetSize()[1]/2;

  itk::ImageRegion<2> region = GetRegionInRadiusAroundPixel(position, patch->GetLargestPossibleRegion().GetSize()[0]/2);

  itk::ImageRegionConstIterator<T> patchIterator(patch,patch->GetLargestPossibleRegion());
  itk::ImageRegionConstIterator<Mask> maskIterator(mask,region);
  itk::ImageRegionIterator<T> imageIterator(image, region);

  while(!patchIterator.IsAtEnd())
    {
    if(mask->IsHole(maskIterator.GetIndex())) // we are in the target region
      {
      imageIterator.Set(patchIterator.Get());
      }
    ++imageIterator;
    ++maskIterator;
    ++patchIterator;
    }

  }
  catch( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught in CopyPatchIntoImage(patch, image, mask, position)!" << std::endl;
    std::cerr << err << std::endl;
    exit(-1);
  }
}



template <class T>
void CopySelfPatchIntoValidRegion(typename T::Pointer image, const Mask::Pointer mask,
                                  const itk::ImageRegion<2>& sourceRegionInput, const itk::ImageRegion<2>& destinationRegionInput)
{
  try
  {
    // We pass the regions by const reference, so copy them here before they are mutated
    itk::ImageRegion<2> sourceRegion = sourceRegionInput;
    itk::ImageRegion<2> destinationRegion = destinationRegionInput;
    
    // Move the source region to the desintation region
    itk::Offset<2> offset = destinationRegion.GetIndex() - sourceRegion.GetIndex();
    sourceRegion.SetIndex(sourceRegion.GetIndex() + offset);

    // Make the destination be entirely inside the image
    destinationRegion.Crop(image->GetLargestPossibleRegion());
    sourceRegion.Crop(image->GetLargestPossibleRegion());

    // Move the source region back
    sourceRegion.SetIndex(sourceRegion.GetIndex() - offset);

    itk::ImageRegionConstIterator<T> sourceIterator(image, sourceRegion);
    itk::ImageRegionIterator<T> destinationIterator(image, destinationRegion);
    itk::ImageRegionConstIterator<Mask> maskIterator(mask, destinationRegion);

    while(!sourceIterator.IsAtEnd())
      {
      if(mask->IsHole(maskIterator.GetIndex())) // we are in the target region
	{
	destinationIterator.Set(sourceIterator.Get());
	}
      ++sourceIterator;
      ++maskIterator;
      ++destinationIterator;
      }

  }
  catch( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught in CopySelfPatchIntoValidRegion!" << std::endl;
    std::cerr << err << std::endl;
    exit(-1);
  }
}

template <class T>
void CopyPatchIntoImage(const typename T::Pointer patch, typename T::Pointer image, const itk::Index<2>& centerPixel)
{
  try
  {
    // This function copies 'patch' into 'image' centered at 'position'.

    // The PasteFilter expects the lower left corner of the destination position, but we have passed the center pixel.
    itk::Index<2> cornerPixel;
    cornerPixel[0] = centerPixel[0] - patch->GetLargestPossibleRegion().GetSize()[0]/2;
    cornerPixel[1] = centerPixel[1] - patch->GetLargestPossibleRegion().GetSize()[1]/2;

    typedef itk::PasteImageFilter <T, T> PasteImageFilterType;

    typename PasteImageFilterType::Pointer pasteFilter = PasteImageFilterType::New();
    pasteFilter->SetInput(0, image);
    pasteFilter->SetInput(1, patch);
    pasteFilter->SetSourceRegion(patch->GetLargestPossibleRegion());
    pasteFilter->SetDestinationIndex(cornerPixel);
    pasteFilter->InPlaceOn();
    pasteFilter->Update();

    image->Graft(pasteFilter->GetOutput());

  }
  catch( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught in CopyPatchIntoImage(patch, image, centerPixel)!" << std::endl;
    std::cerr << err << std::endl;
    exit(-1);
  }
}

template <class T>
void CopyPatch(const typename T::Pointer sourceImage, typename T::Pointer targetImage,
               const itk::Index<2>& sourcePosition, const itk::Index<2>& targetPosition, const unsigned int radius)
{
  try
  {
  // Copy a patch of radius 'radius' centered at 'sourcePosition' from 'sourceImage' to 'targetImage' centered at 'targetPosition'
  typedef itk::RegionOfInterestImageFilter<T,T> ExtractFilterType;

  typename ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
  extractFilter->SetRegionOfInterest(GetRegionInRadiusAroundPixel(sourcePosition, radius));
  extractFilter->SetInput(sourceImage);
  extractFilter->Update();

  CopyPatchIntoImage<T>(extractFilter->GetOutput(), targetImage, targetPosition);
  }
  catch( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught in CopyPatch!" << std::endl;
    std::cerr << err << std::endl;
    exit(-1);
  }
}


template <class T>
void WriteScaledScalarImage(const typename T::Pointer image, const std::string& filename)
{
  if(T::PixelType::Dimension > 1)
    {
    std::cerr << "Cannot write scaled scalar image with vector image input!" << std::endl;
    return;
    }
  typedef itk::RescaleIntensityImageFilter<T, UnsignedCharScalarImageType> RescaleFilterType; // expected ';' before rescaleFilter

  typename RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
  rescaleFilter->SetInput(image);
  rescaleFilter->SetOutputMinimum(0);
  rescaleFilter->SetOutputMaximum(255);
  rescaleFilter->Update();

  typedef itk::ImageFileWriter<UnsignedCharScalarImageType> WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(filename);
  writer->SetInput(rescaleFilter->GetOutput());
  writer->Update();
}


template <typename TImage>
void ColorToGrayscale(const typename TImage::Pointer colorImage, UnsignedCharScalarImageType::Pointer grayscaleImage)
{
  grayscaleImage->SetRegions(colorImage->GetLargestPossibleRegion());
  grayscaleImage->Allocate();

  itk::ImageRegionConstIterator<TImage> colorImageIterator(colorImage, colorImage->GetLargestPossibleRegion());
  itk::ImageRegionIterator<UnsignedCharScalarImageType> grayscaleImageIterator(grayscaleImage, grayscaleImage->GetLargestPossibleRegion());

  typename TImage::PixelType largestPixel;
  largestPixel.Fill(255);

  float largestNorm = largestPixel.GetNorm();

  while(!colorImageIterator.IsAtEnd())
    {
    grayscaleImageIterator.Set(colorImageIterator.Get().GetNorm()*(255./largestNorm));

    ++colorImageIterator;
    ++grayscaleImageIterator;
    }
}

template <typename TImageType>
void DebugWriteSequentialImage(const typename TImageType::Pointer image, const std::string& filePrefix, const unsigned int iteration)
{
  std::stringstream padded;
  padded << "Debug/" << filePrefix << "_" << std::setfill('0') << std::setw(4) << iteration << ".mha";
  Helpers::WriteImage<TImageType>(image, padded.str());
}

template <typename TImageType>
void DebugWriteImageConditional(const typename TImageType::Pointer image, const std::string& fileName, const bool condition)
{
  if(condition)
    {
    WriteImage<TImageType>(image, fileName);
    }
}


template <typename TImage>
void ITKScalarImageToScaledVTKImage(const typename TImage::Pointer image, vtkImageData* outputImage)
{
  //std::cout << "ITKScalarImagetoVTKImage()" << std::endl;
  
  // Rescale and cast for display
  typedef itk::RescaleIntensityImageFilter<TImage, UnsignedCharScalarImageType > RescaleFilterType;
  typename RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
  rescaleFilter->SetOutputMinimum(0);
  rescaleFilter->SetOutputMaximum(255);
  rescaleFilter->SetInput(image);
  rescaleFilter->Update();

  // Setup and allocate the VTK image
  outputImage->SetNumberOfScalarComponents(1);
  outputImage->SetScalarTypeToUnsignedChar();
  outputImage->SetDimensions(image->GetLargestPossibleRegion().GetSize()[0],
                             image->GetLargestPossibleRegion().GetSize()[1],
                             1);

  outputImage->AllocateScalars();

  // Copy all of the scaled magnitudes to the output image
  itk::ImageRegionConstIteratorWithIndex<UnsignedCharScalarImageType> imageIterator(rescaleFilter->GetOutput(), rescaleFilter->GetOutput()->GetLargestPossibleRegion());
  imageIterator.GoToBegin();

  while(!imageIterator.IsAtEnd())
    {
    unsigned char* pixel = static_cast<unsigned char*>(outputImage->GetScalarPointer(imageIterator.GetIndex()[0],
                                                                                     imageIterator.GetIndex()[1],0));
    pixel[0] = imageIterator.Get();

    ++imageIterator;
    }
    
  outputImage->Modified();
}

template<typename TImage>
void SetRegionToConstant(typename TImage::Pointer image, const itk::ImageRegion<2>& region, const typename TImage::PixelType& value)
{
  typename itk::ImageRegionIterator<TImage> imageIterator(image, region);

  while(!imageIterator.IsAtEnd())
    {
    imageIterator.Set(value);

    ++imageIterator;
    }
}

template<typename TImage>
unsigned int CountNonZeroPixels(const typename TImage::Pointer image)
{
  typename itk::ImageRegionIterator<TImage> imageIterator(image, image->GetLargestPossibleRegion());

  unsigned int numberOfNonZeroPixels = 0;
  while(!imageIterator.IsAtEnd())
    {
    if(imageIterator.Get())
      {
      numberOfNonZeroPixels++;
      }

    ++imageIterator;
    }
  return numberOfNonZeroPixels;
}

template<typename TImage>
std::vector<itk::Index<2> > GetNonZeroPixels(const typename TImage::Pointer image)
{
  return GetNonZeroPixels(image, image->GetLargestPossibleRegion());
}

template<typename TImage>
std::vector<itk::Index<2> > GetNonZeroPixels(const typename TImage::Pointer image, const itk::ImageRegion<2>& region)
{
  std::vector<itk::Index<2> > nonZeroPixels;
  
  typename itk::ImageRegionIterator<TImage> imageIterator(image, region);

  while(!imageIterator.IsAtEnd())
    {
    if(imageIterator.Get())
      {
      nonZeroPixels.push_back(imageIterator.GetIndex());
      }

    ++imageIterator;
    }
  return nonZeroPixels;
}


template <class T>
unsigned int argmin(const typename std::vector<T>& vec)
{
  T minValue = std::numeric_limits<T>::max();
  unsigned int minLocation = 0;
  for(unsigned int i = 0; i < vec.size(); ++i)
    {
    if(vec[i] < minValue)
      {
      minValue = vec[i];
      minLocation = i;
      }
    }
    
  return minLocation;
}

template<typename TImage>
void WriteRegion(const typename TImage::Pointer image, const itk::ImageRegion<2>& region, const std::string& filename)
{
  std::cout << "WriteRegion() " << filename << std::endl;
  std::cout << "region " << region << std::endl;
  typedef itk::RegionOfInterestImageFilter<TImage, TImage> RegionOfInterestImageFilterType;

  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();

  std::cout << "regionOfInterestImageFilter " << regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion() << std::endl;
  
  typename itk::ImageFileWriter<TImage>::Pointer writer = itk::ImageFileWriter<TImage>::New();
  writer->SetFileName(filename);
  writer->SetInput(regionOfInterestImageFilter->GetOutput());
  writer->Update();
}

template<typename TImage>
void WriteRegionAsImage(const typename TImage::Pointer image, const itk::ImageRegion<2>& region, const std::string& filename)
{
  // This function varies from WriteRegion() in that the Origin of the output image is (0,0).
  // Because of this, the region cannot be overlayed on the original image, but can be easily compared to other regions.
  //std::cout << "WriteRegion() " << filename << std::endl;
  //std::cout << "region " << region << std::endl;
  
  typedef itk::RegionOfInterestImageFilter<TImage, TImage> RegionOfInterestImageFilterType;

  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();
  
  itk::Point<float, 2> origin;
  origin.Fill(0);
  regionOfInterestImageFilter->GetOutput()->SetOrigin(origin);

  //std::cout << "regionOfInterestImageFilter " << regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion() << std::endl;
  
  typename itk::ImageFileWriter<TImage>::Pointer writer = itk::ImageFileWriter<TImage>::New();
  writer->SetFileName(filename);
  writer->SetInput(regionOfInterestImageFilter->GetOutput());
  writer->Update();
}



template<typename TImage>
void WriteRegionUnsignedChar(const typename TImage::Pointer image, const itk::ImageRegion<2>& region, const std::string& filename)
{
  // The file that is output has Origin = (0,0) because of how VectorImageToRGBImage copies the image.
  
  std::cout << "WriteRegionUnsignedChar() " << filename << std::endl;
  typedef itk::RegionOfInterestImageFilter<TImage, TImage> RegionOfInterestImageFilterType;

  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();

  std::cout << "regionOfInterestImageFilter " << regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion() << std::endl;
  
  RGBImageType::Pointer rgbImage = RGBImageType::New();
  VectorImageToRGBImage(regionOfInterestImageFilter->GetOutput(), rgbImage);
  
  std::cout << "rgbImage " << rgbImage->GetLargestPossibleRegion() << std::endl;
  
  typename itk::ImageFileWriter<RGBImageType>::Pointer writer = itk::ImageFileWriter<RGBImageType>::New();
  writer->SetFileName(filename);
  writer->SetInput(rgbImage);
  writer->Update();
}

template<typename TImage>
void WriteMaskedPatch(const typename TImage::Pointer image, const Mask::Pointer mask, const Patch& patch, const std::string& filename)
{
  WriteMaskedRegion<TImage>(image, mask, patch.Region, filename);
}

template<typename TImage>
void WriteMaskedRegion(const typename TImage::Pointer image, const Mask::Pointer mask, const itk::ImageRegion<2>& region, const std::string& filename)
{
  typedef itk::RegionOfInterestImageFilter<TImage, TImage> RegionOfInterestImageFilterType;
  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();

  typedef itk::RegionOfInterestImageFilter<Mask, Mask> RegionOfInterestMaskFilterType;
  typename RegionOfInterestMaskFilterType::Pointer regionOfInterestMaskFilter = RegionOfInterestMaskFilterType::New();
  regionOfInterestMaskFilter->SetRegionOfInterest(region);
  regionOfInterestMaskFilter->SetInput(mask);
  regionOfInterestMaskFilter->Update();

  itk::ImageRegionIterator<TImage> imageIterator(regionOfInterestImageFilter->GetOutput(), regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion());

  typename TImage::PixelType greenPixel(3);
  greenPixel[0] = 0;
  greenPixel[1] = 255;
  greenPixel[0] = 0;
  
  while(!imageIterator.IsAtEnd())
    {
    typename TImage::PixelType pixel = imageIterator.Get();

    itk::Index<2> index = imageIterator.GetIndex();

    if(regionOfInterestMaskFilter->GetOutput()->IsHole(imageIterator.GetIndex()))
      {
      regionOfInterestImageFilter->GetOutput()->SetPixel(index, greenPixel);
      }

    ++imageIterator;
    }

  typename itk::ImageFileWriter<TImage>::Pointer writer = itk::ImageFileWriter<TImage>::New();
  writer->SetFileName(filename);
  writer->SetInput(regionOfInterestImageFilter->GetOutput());
  writer->Update();
}

template<typename TImage>
void WritePatch(const typename TImage::Pointer image, const Patch& patch, const std::string& filename)
{
  WriteRegion<TImage>(image, patch.Region, filename);
}


template<typename TImage>
void BlankAndOutlineRegion(typename TImage::Pointer image, const itk::ImageRegion<2>& region, const typename TImage::PixelType& blankValue, const typename TImage::PixelType& outlineValue)
{
  SetRegionToConstant<TImage>(image, region, blankValue);
  OutlineRegion<TImage>(image, region, outlineValue);

}

template<typename TImage>
void OutlineRegion(typename TImage::Pointer image, const itk::ImageRegion<2>& region, const typename TImage::PixelType& value)
{
  itk::ImageRegionIterator<TImage> iterator(image, region);

  for(unsigned int i = region.GetIndex()[0]; i < region.GetIndex()[0] + region.GetSize()[0]; ++i)
    {
    itk::Index<2> index;
    index[0] = i;
    index[1] = region.GetIndex()[1];
    image->SetPixel(index, value);

    index[0] = i;
    index[1] = region.GetIndex()[1] + region.GetSize()[1] - 1;
    image->SetPixel(index, value);
    }

  for(unsigned int j = region.GetIndex()[1]; j < region.GetIndex()[1] + region.GetSize()[1]; ++j)
    {
    itk::Index<2> index;
    index[0] = region.GetIndex()[0];
    index[1] = j;
    image->SetPixel(index, value);

    index[0] = region.GetIndex()[0] + region.GetSize()[0] - 1;
    index[1] = j;
    image->SetPixel(index, value);
    }
}

template <typename TImage>
QImage GetQImageColor(const typename TImage::Pointer image, const itk::ImageRegion<2>& region)
{
  QImage qimage(region.GetSize()[0], region.GetSize()[1], QImage::Format_RGB888);

  typedef itk::RegionOfInterestImageFilter< TImage, TImage > RegionOfInterestImageFilterType;
  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();
  
  itk::ImageRegionIterator<TImage> imageIterator(regionOfInterestImageFilter->GetOutput(), regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion());
  
  while(!imageIterator.IsAtEnd())
    {
    typename TImage::PixelType pixel = imageIterator.Get();

    itk::Index<2> index = imageIterator.GetIndex();
    QColor pixelColor(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]), static_cast<int>(pixel[2]));
    qimage.setPixel(index[0], index[1], pixelColor.rgb());

    ++imageIterator;
    }

  QColor highlightColor(255, 0, 255);
  qimage.setPixel(region.GetSize()[0]/2, region.GetSize()[1]/2, highlightColor.rgb());
  
  //return qimage; // The actual image region
  return qimage.mirrored(false, true); // The flipped image region
}

template <typename TImage>
QImage GetQImageMagnitude(const typename TImage::Pointer image, const itk::ImageRegion<2>& region)
{
  QImage qimage(region.GetSize()[0], region.GetSize()[1], QImage::Format_RGB888);

  typedef itk::VectorMagnitudeImageFilter<TImage, FloatScalarImageType>  VectorMagnitudeFilterType;
  typename VectorMagnitudeFilterType::Pointer magnitudeFilter = VectorMagnitudeFilterType::New();
  magnitudeFilter->SetInput(image);
  magnitudeFilter->Update();

  typedef itk::RescaleIntensityImageFilter<FloatScalarImageType, UnsignedCharScalarImageType> RescaleFilterType;
  RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
  rescaleFilter->SetOutputMinimum(0);
  rescaleFilter->SetOutputMaximum(255);
  rescaleFilter->SetInput( magnitudeFilter->GetOutput() );
  rescaleFilter->Update();
  
  typedef itk::RegionOfInterestImageFilter< UnsignedCharScalarImageType, UnsignedCharScalarImageType> RegionOfInterestImageFilterType;
  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(rescaleFilter->GetOutput());
  regionOfInterestImageFilter->Update();

  itk::ImageRegionIterator<TImage> imageIterator(regionOfInterestImageFilter->GetOutput(), regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion());

  while(!imageIterator.IsAtEnd())
    {
    unsigned char pixelValue = imageIterator.Get();

    QColor pixelColor(static_cast<int>(pixelValue), static_cast<int>(pixelValue), static_cast<int>(pixelValue));

    itk::Index<2> index = imageIterator.GetIndex();
    qimage.setPixel(index[0], index[1], pixelColor.rgb());

    ++imageIterator;
    }

  //return qimage; // The actual image region
  return qimage.mirrored(false, true); // The flipped image region
}


template <typename TImage>
QImage GetQImageScalar(const typename TImage::Pointer image, const itk::ImageRegion<2>& region)
{
  QImage qimage(region.GetSize()[0], region.GetSize()[1], QImage::Format_RGB888);

  typedef itk::RegionOfInterestImageFilter< TImage, TImage> RegionOfInterestImageFilterType;
  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();

  itk::ImageRegionIterator<TImage> imageIterator(regionOfInterestImageFilter->GetOutput(), regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion());

  while(!imageIterator.IsAtEnd())
    {
    typename TImage::PixelType pixelValue = imageIterator.Get();

    QColor pixelColor(static_cast<int>(pixelValue), static_cast<int>(pixelValue), static_cast<int>(pixelValue));

    itk::Index<2> index = imageIterator.GetIndex();
    qimage.setPixel(index[0], index[1], pixelColor.rgb());

    ++imageIterator;
    }

  //return qimage; // The actual image region
  return qimage.mirrored(false, true); // The flipped image region
}

template <typename TImage>
QImage GetQImageMasked(const typename TImage::Pointer image, const Mask::Pointer mask, const itk::ImageRegion<2>& region)
{
  QImage qimage(region.GetSize()[0], region.GetSize()[1], QImage::Format_RGB888);

  typedef itk::RegionOfInterestImageFilter< TImage, TImage > RegionOfInterestImageFilterType;
  typename RegionOfInterestImageFilterType::Pointer regionOfInterestImageFilter = RegionOfInterestImageFilterType::New();
  regionOfInterestImageFilter->SetRegionOfInterest(region);
  regionOfInterestImageFilter->SetInput(image);
  regionOfInterestImageFilter->Update();

  typedef itk::RegionOfInterestImageFilter< Mask, Mask> RegionOfInterestMaskFilterType;
  typename RegionOfInterestMaskFilterType::Pointer regionOfInterestMaskFilter = RegionOfInterestMaskFilterType::New();
  regionOfInterestMaskFilter->SetRegionOfInterest(region);
  regionOfInterestMaskFilter->SetInput(mask);
  regionOfInterestMaskFilter->Update();
  
  itk::ImageRegionIterator<TImage> imageIterator(regionOfInterestImageFilter->GetOutput(), regionOfInterestImageFilter->GetOutput()->GetLargestPossibleRegion());

  QColor holeColor(0, 255, 0);
  
  while(!imageIterator.IsAtEnd())
    {
    typename TImage::PixelType pixel = imageIterator.Get();

    itk::Index<2> index = imageIterator.GetIndex();

    if(regionOfInterestMaskFilter->GetOutput()->IsHole(index))
      {
      qimage.setPixel(index[0], index[1], holeColor.rgb());
      }
    else
      {
      QColor pixelColor(static_cast<int>(pixel[0]), static_cast<int>(pixel[1]), static_cast<int>(pixel[2]));
      qimage.setPixel(index[0], index[1], pixelColor.rgb());
      }

    ++imageIterator;
    }

  QColor highlightColor(255, 0, 255);
  qimage.setPixel(region.GetSize()[0]/2, region.GetSize()[1]/2, highlightColor.rgb());
  
  //return qimage; // The actual image region
  return qimage.mirrored(false, true); // The flipped image region
}

// This struct is used inside MaskedBlur()
struct Contribution
{
  float weight;
  unsigned char value;
  itk::Offset<2> offset;
};

template <typename TImage>
void MaskedBlur(const typename TImage::Pointer inputImage, const Mask::Pointer mask, const float blurVariance, typename TImage::Pointer output)
{
  // Create a Gaussian kernel
  typedef itk::GaussianOperator<float, 1> GaussianOperatorType;
  
  // Make a (2*kernelRadius+1)x1 kernel
  itk::Size<1> radius;
  radius.Fill(20); // Make a length 41 kernel
  
  GaussianOperatorType gaussianOperator;
  gaussianOperator.SetDirection(0); // It doesn't matter which direction we set - we will be interpreting the kernel as 1D (no direction)
  gaussianOperator.SetVariance(blurVariance);
  gaussianOperator.CreateToRadius(radius);

//   {
//   // Debugging only
//   std::cout << "gaussianOperator: " << gaussianOperator << std::endl;
//   for(unsigned int i = 0; i < gaussianOperator.Size(); i++)
//     {
//     //std::cout << i << " : " << gaussianOperator.GetOffset(i) << std::endl;
//     std::cout << i << " : " << gaussianOperator.GetElement(i) << std::endl;
//     }
//   }
  
  // Create the output image - data will be deep copied into it
  typename TImage::Pointer blurredImage = TImage::New();
  InitializeImage<TImage>(blurredImage, inputImage->GetLargestPossibleRegion());
  
  // Initialize
  typename TImage::Pointer operatingImage = TImage::New();
  DeepCopy<TImage>(inputImage, operatingImage);
  
  for(unsigned int dimensionPass = 0; dimensionPass < 2; dimensionPass++) // The image is 2D
    {
    itk::ImageRegionIterator<TImage> imageIterator(operatingImage, operatingImage->GetLargestPossibleRegion());
  
    while(!imageIterator.IsAtEnd())
      {
      itk::Index<2> centerPixel = imageIterator.GetIndex();
    
      // We should not compute derivatives for pixels in the hole.
      if(mask->IsHole(centerPixel))
	{
	++imageIterator;
	continue;
	}
	
      // Loop over all of the pixels in the kernel and use the ones that fit a criteria
      std::vector<Contribution> contributions;
      for(unsigned int i = 0; i < gaussianOperator.Size(); i++)
	{
	// Since we use 1D kernels, we must manually construct a 2D offset with 0 in all dimensions except the dimension of the current pass
	itk::Offset<2> offset = OffsetFrom1DOffset(gaussianOperator.GetOffset(i), dimensionPass);
      
	itk::Index<2> pixel = centerPixel + offset;
	if(blurredImage->GetLargestPossibleRegion().IsInside(pixel) && mask->IsValid(pixel))
	  {
	  Contribution contribution;
	  contribution.weight = gaussianOperator.GetElement(i);
	  contribution.value = operatingImage->GetPixel(pixel);
	  contribution.offset = OffsetFrom1DOffset(gaussianOperator.GetOffset(i), dimensionPass);
	  contributions.push_back(contribution);
	  }
	}
	
      float total = 0.0f;
      for(unsigned int i = 0; i < contributions.size(); i++)
	{
	total += contributions[i].weight;
	}
	
      // Determine the new pixel value
      float newPixelValue = 0.0f;
      for(unsigned int i = 0; i < contributions.size(); i++)
	{
	itk::Index<2> pixel = centerPixel + contributions[i].offset;
	newPixelValue += contributions[i].weight/total * operatingImage->GetPixel(pixel);
	}
	
      blurredImage->SetPixel(centerPixel, newPixelValue);
      ++imageIterator;
      }

    // For the separable Gaussian filtering concept to work, the next pass must operate on the output of the current pass.
    DeepCopy<TImage>(blurredImage, operatingImage);
    }
  
  // Copy the final image to the output.
  DeepCopy<TImage>(blurredImage, output);
}

template<typename TPixel>
void GradientFromDerivatives(const typename itk::Image<TPixel, 2>::Pointer xDerivative, const typename itk::Image<TPixel, 2>::Pointer yDerivative, typename itk::Image<itk::CovariantVector<TPixel, 2> >::Pointer output)
{
  if(xDerivative->GetLargestPossibleRegion() != yDerivative->GetLargestPossibleRegion())
    {
    std::cerr << "X and Y derivative images must be the same size!" << std::endl;
    return;
    }
    
  output->SetRegions(xDerivative->GetLargestPossibleRegion());
  output->Allocate();
  
  itk::ImageRegionIterator<itk::Image<itk::CovariantVector<TPixel, 2> > > imageIterator(output, output->GetLargestPossibleRegion());
 
  while(!imageIterator.IsAtEnd())
    {
    itk::CovariantVector<TPixel, 2> vectorPixel;
    vectorPixel[0] = xDerivative->GetPixel(imageIterator.GetIndex());
    vectorPixel[1] = yDerivative->GetPixel(imageIterator.GetIndex());
  
    output->SetPixel(imageIterator.GetIndex(), vectorPixel);
 
    ++imageIterator;
    }
}

template <typename TImage>
void MaskedDerivative(const typename TImage::Pointer image, const Mask::Pointer mask, const unsigned int direction, FloatScalarImageType::Pointer output)
{
  // Setup the output
  output->SetRegions(image->GetLargestPossibleRegion());
  output->Allocate();
  output->FillBuffer(0);
  
  itk::ImageRegionIterator<TImage> imageIterator(image, image->GetLargestPossibleRegion());
 
  while(!imageIterator.IsAtEnd())
    {
    // We should not compute derivatives for pixels in the hole.
    if(mask->IsHole(imageIterator.GetIndex()))
      {
      ++imageIterator;
      continue;
      }

    // Determine which neighbors are valid
    bool backwardValid = false;
    itk::Index<2> backwardIndex = imageIterator.GetIndex();
    backwardIndex[direction]--;
    if(image->GetLargestPossibleRegion().IsInside(backwardIndex) && mask->IsValid(backwardIndex))
      {
      backwardValid = true;
      }
      
    bool forwardValid = false;
    itk::Index<2> forwardIndex = imageIterator.GetIndex();
    forwardIndex[direction]++;
    if(image->GetLargestPossibleRegion().IsInside(forwardIndex) && mask->IsValid(forwardIndex))
      {
      forwardValid = true;
      }
      
    // Compute the correct difference
    float difference = 0.0f;
    
    if(backwardValid && !forwardValid) // Use backwards half difference
      {
      difference = image->GetPixel(imageIterator.GetIndex()) - image->GetPixel(backwardIndex);
      }
    else if(!backwardValid && forwardValid) // Use forwards half difference
      {
      difference = image->GetPixel(forwardIndex) - image->GetPixel(imageIterator.GetIndex());
      }
    else if(backwardValid && forwardValid) // Use full difference
      {
      difference = (image->GetPixel(forwardIndex) - image->GetPixel(backwardIndex))/2.0f;
      }
    else// if(!backwardValid && !forwardValid) // No valid neighbors in this direction
      {
      difference = 0.0f; // There is nothing we can do here, so set the derivative to zero.
      }
      
    output->SetPixel(imageIterator.GetIndex(), difference);

    ++imageIterator;
    }
}


template <typename TImage>
void MaskedDerivativePrewitt(const typename TImage::Pointer image, const Mask::Pointer mask, const unsigned int direction, FloatScalarImageType::Pointer output)
{
  if(direction > 1)
    {
    std::cerr << "This function can only compute derivatives of 2D images!" << std::endl;
    exit(-1);
    }
    
  // Setup the output
  InitializeImage<FloatScalarImageType>(output, image->GetLargestPossibleRegion());

  itk::ImageRegionIterator<TImage> imageIterator(image, image->GetLargestPossibleRegion());

  // If we are taking x derivatives, we want to use 3 columns. If we are taking y derivatives, we want to use 3 rows.
  unsigned int shiftIndex;
  if(direction == 0)
    {
    shiftIndex = 1;
    }
  else
    {
    shiftIndex = 0;
    }

  while(!imageIterator.IsAtEnd())
    {
    // We should not compute derivatives for pixels in the hole.
    if(mask->IsHole(imageIterator.GetIndex()))
      {
      ++imageIterator;
      continue;
      }

    float difference = 0.0f;
    unsigned int numberUsed = 0;
    for(int shift = -1; shift <= 1; shift++) // this shift is either rows or columns, depending on the derivative direction
      {
      itk::Index<2> centerIndex;
      centerIndex[direction] = imageIterator.GetIndex()[direction];
      centerIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      if(!(image->GetLargestPossibleRegion().IsInside(centerIndex) && mask->IsValid(centerIndex)))
        {
        continue;
        }

      // Determine which neighbors are valid
      bool backwardValid = false;
      itk::Index<2> backwardIndex;
      backwardIndex[direction] = imageIterator.GetIndex()[direction] - 1;
      backwardIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      if(image->GetLargestPossibleRegion().IsInside(backwardIndex) && mask->IsValid(backwardIndex))
        {
        backwardValid = true;
        }

      bool forwardValid = false;
      itk::Index<2> forwardIndex;
      forwardIndex[direction] = imageIterator.GetIndex()[direction] + 1;
      forwardIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      
      if(image->GetLargestPossibleRegion().IsInside(forwardIndex) && mask->IsValid(forwardIndex))
        {
        forwardValid = true;
        }

      if(backwardValid && !forwardValid) // Use backwards half difference
        {
        difference += image->GetPixel(centerIndex) - image->GetPixel(backwardIndex);
        numberUsed++;
        }
      else if(!backwardValid && forwardValid) // Use forwards half difference
        {
        difference += image->GetPixel(forwardIndex) - image->GetPixel(centerIndex);
        numberUsed++;
        }
      else if(backwardValid && forwardValid) // Use full difference
        {
        difference += (image->GetPixel(forwardIndex) - image->GetPixel(backwardIndex))/2.0f;
        numberUsed++;
        }
      else// if(!backwardValid && !forwardValid) // No valid neighbors in this direction
        {
        difference += 0.0f; // There is nothing we can do here, so set the derivative to zero.
        }
      } // end shift loop

    if(numberUsed > 1)
      {
      difference /= static_cast<float>(numberUsed);
      }

    output->SetPixel(imageIterator.GetIndex(), difference);

    ++imageIterator;
    }
}


template <typename TImage>
void MaskedDerivativeSobel(const typename TImage::Pointer image, const Mask::Pointer mask, const unsigned int direction, FloatScalarImageType::Pointer output)
{
  if(direction > 1)
    {
    std::cerr << "This function can only compute derivatives of 2D images!" << std::endl;
    exit(-1);
    }

  // Setup the output
  InitializeImage<FloatScalarImageType>(output, image->GetLargestPossibleRegion());

  itk::ImageRegionIterator<TImage> imageIterator(image, image->GetLargestPossibleRegion());

  // If we are taking x derivatives, we want to use 3 columns. If we are taking y derivatives, we want to use 3 rows.
  unsigned int shiftIndex;
  if(direction == 0)
    {
    shiftIndex = 1;
    }
  else
    {
    shiftIndex = 0;
    }

  while(!imageIterator.IsAtEnd())
    {
    // We should not compute derivatives for pixels in the hole.
    if(mask->IsHole(imageIterator.GetIndex()))
      {
      ++imageIterator;
      continue;
      }

    float totalDifference = 0.0f;
    unsigned int numberUsed = 0;
    for(int shift = -1; shift <= 1; shift++) // this shift is either rows or columns, depending on the derivative direction
      {
      itk::Index<2> centerIndex;
      centerIndex[direction] = imageIterator.GetIndex()[direction];
      centerIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      if(!(image->GetLargestPossibleRegion().IsInside(centerIndex) && mask->IsValid(centerIndex)))
        {
        continue;
        }

      float difference = 0.0f;
    
      // Determine which neighbors are valid
      bool backwardValid = false;
      itk::Index<2> backwardIndex;
      backwardIndex[direction] = imageIterator.GetIndex()[direction] - 1;
      backwardIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      if(image->GetLargestPossibleRegion().IsInside(backwardIndex) && mask->IsValid(backwardIndex))
        {
        backwardValid = true;
        }

      bool forwardValid = false;
      itk::Index<2> forwardIndex;
      forwardIndex[direction] = imageIterator.GetIndex()[direction] + 1;
      forwardIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;

      if(image->GetLargestPossibleRegion().IsInside(forwardIndex) && mask->IsValid(forwardIndex))
        {
        forwardValid = true;
        }
      unsigned int weight = 1;
      if(shift == 0)
        {
        weight = 2;
        }
      if(backwardValid && !forwardValid) // Use backwards half difference
        {
        difference = image->GetPixel(centerIndex) - image->GetPixel(backwardIndex);
        numberUsed += weight;
        }
      else if(!backwardValid && forwardValid) // Use forwards half difference
        {
        difference = image->GetPixel(forwardIndex) - image->GetPixel(centerIndex);
        numberUsed += weight;
        }
      else if(backwardValid && forwardValid) // Use full difference
        {
        difference = (image->GetPixel(forwardIndex) - image->GetPixel(backwardIndex))/2.0f;
        numberUsed += weight;
        }
      else// if(!backwardValid && !forwardValid) // No valid neighbors in this direction
        {
        difference = 0.0f; // There is nothing we can do here, so set the derivative to zero.
        }

      difference *= static_cast<float>(weight);

      totalDifference += difference;
      } // end shift loop

    if(numberUsed > 1)
      {
      totalDifference /= static_cast<float>(numberUsed);
      }

    output->SetPixel(imageIterator.GetIndex(), totalDifference);

    ++imageIterator;
    }
}


template <typename TImage>
void MaskedDerivativeGaussian(const typename TImage::Pointer image, const Mask::Pointer mask, const unsigned int direction, FloatScalarImageType::Pointer output)
{
  if(direction > 1)
    {
    std::cerr << "This function can only compute derivatives of 2D images!" << std::endl;
    exit(-1);
    }

  // Create a Gaussian kernel
  typedef itk::GaussianOperator<float, 1> GaussianOperatorType;

  // Make a (2*kernelRadius+1)x1 kernel
  itk::Size<1> radius;
  radius.Fill(5); // Make a length 11 kernel

  GaussianOperatorType gaussianOperator;
  gaussianOperator.SetDirection(0); // It doesn't matter which direction we set - we will be interpreting the kernel as 1D (no direction)
  gaussianOperator.SetVariance(3);
  gaussianOperator.CreateToRadius(radius);
  
  // Setup the output
  InitializeImage<FloatScalarImageType>(output, image->GetLargestPossibleRegion());

  itk::ImageRegionIterator<TImage> imageIterator(image, image->GetLargestPossibleRegion());

  // If we are taking x derivatives, we want to use 3 columns. If we are taking y derivatives, we want to use 3 rows.
  unsigned int shiftIndex;
  if(direction == 0)
    {
    shiftIndex = 1;
    }
  else
    {
    shiftIndex = 0;
    }

  while(!imageIterator.IsAtEnd())
    {
    // We should not compute derivatives for pixels in the hole.
    if(mask->IsHole(imageIterator.GetIndex()))
      {
      ++imageIterator;
      continue;
      }

    float totalDifference = 0.0f;
    float totalWeight = 0.0f;
    for(unsigned int shiftId = 0; shiftId < gaussianOperator.Size(); shiftId++) // this shift is either rows or columns, depending on the derivative direction
      {
      int shift = gaussianOperator.GetOffset(shiftId)[0];
    
      itk::Index<2> centerIndex;
      centerIndex[direction] = imageIterator.GetIndex()[direction];
      centerIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      if(!(image->GetLargestPossibleRegion().IsInside(centerIndex) && mask->IsValid(centerIndex)))
        {
        continue;
        }

      float difference = 0.0f;

      // Determine which neighbors are valid
      bool backwardValid = false;
      itk::Index<2> backwardIndex;
      backwardIndex[direction] = imageIterator.GetIndex()[direction] - 1;
      backwardIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;
      if(image->GetLargestPossibleRegion().IsInside(backwardIndex) && mask->IsValid(backwardIndex))
        {
        backwardValid = true;
        }

      bool forwardValid = false;
      itk::Index<2> forwardIndex;
      forwardIndex[direction] = imageIterator.GetIndex()[direction] + 1;
      forwardIndex[shiftIndex] = imageIterator.GetIndex()[shiftIndex] + shift;

      if(image->GetLargestPossibleRegion().IsInside(forwardIndex) && mask->IsValid(forwardIndex))
        {
        forwardValid = true;
        }

      float weight = gaussianOperator.GetElement(shiftId);

      if(backwardValid && !forwardValid) // Use backwards half difference
        {
        difference = image->GetPixel(centerIndex) - image->GetPixel(backwardIndex);
        totalWeight += weight;
        }
      else if(!backwardValid && forwardValid) // Use forwards half difference
        {
        difference = image->GetPixel(forwardIndex) - image->GetPixel(centerIndex);
        totalWeight += weight;
        }
      else if(backwardValid && forwardValid) // Use full difference
        {
        difference = (image->GetPixel(forwardIndex) - image->GetPixel(backwardIndex))/2.0f;
        totalWeight += weight;
        }
      else// if(!backwardValid && !forwardValid) // No valid neighbors in this direction
        {
        difference = 0.0f; // There is nothing we can do here, so set the derivative to zero.
        }

      difference *= weight;
      totalDifference += difference;
      totalWeight += weight;
      } // end shift loop

    if(totalWeight > 0.0f)
      {
      totalDifference /= totalWeight;
      }

    output->SetPixel(imageIterator.GetIndex(), totalDifference);

    ++imageIterator;
    }
}

template <typename TImage>
void MaskedGradient(const typename TImage::Pointer image, const Mask::Pointer mask, FloatVector2ImageType::Pointer output)
{
  // Compute the derivatives
  // X derivative
  FloatScalarImageType::Pointer xDerivative = FloatScalarImageType::New();
  //Helpers::MaskedDerivative<FloatScalarImageType>(image, mask, 0, xDerivative);
  //Helpers::MaskedDerivativePrewitt<FloatScalarImageType>(image, mask, 0, xDerivative);
  //Helpers::MaskedDerivativeSobel<FloatScalarImageType>(image, mask, 0, xDerivative);
  Helpers::MaskedDerivativeGaussian<FloatScalarImageType>(image, mask, 0, xDerivative);
  //Helpers::DebugWriteImageConditional<FloatScalarImageType>(xDerivative, "Debug/ComputeMaskedIsophotes.xderivative.mha", this->DebugImages);

  // Y derivative
  FloatScalarImageType::Pointer yDerivative = FloatScalarImageType::New();
  //Helpers::MaskedDerivative<FloatScalarImageType>(image, mask, 1, yDerivative);
  //Helpers::MaskedDerivativePrewitt<FloatScalarImageType>(image, mask, 1, yDerivative);
  //Helpers::MaskedDerivativeSobel<FloatScalarImageType>(image, mask, 1, yDerivative);
  Helpers::MaskedDerivativeGaussian<FloatScalarImageType>(image, mask, 1, yDerivative);
  //Helpers::DebugWriteImageConditional<FloatScalarImageType>(yDerivative, "Debug/ComputeMaskedIsophotes.yderivative.mha", this->DebugImages);

  // Combine derivatives
  Helpers::GradientFromDerivatives<float>(xDerivative, yDerivative, output);
}


template<typename TImage>
void InitializeImage(typename TImage::Pointer image, const itk::ImageRegion<2>& region)
{
  image->SetRegions(region);
  image->Allocate();
  image->FillBuffer(0);
}

}// end namespace
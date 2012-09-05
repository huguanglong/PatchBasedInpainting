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

#ifndef IntroducedEnergy_HPP
#define IntroducedEnergy_HPP

#include "IntroducedEnergy.h"

#include "ImageProcessing/Derivatives.h"

template <typename TImage>
float IntroducedEnergy<TImage>::ComputeIntroducedEnergyPatchBoundary(const TImage* const image, const Mask* const mask,
                                                                     const itk::ImageRegion<2>& sourceRegion, const itk::ImageRegion<2>& targetRegion)
{
  typedef itk::Image<itk::CovariantVector<float, 2> > GradientImageType;

  // Compute gradients in the target region
  GradientImageType::Pointer targetRegionGradientImage = GradientImageType::New();
  targetRegionGradientImage->SetRegions(image->GetLargestPossibleRegion());
  targetRegionGradientImage->Allocate();

  Derivatives::MaskedGradientInRegion(image, mask, targetRegion, targetRegionGradientImage.GetPointer());

  // Copy the source patch to the target patch
  typename TImage::Pointer inpaintedImage = TImage::New();
  ITKHelpers::DeepCopy(image, inpaintedImage.GetPointer());
  ITKHelpers::CopyRegion(image, inpaintedImage.GetPointer(), sourceRegion, targetRegion);

  // Compute the gradient of the inpainted image
  GradientImageType::Pointer inpaintedGradientImage = GradientImageType::New();
  inpaintedGradientImage->SetRegions(image->GetLargestPossibleRegion());
  inpaintedGradientImage->Allocate();

  Derivatives::MaskedGradientInRegion(inpaintedImage.GetPointer(), mask, targetRegion, inpaintedGradientImage.GetPointer());

  // Get the pixels on the boundary (outline) of the region
  typedef std::vector<itk::Index<2> > PixelContainer;
  PixelContainer patchBoundaryPixels = ITKHelpers::GetBoundaryPixels(targetRegion);

  // Remove boundary pixels that are not in the valid region of the patch (i.e. remove hole pixels)
  patchBoundaryPixels.erase(std::remove_if(patchBoundaryPixels.begin(), patchBoundaryPixels.end(),
                    [mask](const itk::Index<2>& queryPixel)
                    {
                      return mask->IsHole(queryPixel);
                    }),
                    patchBoundaryPixels.end());

  // Compare the gradient magnitude before and after the inpainting
  float gradientMagnitudeChange = 0.0f;

  for(PixelContainer::const_iterator boundaryPixelIterator = patchBoundaryPixels.begin();
      boundaryPixelIterator != patchBoundaryPixels.end(); ++boundaryPixelIterator)
  {
    gradientMagnitudeChange += (targetRegionGradientImage->GetPixel(*boundaryPixelIterator) -
                                inpaintedGradientImage->GetPixel(*boundaryPixelIterator)).GetSquaredNorm();
  }

  return gradientMagnitudeChange;
}

template <typename TImage>
float IntroducedEnergy<TImage>::ComputeIntroducedEnergyMaskBoundary(const TImage* const image, const Mask* const mask,
                                                                    const itk::ImageRegion<2>& sourceRegion, const itk::ImageRegion<2>& targetRegion)
{
  typedef itk::Image<itk::CovariantVector<float, 2> > GradientImageType;
  // Find pixels in the region and on the mask boundary
  typedef std::vector<itk::Index<2> > PixelContainer;
  PixelContainer holeBoundaryPixels = mask->FindBoundaryPixelsInRegion(targetRegion, Mask::HOLE);
  PixelContainer validBoundaryPixels = mask->FindBoundaryPixelsInRegion(targetRegion, Mask::VALID);
  PixelContainer boundaryPixels;
  boundaryPixels.insert(holeBoundaryPixels.begin(), holeBoundaryPixels.end(), boundaryPixels.end());
  boundaryPixels.insert(validBoundaryPixels.begin(), validBoundaryPixels.end(), boundaryPixels.end());

  // Compute gradients in the target region
  GradientImageType::Pointer targetRegionGradientImage = GradientImageType::New();
  targetRegionGradientImage->SetRegions(image->GetLargestPossibleRegion());
  targetRegionGradientImage->Allocate();

  Derivatives::MaskedGradientInRegion(image, mask, targetRegion, targetRegionGradientImage.GetPointer());

  // Copy the patch into an image
  typename TImage::Pointer inpaintedImage = TImage::New();
  ITKHelpers::DeepCopy(image, inpaintedImage.GetPointer());
  ITKHelpers::CopyRegion(image, inpaintedImage.GetPointer(), sourceRegion, targetRegion);

  // Compute the gradient of the inpainted image
  GradientImageType::Pointer inpaintedGradientImage = GradientImageType::New();
  inpaintedGradientImage->SetRegions(image->GetLargestPossibleRegion());
  inpaintedGradientImage->Allocate();

  // Compare the gradient magnitude before and after the inpainting
  float gradientMagnitudeChange = 0.0f;

  for(PixelContainer::const_iterator boundaryPixelIterator = boundaryPixels.begin();
      boundaryPixelIterator != boundaryPixels.end(); ++boundaryPixelIterator)
  {
    gradientMagnitudeChange += (targetRegionGradientImage->GetPixel(*boundaryPixelIterator) -
                                inpaintedGradientImage->GetPixel(*boundaryPixelIterator)).GetSquaredNorm();
  }

  return gradientMagnitudeChange;
}

template <typename TImage>
float IntroducedEnergy<TImage>::ComputeIntroducedEnergy(const TImage* const image, const Mask* const mask,
                                                        const itk::ImageRegion<2>& sourceRegion, const itk::ImageRegion<2>& targetRegion)
{
  return ComputeIntroducedEnergyPatchBoundary(image, mask, sourceRegion, targetRegion) +
      ComputeIntroducedEnergyMaskBoundary(image, mask, sourceRegion, targetRegion);
}

#endif
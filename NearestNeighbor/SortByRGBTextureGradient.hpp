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

#ifndef SortByRGBTextureGradient_HPP
#define SortByRGBTextureGradient_HPP

// Submodules
#include <Utilities/Histogram/HistogramHelpers.hpp>
#include <Utilities/Histogram/HistogramDifferences.hpp>
#include <Utilities/Histogram/HistogramGenerator.h>
#include <Utilities/Histogram/MaskedHistogramGenerator.h>

#include <ITKHelpers/itkNormImageAdaptor.h>

#include <Helpers/ParallelSort.h>

// Custom
#include "ImageProcessing/Derivatives.h"
#include <Utilities/Debug/Debug.h>

// ITK
#include "itkNthElementImageAdaptor.h"

/**
 * This class uses comparisons of histograms of the gradient magnitudes to sort a set of matches
 * by their Gradient Magnitude Histogram difference.
 *
 * This class expects an RGB image to be passed.
 */
template <typename PropertyMapType, typename TImage, typename TImageToWrite = TImage>
class SortByRGBTextureGradient : public Debug
{
  PropertyMapType PropertyMap;
  TImage* Image;
  Mask* MaskImage;
  unsigned int Iteration = 0;
  TImageToWrite* ImageToWrite = nullptr;

  typedef itk::Image<itk::CovariantVector<float, 2>, 2> GradientImageType;
  std::vector<GradientImageType::Pointer> ChannelGradients;

public:
  /** Constructor. This class requires the property map, an image, and a mask. */
  SortByRGBTextureGradient(PropertyMapType propertyMap, TImage* const image, Mask* const mask,
                           TImageToWrite* imageToWrite = nullptr, const Debug& debug = Debug()) :
    Debug(debug), PropertyMap(propertyMap), Image(image), MaskImage(mask), ImageToWrite(imageToWrite)
  {
    // Compute the gradients in all source patches
    typedef itk::NthElementImageAdaptor<TImage, float> ImageChannelAdaptorType;
    typename ImageChannelAdaptorType::Pointer imageChannelAdaptor = ImageChannelAdaptorType::New();
    imageChannelAdaptor->SetImage(this->Image);

    this->ChannelGradients.resize(3);

    for(unsigned int channel = 0; channel < 3; ++channel) // 3 RGB channels
    {
      imageChannelAdaptor->SelectNthElement(channel);

      GradientImageType::Pointer gradientImage = GradientImageType::New();
      gradientImage->SetRegions(this->Image->GetLargestPossibleRegion());
      gradientImage->Allocate();

      Derivatives::MaskedGradient(imageChannelAdaptor.GetPointer(), this->MaskImage,
                                    gradientImage.GetPointer());

      this->ChannelGradients[channel] = gradientImage;

      if(this->DebugImages)
      {
        std::stringstream ss;
        ss << "RGB_Gradient_" << channel << ".mha";
        ITKHelpers::WriteImage(this->ChannelGradients[channel].GetPointer(), ss.str());
      }
    }
  }

  struct RegionSorter
  {
    itk::Functor::IndexLexicographicCompare<2> IndexCompareFunctor;
    bool operator()(const itk::ImageRegion<2> region1, const itk::ImageRegion<2> region2) const
    {
      return IndexCompareFunctor(region1.GetIndex(), region2.GetIndex());
    }
  };

  typedef float BinValueType; // bins must be float if we are going to normalize
  typedef MaskedHistogramGenerator<BinValueType> MaskedHistogramGeneratorType;
  typedef HistogramGenerator<BinValueType> HistogramGeneratorType;
  typedef HistogramGeneratorType::HistogramType HistogramType;

  typedef std::map<itk::ImageRegion<2>, HistogramType, RegionSorter> HistogramMapType;
  HistogramMapType PreviouslyComputedHistograms;

  /**
    * \tparam TIterator The forward-iterator type.
    * \tparam TOutputIterator The iterator type of the output container.
    * \param first Start of the range in which to search.
    * \param last One element past the last element in the range in which to search.
    * \param query The element to compare to.
    * \param outputFirst An iterator pointing to the beginning of the (preallocated) output container.
    * \return The iterator to the best element in the range (best is defined as the one which would compare favorably to all
    *         the elements in the range with respect to the distance metric).
    */
  template <typename TIterator, typename TOutputIterator>
  TOutputIterator operator()(const TIterator first, const TIterator last,
                             typename TIterator::value_type query,
                             TOutputIterator outputFirst)
  {
    unsigned int numberOfBins = 30;

    // If the input element range is empty, there is nothing to do.
    if(first == last)
    {
      std::cerr << "LinearSearchBestHistogram: Nothing to do..." << std::endl;
      return outputFirst;
    }

    // Get the region to process
    itk::ImageRegion<2> queryRegion = get(this->PropertyMap, query).GetRegion();

    typedef itk::NthElementImageAdaptor<TImage, float> ImageChannelAdaptorType;
    typename ImageChannelAdaptorType::Pointer imageChannelAdaptor = ImageChannelAdaptorType::New();
    imageChannelAdaptor->SetImage(this->Image);

    // Compute the gradient of each channel
    for(unsigned int channel = 0; channel < 3; ++channel) // 3 is the number of RGB channels
    {
      imageChannelAdaptor->SelectNthElement(channel);

      Derivatives::MaskedGradientInRegion(imageChannelAdaptor.GetPointer(), this->MaskImage,
                                          queryRegion, this->ChannelGradients[channel].GetPointer());
    }

    HistogramType targetHistogram;

    // Store, for each channel (the elements of the vector), the min/max value of the valid region of the target patch
    std::vector<GradientImageType::PixelType::RealValueType> minChannelGradientMagnitudes(this->Image->GetNumberOfComponentsPerPixel());
    std::vector<GradientImageType::PixelType::RealValueType> maxChannelGradientMagnitudes(this->Image->GetNumberOfComponentsPerPixel());

    std::vector<itk::Index<2> > validPixels = ITKHelpers::GetPixelsWithValueInRegion(this->MaskImage, queryRegion, this->MaskImage->GetValidValue());

    typedef itk::NormImageAdaptor<GradientImageType, GradientImageType::PixelType::RealValueType> NormImageAdaptorType;
    typename NormImageAdaptorType::Pointer normImageAdaptor = NormImageAdaptorType::New();

    // Compute the gradient magnitude images for each RGB channel's gradient, and compute the histograms for the target/query region
    for(unsigned int channel = 0; channel < 3; ++channel) // 3 is the number of RGB channels
    {
      imageChannelAdaptor->SelectNthElement(channel);

      normImageAdaptor->SetImage(this->ChannelGradients[channel].GetPointer());

      std::vector<GradientImageType::PixelType::RealValueType> gradientMagnitudes =
          ITKHelpers::GetPixelValues(normImageAdaptor.GetPointer(), validPixels);

      minChannelGradientMagnitudes[channel] = Helpers::Min(gradientMagnitudes);
      maxChannelGradientMagnitudes[channel] = Helpers::Max(gradientMagnitudes);

      // Compute histograms of the gradient magnitudes (to measure texture)
      bool allowOutside = false;
      HistogramType targetChannelHistogram =
        MaskedHistogramGeneratorType::ComputeMaskedScalarImageHistogram(
            normImageAdaptor.GetPointer(), queryRegion, this->MaskImage, queryRegion, numberOfBins,
            minChannelGradientMagnitudes[channel], maxChannelGradientMagnitudes[channel],
            allowOutside, this->MaskImage->GetValidValue());

      targetChannelHistogram.Normalize();

      targetHistogram.Append(targetChannelHistogram);
    }

    // Initialize
    float bestDistance = std::numeric_limits<float>::max();
    TIterator bestPatch = last;

    unsigned int bestId = 0; // Keep track of which of the top SSD patches is the best by histogram score (just for information sake)
    HistogramType bestHistogram;

    // Store the scores in this container so we can sort them later
    std::vector<float> scores(last - first);

    // Iterate through all of the supplied source patches
    for(TIterator currentPatch = first; currentPatch != last; ++currentPatch)
    {
      itk::ImageRegion<2> currentRegion = get(this->PropertyMap, *currentPatch).GetRegion();

      // Determine if the gradient and histogram have already been computed
      typename HistogramMapType::iterator histogramMapIterator;
      histogramMapIterator = this->PreviouslyComputedHistograms.find(currentRegion);

      bool alreadyComputed;

      if(histogramMapIterator == this->PreviouslyComputedHistograms.end())
      {
        alreadyComputed = false;
      }
      else
      {
        alreadyComputed = true;
      }

      HistogramType testHistogram;

      bool allowOutside = true;
      // Compute the RGB histograms of the source region using the queryRegion mask
      for(unsigned int channel = 0; channel < 3; ++channel) // 3 is the number of RGB channels
      {
        normImageAdaptor->SetImage(this->ChannelGradients[channel].GetPointer());

        HistogramType testChannelHistogram;

        if(!alreadyComputed)
        {
          // We don't need a masked histogram since we are using the full source patch
          testChannelHistogram = HistogramGeneratorType::ComputeScalarImageHistogram(
                          normImageAdaptor.GetPointer(), currentRegion,
                          numberOfBins,
                          minChannelGradientMagnitudes[channel],
                          maxChannelGradientMagnitudes[channel], allowOutside);

          testChannelHistogram.Normalize();

          this->PreviouslyComputedHistograms[currentRegion] = testChannelHistogram;
        }
        else // already computed
        {
          testChannelHistogram = this->PreviouslyComputedHistograms[currentRegion];
        }

        testHistogram.Append(testChannelHistogram);
      }

      // Compute the differences in the histograms
      float histogramDifference = HistogramDifferences::HistogramDifference(targetHistogram, testHistogram);

      scores[currentPatch - first] = histogramDifference;

      if(histogramDifference < bestDistance)
      {
        bestDistance = histogramDifference;
        bestPatch = currentPatch;

        // These are not needed - just for debugging
        bestId = currentPatch - first;
        bestHistogram = testHistogram;
      }
    }

    std::cout << "BestId: " << bestId << std::endl;
    std::cout << "Best distance: " << bestDistance << std::endl;

    if(this->DebugOutputFiles)
    {
      Helpers::WriteVectorToFileLines(scores, Helpers::GetSequentialFileName("Scores", this->Iteration, "txt", 3));
    }

    typedef ParallelSort<float> ParallelSortType;

    ParallelSortType::IndexedVector sortedScores = ParallelSortType::ParallelSortAscending(scores);


    TOutputIterator currentOutputIterator = outputFirst; // c++ doesn't allow a second counter to be declared inside for()
    for(unsigned int i = 0; i < sortedScores.size(); ++i, ++currentOutputIterator)
    {
      unsigned int currentId = sortedScores[i].index;

      TIterator current = first;
      std::advance(current, currentId);

      *currentOutputIterator = *current;
    }

    this->Iteration++;

    return outputFirst;
  } // end operator()

}; // end class SortByRGBTextureGradient

#endif
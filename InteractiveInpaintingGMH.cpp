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

// ITK
#include "itkImageFileReader.h"

// Submodules
#include <ITKHelpers/ITKHelpers.h>
#include <Mask/Mask.h>

// Qt
#include <QApplication>

// Driver
#include "Drivers/InteractiveInpaintingGMH.hpp"
#include "Drivers/TestDriver.hpp"

// Run with: image.png image.mask 15 100 .1 filled.png
int main(int argc, char *argv[])
{
  // Verify arguments
  if(argc != 7)
  {
    std::cerr << "Required arguments: image.png image.mask patchHalfWidth"
              << " knn maxAllowedDifference output.png" << std::endl;
    std::cerr << "Input arguments: ";
    for(int i = 1; i < argc; ++i)
    {
      std::cerr << argv[i] << " ";
    }
    return EXIT_FAILURE;
  }

  // Parse arguments
  std::string imageFilename = argv[1];
  std::string maskFilename = argv[2];

  std::stringstream ssPatchRadius;
  ssPatchRadius << argv[3];
  unsigned int patchHalfWidth = 0;
  ssPatchRadius >> patchHalfWidth;

  std::stringstream ssKNN;
  ssKNN << argv[4];
  float knn = 0.0f;
  ssKNN >> knn;

  std::stringstream ssMaxAllowedDifference;
  ssMaxAllowedDifference << argv[5];
  float maxAllowedDifference = 0.0f;
  ssMaxAllowedDifference >> maxAllowedDifference;

  std::string outputFilename = argv[6];

  // Output arguments
  std::cout << "Reading image: " << imageFilename << std::endl;
  std::cout << "Reading mask: " << maskFilename << std::endl;
  std::cout << "Patch half width: " << patchHalfWidth << std::endl;
  std::cout << "KNN: " << knn << std::endl;
  std::cout << "Max Allowed Difference: " << maxAllowedDifference << std::endl;
  std::cout << "Output: " << outputFilename << std::endl;

  typedef itk::Image<itk::CovariantVector<float, 3>, 2> ImageType;

  typedef  itk::ImageFileReader<ImageType> ImageReaderType;
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName(imageFilename);
  imageReader->Update();

  ImageType::Pointer image = ImageType::New();
  ITKHelpers::DeepCopy(imageReader->GetOutput(), image.GetPointer());

  Mask::Pointer mask = Mask::New();
  mask->Read(maskFilename);

  std::cout << "Mask size: " << mask->GetLargestPossibleRegion().GetSize() << std::endl;
  std::cout << "hole pixels: " << mask->CountHolePixels() << std::endl;
  std::cout << "valid pixels: " << mask->CountValidPixels() << std::endl;

  // Setup the GUI system
  QApplication app( argc, argv );
  // Without this, after we close the first dialog
  // (after the first iteration that is not accepted automatically), the event loop quits.
  app.setQuitOnLastWindowClosed(false);

  InteractiveInpaintingGMH(image, mask, patchHalfWidth, knn,
                           maxAllowedDifference, outputFilename);

  // Can't do anything here like write the output image because we get to app.exec() at the very beginning
  // of the algorithm because it is running in a different thread.

  return app.exec();
}

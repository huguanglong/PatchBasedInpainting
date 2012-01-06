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

#ifndef PATCHPAIR_H
#define PATCHPAIR_H

#include "PairDifferences.h"
#include "Patch.h"

#include <memory>

class PatchPair
{
public:

  PatchPair(const Patch* const sourcePatch, const Patch& targetPatch);

  // Compute the relative location of the source and target patch corners
  itk::Offset<2> GetTargetToSourceOffset() const;
  itk::Offset<2> GetSourceToTargetOffset() const;

  const Patch* GetSourcePatch() const;
  const Patch& GetTargetPatch() const;

  PairDifferences& GetDifferences();
  const PairDifferences& GetDifferences() const;

private:
  PairDifferences Differences;
  const Patch* const SourcePatch; // This is a pointer to a patch in the main (only) SourcePatchCollection.
  Patch TargetPatch; // This is not a pointer to a patch in the main SourcePatchCollection, because it is not valid yet (we are going to copy pixels here)!
};

struct PairSortFunctor
{
  PairSortFunctor(const PairDifferences::PatchDifferenceTypes sortBy) : SortBy(sortBy) {}
  bool operator()(const PatchPair& pair1, const PatchPair& pair2);

  bool operator()(const std::shared_ptr<PatchPair>& pair1, const std::shared_ptr<PatchPair>& pair2);

  PairDifferences::PatchDifferenceTypes SortBy;
};

#endif

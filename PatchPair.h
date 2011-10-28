#ifndef PATCHPAIR_H
#define PATCHPAIR_H

#include "Patch.h"

struct PatchPair
{
  void DefaultConstructor();
  PatchPair();
  PatchPair(const Patch& sourcePatch, const Patch& targetPatch);
  
  Patch SourcePatch;
  Patch TargetPatch;
  
  float GetAverageSquaredDifference() const;
  float GetTotalSquaredDifference() const;
  float GetTotalAbsoluteDifference() const;
  float GetAverageAbsoluteDifference() const;
  
  float GetBoundaryPixelDifference() const;
  float GetBoundaryIsophoteAngleDifference() const;
  float GetBoundaryIsophoteStrengthDifference() const;
  float GetTotalScore() const;
  
  void SetAverageSquaredDifference(const float value);
  void SetTotalSquaredDifference(const float value);
  void SetAverageAbsoluteDifference(const float value);
  void SetTotalAbsoluteDifference(const float value);
  
  void SetBoundaryPixelDifference(const float value);
  void SetBoundaryIsophoteAngleDifference(const float value);
  void SetBoundaryIsophoteStrengthDifference(const float value);
  
  bool IsValidAverageSquaredDifference();
  bool IsValidTotalSquaredDifference();
  bool IsValidAverageAbsoluteDifference();
  bool IsValidTotalAbsoluteDifference();
  
  bool IsValidBoundaryPixelDifference();
  bool IsValidBoundaryIsophoteAngleDifference();
  bool IsValidBoundaryIsophoteStrengthDifference();

  itk::Offset<2> GetTargetToSourceOffset() const;
  itk::Offset<2> GetSourceToTargetOffset() const;
  
private:
  float TotalSquaredDifference;
  float AverageSquaredDifference;
  float TotalAbsoluteDifference;
  float AverageAbsoluteDifference;
  
  float BoundaryPixelDifference;
  float BoundaryIsophoteAngleDifference;
  float BoundaryIsophoteStrengthDifference;
  
  float TotalScore;
  
  void ComputeTotal();
  
  // These are initialized to false and set to true when the corresponding SetXYZ() function is called.
  bool ValidTotalSquaredDifference;
  bool ValidAverageSquaredDifference;
  bool ValidTotalAbsoluteDifference;
  bool ValidAverageAbsoluteDifference;
  
  bool ValidBoundaryPixelDifference;
  bool ValidBoundaryIsophoteAngleDifference;
  bool ValidBoundaryIsophoteStrengthDifference;
};

bool SortByAverageSquaredDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByAverageAbsoluteDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByTotalSquaredDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByTotalAbsoluteDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByBoundaryIsophoteAngleDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByBoundaryIsophoteStrengthDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByBoundaryPixelDifference(const PatchPair& pair1, const PatchPair& pair2);
bool SortByTotalScore(const PatchPair& pair1, const PatchPair& pair2);

#endif
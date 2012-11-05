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

#ifndef MovablePatch_H
#define MovablePatch_H

// Custom
#include "Layer.h"
class InteractorStyleImageWithDrag;

// ITK
#include "itkImageRegion.h"

// Qt
#include <QColor>
#include <QGraphicsScene>
#include <QObject>
class QGraphicsView;

// VTK
class vtkRenderer;
#include <vtkImageSlice.h>

/** This class gives us a little square that the user can drag around to select a region/patch in
  * an image. It also lets the patch be displayed in a QGraphicsView.
  */
class MovablePatch : public QObject
{
Q_OBJECT

signals:
  void signal_PatchMoved();

public:
  /** This constructor is provided so that we can store a MovablePatch as a member,
    * but not initialize it until after the renderers/interactors are setup and configured.*/
//  MovablePatch();

  /** This constructor is provided if everything is known when we create the object.*/
  MovablePatch(const unsigned int radius, InteractorStyleImageWithDrag* const interactorStyle,
               QGraphicsView* const view = NULL, const QColor color = QColor());

  void SetVisibility(const bool);
  bool GetVisibility();

  /** The ITK region describing the position of the patch. */
  itk::ImageRegion<2> GetRegion();

  /** Set a GraphicsView in which to display the patch. */
  void SetGraphicsView(QGraphicsView* const view);

  /** Display the selected patch in the QGraphcisView. */
  void Display();

private:
  /** We need an ImageData/Slice/SliceMapper to display the square. */
  Layer PatchLayer;

  /** This function is used as a callback from VTK's interactor style.
    * Once the mouse is released, we update some things and then emit our own
    * signal indicating that the patch was moved. */
  void PatchMoved();

  /** The radius of the patch. */
  unsigned int Radius = 0;

  /** The interactor style in which the patch is displayed. */
  InteractorStyleImageWithDrag* InteractorStyle = nullptr;

  /** The QGraphicsView in which to display the patch. */
  QGraphicsView* View = nullptr;

  /** The color of the patch outline. */
  QColor Color;

  /** The scene to draw into and then display in the QGraphicsView. */
  QSharedPointer<QGraphicsScene> PatchScene = QSharedPointer<QGraphicsScene>(new QGraphicsScene);

  /** The image to draw in the scene that is displayed in the QGraphicsView. */
  QImage PatchImage;
};

#endif

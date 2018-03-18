/*=======================================================================
 *** THE CONTENT OF THIS WORK IS PROPRIETARY TO FEI S.A.S, (FEI S.A.S.),            ***
 ***              AND IS DISTRIBUTED UNDER A LICENSE AGREEMENT.                     ***
 ***                                                                                ***
 ***  REPRODUCTION, DISCLOSURE,  OR USE,  IN WHOLE OR IN PART,  OTHER THAN AS       ***
 ***  SPECIFIED  IN THE LICENSE ARE  NOT TO BE  UNDERTAKEN  EXCEPT WITH PRIOR       ***
 ***  WRITTEN AUTHORIZATION OF FEI S.A.S.                                           ***
 ***                                                                                ***
 ***                        RESTRICTED RIGHTS LEGEND                                ***
 ***  USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT OF THE CONTENT OF THIS      ***
 ***  WORK OR RELATED DOCUMENTATION IS SUBJECT TO RESTRICTIONS AS SET FORTH IN      ***
 ***  SUBPARAGRAPH (C)(1) OF THE COMMERCIAL COMPUTER SOFTWARE RESTRICTED RIGHT      ***
 ***  CLAUSE  AT FAR 52.227-19  OR SUBPARAGRAPH  (C)(1)(II)  OF  THE RIGHTS IN      ***
 ***  TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 52.227-7013.             ***
 ***                                                                                ***
 ***                   COPYRIGHT (C) 1996-2017 BY FEI S.A.S,                        ***
 ***                        MERIGNAC, FRANCE                                        ***
 ***                      ALL RIGHTS RESERVED                                       ***
**=======================================================================*/
/*=======================================================================
** Author      : VSG (MMM YYYY)
**=======================================================================*/

#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoBBox.h>

#include <Inventor/events/SoKeyboardEvent.h>

#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/sensors/SoOneShotSensor.h>
#include <Inventor/SoWinApp.h>
#include <Inventor/SoMacApp.h>

#ifdef DVD_DEMO
#include <Inventor/lock/SoLockMgr.h>
#endif

#include <DialogViz/SoDialogVizAll.h>

//#define SO_VEXTEX_PROPERTY_IN_STATE

#define SO_DEFAULT_MESH_SIZE  400
#define SO_MIN_MESH_SIZE      50
#define SO_MESH_SIZE_STEP     50
#define SO_ANIM_STEP          0.05f
#define SO_PERIOD             12.5653f
#define SO_DEFAULT_AMPLITUDE  10.0f
#define SO_TITLE_VBO          "VBO mode"
#define SO_TITLE_NORMAL       "Normal mode"
#define SO_STRIP_COLOR_WIDTH  0.125f

int                        MeshSize=SO_DEFAULT_MESH_SIZE;
float                      MeshAmplitude=SO_DEFAULT_AMPLITUDE*((float)MeshSize/(float)SO_DEFAULT_MESH_SIZE) ;
int                        NumTriangles=MeshSize*MeshSize*2;
int                        AnimStep = (int)(MeshSize * SO_ANIM_STEP);
int                        StripColorWidth = (int)(MeshSize * SO_STRIP_COLOR_WIDTH);
SoVertexProperty          *MeshVtxProp ;
SoDrawStyle               *DrawStyle ;
SoSwitch                  *InfoSwitch ;
SoIndexedTriangleStripSet *MeshShape ;
SoShapeHints              *ShapeHints ;
SoOneShotSensor           *OneShotSensor ;
uint32_t                   AnimIndex=0 ;
SbBool                     IsColorAnimation=TRUE ;
SbBool                     IsGeomAnimation=FALSE ;
SbBool                     IsTexAnimation=FALSE ;
SbBool                     IsVBO=TRUE ;
SbString                   Title(SO_TITLE_VBO) ;

SoXtExaminerViewer        *MyViewer ;
SoSeparator               *Scene3D ;
SoPerspectiveCamera       *Camera ;
SoBBox                    *g_boundingBoxNode;

long                       TimeColor, TimeVtx, TimeNormal, TimeTexCoord ;

bool                       fullDemo=true;
/*---------------------------------------------------------------------------*/

void
updateColors()
{
  SbTime t1 (SbTime::getTimeOfDay()) ;

  uint32_t *colors    = MeshVtxProp->orderedRGBA.startEditing() ;

  uint32_t defaultColor = SbColor(1,1,0).getPackedValue() ;
  uint32_t stripColor   = SbColor(0,0,1).getPackedValue() ;

  int minStripColor = AnimIndex ;
  int maxStripColor = AnimIndex + StripColorWidth ;
  if(maxStripColor >= MeshSize) {
    maxStripColor = MeshSize-1 ;
  }

  for(int i=0; i < MeshSize; i++) {
    int lineInd = i*MeshSize ;
    for(int j=0; j < MeshSize; j++) {
      if(j >= minStripColor && j <= maxStripColor)
        colors[lineInd+j] = stripColor ;
      else
        colors[lineInd+j] = defaultColor ;
    }
  }

  SbTime t2 (SbTime::getTimeOfDay()) ;
  TimeColor = (t2 - t1).getMsecValue() ;

  MeshVtxProp->orderedRGBA.finishEditing() ;

}/*---------------------------------------------------------------------------*/

void
updateTexCoords()
{
  SbTime t1 (SbTime::getTimeOfDay()) ;

  SbVec2f *texCoords    = MeshVtxProp->texCoord.startEditing() ;
  float animStep = (float)AnimIndex / (float) MeshSize ;

  for(int i=0; i < MeshSize; i++) {
    int lineInd = i*MeshSize ;
    for(int j=0; j < MeshSize; j++) {
      texCoords[lineInd+j].setValue((float)i/(float)MeshSize, (float)j/(float)MeshSize + animStep) ;
    }
  }

  SbTime t2 (SbTime::getTimeOfDay()) ;
  TimeTexCoord = (t2 - t1).getMsecValue() ;

  MeshVtxProp->texCoord.finishEditing() ;

}/*---------------------------------------------------------------------------*/

void
updateNormals()
{
  SbTime t1 (SbTime::getTimeOfDay()) ;

  SbVec3f *normals   = MeshVtxProp->normal.startEditing() ;
  const SbVec3f *vertices  = MeshVtxProp->vertex.getValues(0) ;

  float dx, dy ;

  for(int i=0; i < MeshSize; i++) {
    int lineInd  = i*MeshSize ;
    int line1Ind = (i+1)*MeshSize ;
    for(int j=0; j < MeshSize; j++) {
      dx = dy = 0.0f ;
      if(j+1 < MeshSize)
        dx = vertices[lineInd+j+1][2] - vertices[lineInd+j][2] ;
      
      if(i+1 < MeshSize)
        dy = vertices[line1Ind+j][2]  - vertices[lineInd+j][2] ;

      normals[lineInd+j].setValue(-dx, -dy, 1) ;
    }
  }

  SbTime t2 (SbTime::getTimeOfDay()) ;
  TimeNormal = (t2 - t1).getMsecValue() ;

  MeshVtxProp->normal.finishEditing() ;

}/*---------------------------------------------------------------------------*/

void
updateVertices()
{
  SbTime t1 (SbTime::getTimeOfDay()) ;

  SbVec3f *vertices  = (SbVec3f*) MeshVtxProp->vertex.startEditing() ;

  float coef = (float)MeshSize/SO_PERIOD ;
  int size = MeshSize*MeshSize ;
  
  for(int x=0; x < MeshSize; x++) {
    float sinAlpha = sin(((float)x+AnimIndex)/coef)*MeshAmplitude ;
    for(int i=0,y=0; i < size; i += MeshSize, y++)
      vertices[i+x].setValue(float(x), float(y), sinAlpha) ;
  }


  SbTime t2 (SbTime::getTimeOfDay()) ;
  TimeVtx = (t2 - t1).getMsecValue() ;

  MeshVtxProp->vertex.finishEditing() ;

  //update BBox node 
  g_boundingBoxNode->boundingBox.setValue( 0.f, 0.f, -1.f * float( MeshAmplitude ), 
            float( MeshSize ), float( size ) / float( MeshSize ), float( MeshAmplitude ) );

  updateNormals() ;

}/*---------------------------------------------------------------------------*/

void
buildMesh()
{
  NumTriangles=MeshSize*MeshSize*2 ;
  AnimStep = (int)(MeshSize * SO_ANIM_STEP) ;
  StripColorWidth = (int)(MeshSize * SO_STRIP_COLOR_WIDTH) ;
  MeshAmplitude=SO_DEFAULT_AMPLITUDE*((float)MeshSize/(float)SO_DEFAULT_MESH_SIZE) ;

  MeshVtxProp->materialBinding = SoVertexProperty::PER_VERTEX_INDEXED;
  MeshVtxProp->normal.setNum(MeshSize*MeshSize) ;
  MeshVtxProp->vertex.setNum(MeshSize*MeshSize) ;
  MeshVtxProp->orderedRGBA.setNum(MeshSize*MeshSize) ;
  MeshVtxProp->texCoord.setNum(MeshSize*MeshSize) ;

  MeshShape->coordIndex.setNum(MeshSize*MeshSize*2) ;

  int32_t *coordInd  = MeshShape->coordIndex.startEditing() ;
  
  int k=0;
  for(int i=0; i < MeshSize-1; i++) {
    int lineInd = i*MeshSize ;
    for(int j=0; j < MeshSize; j++) {
      coordInd[k++] = lineInd+j ;
      coordInd[k++] = lineInd+j+MeshSize ;
    }
    coordInd[k++] = -1 ;
  }

  MeshShape->coordIndex.setNum(k) ;
  MeshShape->coordIndex.finishEditing() ;

  updateColors() ;
  updateTexCoords() ;
  updateVertices() ;
}/*---------------------------------------------------------------------------*/

void
animateCB(void *, SoSensor *sensor) 
{
  if (fullDemo)
  {
    AnimIndex = (AnimIndex + AnimStep)%MeshSize ;
    if(IsGeomAnimation)
      updateVertices() ;
    if(IsColorAnimation)
      updateColors() ;
    if(IsTexAnimation)
      updateTexCoords() ;
    if (IsTexAnimation || IsColorAnimation || IsGeomAnimation)
      MeshShape->touch() ;
  }
  sensor->schedule() ;
}/*---------------------------------------------------------------------------*/

void
fpsCB(float fps, void *, SoXtViewer *viewer) {
  SbString titleStr(Title) ;
  SbString fpsStr((int)fps) ;
  SbString timeVtxStr(TimeVtx) ;
  SbString timeColorStr(TimeColor) ;
  SbString timeNormalStr(TimeNormal) ;
  SbString timeTexCoordStr(TimeTexCoord) ;

  titleStr += " : " ;
  titleStr += fpsStr ;
  titleStr += " fps" ;
  titleStr += " / " ;
  titleStr += NumTriangles ;
  titleStr += " triangles" ;
  titleStr += " / " ;
  titleStr += timeVtxStr ;
  titleStr += " ms (V)" ;
  titleStr += " / " ;
  titleStr += timeColorStr ;
  titleStr += " ms (C)" ;
  titleStr += " / " ;
  titleStr += timeNormalStr ;
  titleStr += " ms (N)" ;
  titleStr += " / " ;
  titleStr += timeTexCoordStr ;
  titleStr += " ms (T)" ;

  
  TimeColor = TimeVtx = TimeNormal = TimeTexCoord = 0 ;

  viewer->setTitle(titleStr.getString()) ;
}/*----------------------------------------------------------------------------*/

void 
myKeyPressCB (void *, SoEventCallback *eventCB) 
{
  const SoEvent *event = eventCB->getEvent();
  
  // Check for the keys being pressed
  if (SO_KEY_PRESS_EVENT(event, H)) { // Toggle help display
    if(InfoSwitch->whichChild.getValue() == SO_SWITCH_ALL)
      InfoSwitch->whichChild = SO_SWITCH_NONE ;
    else
      InfoSwitch->whichChild = SO_SWITCH_ALL ;   
  }

  else if(SO_KEY_PRESS_EVENT(event, F1)) { // Toggle VBO Versus normal mode
    IsVBO = (IsVBO ? FALSE : TRUE) ;
    if(IsVBO) {
      ShapeHints->useVBO = TRUE ;
      Title = SO_TITLE_VBO ;
    }
    else {
      ShapeHints->useVBO = FALSE ;
      Title = SO_TITLE_NORMAL ;
    }
  }

  else if(SO_KEY_PRESS_EVENT(event, F2)) { // Toggle wireframe/filled
    if(DrawStyle->style.getValue()==SoDrawStyle::FILLED)
      DrawStyle->style = SoDrawStyle::LINES ;
    else
      DrawStyle->style = SoDrawStyle::FILLED ;
  }

  else if(SO_KEY_PRESS_EVENT(event, F3)) { // Toggle geometry animation
    IsGeomAnimation = (IsGeomAnimation ? FALSE : TRUE) ;
  }

  else if(SO_KEY_PRESS_EVENT(event, F4)) { // Toggle color animation
    IsColorAnimation = (IsColorAnimation ? FALSE : TRUE) ;
  }

  else if(SO_KEY_PRESS_EVENT(event, F5)) { // Toggle texture coordinate  animation
    IsTexAnimation = (IsTexAnimation ? FALSE : TRUE) ;
  }

  else if(SO_KEY_PRESS_EVENT(event, PAD_ADD)) { // Increase mesh size
    MeshSize += SO_MESH_SIZE_STEP ;
    buildMesh() ;
    Camera->viewAll(Scene3D, MyViewer->getViewportRegion()) ;
  }

  else if(SO_KEY_PRESS_EVENT(event, PAD_SUBTRACT)) { // Decrease mesh size
    if(MeshSize - SO_MESH_SIZE_STEP > SO_MIN_MESH_SIZE) {
      MeshSize -= SO_MESH_SIZE_STEP ;
      buildMesh() ;
      Camera->viewAll(Scene3D, MyViewer->getViewportRegion()) ;
    }
  }
}/*----------------------------------------------------------------------------*/

SoSwitch*
displayInfo() 
{
  // Informations
  SoSwitch *infoSwitch = new SoSwitch ;
  infoSwitch->ref() ;
  infoSwitch->whichChild = SO_SWITCH_ALL ;
  
  SoSeparator *infoSep = new SoSeparator ;
  
  SoPickStyle *pickStyle = new SoPickStyle ;
  pickStyle->style = SoPickStyle::UNPICKABLE ;
  infoSep->addChild(pickStyle) ;
  
  infoSwitch->addChild(infoSep) ;
  
  SoLightModel *lModel = new SoLightModel ;
  lModel->model = SoLightModel::BASE_COLOR ;
  infoSep->addChild(lModel) ;
  
  SoFont *fontInfo = new SoFont ;
  fontInfo->name = "Courier New" ;
  fontInfo->size = 12 ;
  infoSep->addChild(fontInfo) ;
  
  SoBaseColor *infoColor = new SoBaseColor ;
  infoColor->rgb.setValue(SbColor(1, 1, 0.f)) ;
  infoSep->addChild(infoColor) ;
  
  SoTranslation *transInfo = new SoTranslation ;
  transInfo->translation.setValue(-0.95f, 0.95f, 0.) ;
  infoSep->addChild(transInfo) ;
  
  SoText2 *infoText = new SoText2 ;
  infoText->string.set1Value(0, "H   : Toggle this display") ;
  infoText->string.set1Value(1, "F1  : Toggle VBO Versus normal mode") ;
  infoText->string.set1Value(2, "F2  : Toggle wireframe/filled") ;
  infoText->string.set1Value(3, "F3  : Toggle geometry animation") ;
  infoText->string.set1Value(4, "F4  : Toggle color animation") ;
  infoText->string.set1Value(5, "F5  : Toggle texture animation") ;
  infoText->string.set1Value(6, "+/- : Increase/Decrease mesh refinement") ;
  infoSep->addChild(infoText) ;
  
  infoSwitch->unrefNoDelete() ;
  return infoSwitch ;
}/*---------------------------------------------------------------------------*/

int
main(int argc, char **argv)
{
#ifdef DVD_DEMO
#include <lock/demoUnlock.cxx>
#endif

  Widget myWindow = SoXt::init(argv[0]);
  if(myWindow == NULL) exit(1);

  fullDemo = !(argc >= 2 && argv[1] && strcmp(argv[1], "-noanim") == 0);

  SoDialogViz::init();

  SoSeparator *root = new SoSeparator;
  root->ref();

  SoMessageDialog *errorDialog = NULL ;
  
  if(!SoShapeHints::isVBOSupported()) {
    errorDialog = new SoMessageDialog ;
    errorDialog->title = "VBO Warning";
    errorDialog->type = SoMessageDialog::MD_WARNING;
    errorDialog->label = "VBO is not supported by your graphic board ! No performance gain !";
    errorDialog->show() ;
  }

  // Display Info
  InfoSwitch = displayInfo() ;
  root->addChild(InfoSwitch) ;

   // Track the keyboard events
  SoEventCallback *myEventCB = new SoEventCallback;
  myEventCB->addEventCallback(SoKeyboardEvent::getClassTypeId(), myKeyPressCB, NULL);
  root->addChild(myEventCB);

  SoTexture2 *texture2 = new SoTexture2 ;

  texture2->filename = "$OIVHOME/examples/source/Inventor/examples/data/Textures/MarbleTiles.jpg" ;

  // Because the 3D scene is potentially often modified.
  // No render cache should be created
  SoDB::setNumRenderCaches(0) ;

  Scene3D = new SoSeparator ;
  root->addChild(Scene3D) ;

  Camera = new SoPerspectiveCamera ;
  Scene3D->addChild(Camera) ;

  DrawStyle = new SoDrawStyle ;
  Scene3D->addChild(DrawStyle) ;

  // All subsequent shapes will use VBO
  ShapeHints = new SoShapeHints ;
  ShapeHints->useVBO = TRUE ;
  Scene3D->addChild(ShapeHints) ;
  Scene3D->addChild(texture2) ;

  // keep
  g_boundingBoxNode = new SoBBox;
  g_boundingBoxNode->mode.setValue(SoBBox::USER_DEFINED);
  Scene3D->addChild(g_boundingBoxNode);

  MeshVtxProp = new SoVertexProperty ;
  MeshShape = new SoIndexedTriangleStripSet ;

  buildMesh() ;

#ifdef SO_VEXTEX_PROPERTY_IN_STATE
  Scene3D->addChild(MeshVtxProp);
#else
  MeshShape->vertexProperty.setValue(MeshVtxProp) ;
#endif

  Scene3D->addChild(MeshShape) ;

  MyViewer = new SoXtExaminerViewer(myWindow);
  Camera->viewAll(Scene3D, MyViewer->getViewportRegion()) ;
  MyViewer->setSceneGraph(root);
  MyViewer->setBackgroundColor(SbColor(0.,0.,0.5));
  if (fullDemo) MyViewer->setFramesPerSecondCallback(fpsCB) ;
  MyViewer->setSize(SbVec2s(800,800)) ;
  MyViewer->setTitle(Title.getString());
  // Dynamic adjustment of camera clipping planes 
  // is desactivated because it is time consuming
  // to compute them with a not cached scene graph
  MyViewer->setAutoClipping(FALSE) ; 
  MyViewer->viewAll() ;
  
  MyViewer->show();

  OneShotSensor = new SoOneShotSensor(animateCB, NULL) ;
  OneShotSensor->schedule() ;

  SoXt::show(myWindow);
  SoXt::mainLoop();

  if(errorDialog)
    errorDialog->destroy() ;

  root->unref();
  delete OneShotSensor;
  delete MyViewer;
  SoDialogViz::finish();
  SoXt::finish();

  return 0;
}



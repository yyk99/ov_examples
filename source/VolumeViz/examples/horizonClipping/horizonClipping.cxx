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
** Author      : David Beilloin (Dec 2010)
**=======================================================================*/

#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#include <Inventor/Xt/SoXtDirectionalLightEditor.h>
#include <Inventor/Xt/SoXtRenderArea.h>

// DialogViz Interface
#include <DialogViz/SoDialogVizAll.h>
#include <DialogViz/dialog/SoTopLevelDialog.h>

#include <Inventor/helpers/SbFileHelper.h>
#include <Inventor/SoInput.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SoLists.h>

#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoInteractiveComplexity.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTextureUnit.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoTranslation.h>

// used for synthetic horizon generator
#include <Inventor/nodes/SoQuadMesh.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoTransform.h>

#include <Inventor/manips/SoJackManip.h>

#include <VolumeViz/nodes/SoVolumeData.h>
#include <VolumeViz/nodes/SoVolumeRendering.h>
#include <VolumeViz/nodes/SoVolumeRenderingQuality.h>
#include <VolumeViz/nodes/SoUniformGridProjectionClipping.h>
#include <VolumeViz/nodes/SoVolumeIndexedFaceSet.h>


#define DEMOROOT "$OIVHOME/examples/source/VolumeViz/examples/horizonClipping/"

/** Resolution of the geometry that will be generated */
#define SYNTHETIC_SIZE 100

/** Global root scene graph loaded from scenegraph.iv */
SoSeparator *g_rootSceneGraph = NULL;

/** create a wireframe representation of a SoVolumeData node */
SoSeparator *makeVolBBox( const SbBox3f &volSize );

/** setup the DialogViz interface */
Widget buildInterface(Widget window, const char* filename, const char* viewer, SoTopLevelDialog** myTopLevelDialog);
void releaseInterface();

/** helper to search a node by type */
template<typename T> T* searchNode(SoNode* scene);

/** helper to search a node by name */
template<typename T> T* searchName(SoNode* scene, SbName name);

/**
 * Generate a synthetic horizon based on the extent of a volumData
 */
SoNode* generateSyntheticHorizon(SoVolumeData* volumeData, const float zcenter, const float zscale, const SoUniformGridClipping::Axis axis)
{
    SbBox3f volumeBox = volumeData->extent.getValue();
    SoSeparator* shapeSep = new SoSeparator;

    // By default elevation is generated on Y axis
    // then change the index indirection to generate on correct axis
    SbVec3i32 axisOrder;
    switch (axis)
    {
    case SoUniformGridClipping::X: axisOrder = SbVec3i32(2, 0, 1); break;
    case SoUniformGridClipping::Y: axisOrder = SbVec3i32(0, 1, 2); break;
    case SoUniformGridClipping::Z: axisOrder = SbVec3i32(1, 2, 0); break;
    }

    // We generate a synthetic shape in range [-1,-1,-1] [1,1,1] range
    SoTransform* transform = new SoTransform;
    transform->translation = volumeBox.getCenter();
    transform->scaleFactor = volumeBox.getSize() / 2.0;

    shapeSep->addChild(transform);

    static bool once = true;
    if (once) {
        SoJackManip *jack = new SoJackManip();
        shapeSep->addChild(jack);
        once = false;
    }

    // setup two side lighting
    SoShapeHints *shapeHints = new SoShapeHints;
    shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapeSep->addChild(shapeHints);

    SoQuadMesh* IShape = new SoQuadMesh;
    SoVertexProperty* vp = new SoVertexProperty;
    IShape->vertexProperty.setValue(vp);

    // Define the resolution of the HeightMap
    float sizeX = SYNTHETIC_SIZE;
    float sizeY = SYNTHETIC_SIZE;

    // Define the range of the synthetic HeightMap
    SbVec3f vmin(-1, -1, -0.2f);
    SbVec3f vmax(1, 1, 0.2f);

    SbVec3f vrange(vmax - vmin);
    float deltaX = vrange[0] / sizeX;
    float deltaY = vrange[1] / sizeY;


    // setup vertices
    vp->vertex.setNum(int(sizeX*sizeY));
    SbVec3f* vertices = vp->vertex.startEditing();
    for (int vy = 0; vy < sizeY; ++vy)
    {
        for (int vx = 0; vx < sizeX; ++vx)
        {
            float elevation = sin(3 * ((float)vx / float(sizeX) + 4 * (float)(vy) / float(sizeY)));
            elevation /= (2.0f / vrange[2]);
            {
                float v0 = (*vertices)[axisOrder[0]] = (float)vmin[0] + vx*deltaX;
                float v1 = (*vertices)[axisOrder[2]] = (float)vmin[1] + vy*deltaY;

                v0 -= zcenter;
                v1 -= zcenter;

                if (sqrtf(v0*v0 + v1*v1) < 0.5f) {
                    (*vertices)[axisOrder[1]] = std::numeric_limits<float>::quiet_NaN();
                } else {
                    (*vertices)[axisOrder[1]] = (float)elevation*zscale + zcenter;
                }
            }
            ++vertices;
        }
    }
    vp->vertex.finishEditing();

    // setup indices
    IShape->verticesPerColumn = int(sizeX);
    IShape->verticesPerRow = int(sizeY);

    shapeSep->addChild(IShape);

    return shapeSep;
}

/**
 * Generate a synthetic horizon based on the extent of a volumData
 */
SoNode* generateSyntheticVolumegeometry(SoVolumeData* volumeData, const float zcenter, const float zscale, const SoUniformGridClipping::Axis axis)
{
  SbBox3f volumeBox = volumeData->extent.getValue();
  SoSeparator* shapeSep = new SoSeparator;

  // By default elevation is generated on Y axis
  // then change the index indirection to generate on correct axis
  SbVec3i32 axisOrder;
  switch (axis)
  {
  case SoUniformGridClipping::X : axisOrder = SbVec3i32(2,0,1); break;
  case SoUniformGridClipping::Y : axisOrder = SbVec3i32(0,1,2); break;
  case SoUniformGridClipping::Z : axisOrder = SbVec3i32(1,2,0); break;
  }

  // We generate a synthetic shape in range [-1,-1,-1] [1,1,1] range
  SoTransform* transform = new SoTransform;
  transform->translation = volumeBox.getCenter();
  transform->scaleFactor = volumeBox.getSize()/2.0;

  shapeSep->addChild(transform);

  // setup two side lighting
  SoShapeHints *shapeHints = new SoShapeHints;
  shapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
  shapeSep->addChild(shapeHints);

  SoVolumeIndexedFaceSet* IShape = new SoVolumeIndexedFaceSet;
  SoVertexProperty* vp = new SoVertexProperty;
  IShape->vertexProperty.setValue(vp);

  // Define the resolution of the HeightMap
  float sizeX=SYNTHETIC_SIZE;
  float sizeY=SYNTHETIC_SIZE;

  // Define the range of the synthetic HeightMap
  SbVec3f vmin(-1,-1,-0.2f);
  SbVec3f vmax( 1, 1, 0.2f);

  SbVec3f vrange(vmax-vmin);
  float deltaX =vrange[0]/sizeX;
  float deltaY =vrange[1]/sizeY;


  // setup vertices
  vp->vertex.setNum(int(sizeX*sizeY));
  SbVec3f* vertices = vp->vertex.startEditing();
  for (int vy=0;vy<sizeY;++vy)
  {
    for (int vx=0;vx<sizeX;++vx)
    {
      float elevation = sin( 3*((float)vx/float(sizeX) + 4*(float)(vy)/float(sizeY) ) );
      elevation/= (2.0f/vrange[2]);
      (*vertices)[axisOrder[0]] = (float)vmin[0]+vx*deltaX ;
      (*vertices)[axisOrder[1]] = (float)elevation*zscale + zcenter ;
      (*vertices)[axisOrder[2]] = (float)vmin[1]+vy*deltaY ;
      ++vertices;
    }
  }
  vp->vertex.finishEditing();

  // setup indices
  IShape->coordIndex.setNum(int( (sizeX-1)*(sizeY-1)*5));
  int32_t* indices = IShape->coordIndex.startEditing();
  for (int vy=0;vy<sizeY-1;++vy)
  {
    for (int vx=0;vx<sizeX-1;++vx)
    {
      indices[0] = int32_t((vy+0)*sizeY + (vx+0));
      indices[1] = int32_t((vy+0)*sizeY + (vx+1));
      indices[2] = int32_t((vy+1)*sizeY + (vx+1));
      indices[3] = int32_t((vy+1)*sizeY + (vx+0));
      indices[4] = -1; // endup the quad
      indices+=5;
    }
  }
  IShape->coordIndex.finishEditing();

  shapeSep->addChild(IShape);

  return shapeSep;
}

/**
 * load an horizon files and put it at the correct place in the scene graph
 */
void loadAndAddHorizon(const SbString& filename, const SbString& horizonSepName)
{
  SoSeparator* horizonSep = searchName<SoSeparator>(g_rootSceneGraph,horizonSepName.toLatin1());
  if ( horizonSep)
  {
    SoInput myInput;
    if ( myInput.openFile( filename, TRUE ) )
    {
      SoSeparator* horizon = SoDB::readAll(&myInput);
      horizonSep->addChild(horizon);
    }
    else
    {
      fprintf(stdout,"Unable to open horizon file %s\n",filename.toLatin1());
      fprintf(stdout,"Generating synthetic horizon...\n");
      
      // Generate a synthetic horizon for the given VolumeData node
      SoVolumeData* volumeData = searchNode<SoVolumeData>(g_rootSceneGraph);
      if (volumeData)
      {
        SoUniformGridClipping* clipingNode = searchNode<SoUniformGridClipping>(g_rootSceneGraph);
        SoUniformGridClipping::Axis axis = SoUniformGridClipping::Y;

        if ( clipingNode )
          axis = (SoUniformGridClipping::Axis)clipingNode->axis.getValue();

        // Increment the base to get 2 horizon at different position and
        // be able to do dual horizon clipping.
        static float zcenter = -0.25f;
        static float zscale  = 0.5f;
        horizonSep->addChild(generateSyntheticHorizon(volumeData,zcenter,zscale, axis));
        zcenter+=0.5f;
        zscale+=0.5f;
      }
    }
  }
}


int
main(int argc, char **argv)
{
  Widget mainWindow = SoXt::init(argv[0]);
  SoDialogViz::init();
  SoVolumeRendering::init();

  // Read the global scene graph
  {
    SoInput myInput;
    myInput.openFile( SbString(DEMOROOT) + "scenegraph.iv" );
    g_rootSceneGraph = SoDB::readAll(&myInput);
    g_rootSceneGraph->ref();
  }

  // load and add horizons from file if available
  loadAndAddHorizon(SbString(DEMOROOT) + "horizon1.iv", "Horizon1");
  loadAndAddHorizon(SbString(DEMOROOT) + "horizon2.iv", "Horizon2");

  // add a synthetic VolumeGeometry node in the representation mode
  {
    SoVolumeData* volumeData = searchNode<SoVolumeData>(g_rootSceneGraph);
    SoSwitch* renderSwitch = searchName<SoSwitch>(g_rootSceneGraph,"VolumeRenderingSwitch");
    if ( volumeData && renderSwitch )
    {
      SoSeparator * geomSep = new SoSeparator;
      geomSep->setName("VolumeGeometryRender");
      geomSep->addChild(generateSyntheticVolumegeometry(volumeData,0,1.0, SoUniformGridClipping::X));
      renderSwitch->addChild(geomSep);
    }
  }

  // Interface setup
  SbString InterfaceFile = SbString(DEMOROOT) + "interface.iv";
  SoTopLevelDialog *myTopLevelDialog;
  Widget parent = buildInterface(mainWindow, InterfaceFile.toLatin1(), "Viewer", &myTopLevelDialog);

  // Viewer setup
  SoXtExaminerViewer* volClippingViewer = new SoXtExaminerViewer(parent);
  volClippingViewer->setTitle("Volume with Horizon clipping");
  volClippingViewer->setSceneGraph(g_rootSceneGraph);
  volClippingViewer->setBackgroundColor(SbColor(0.f, 0.f, 0.f));
  volClippingViewer->setTransparencyType(SoGLRenderAction::DELAYED_BLEND);
  volClippingViewer->show();        
  volClippingViewer->viewAll(); 

  // Main Inventor event loop
  SoXt::show(mainWindow);
  SoXt::mainLoop();

  // release global object
  g_rootSceneGraph->unref();
  delete volClippingViewer;
  releaseInterface();

  // release modules
  SoVolumeRendering::finish();
  SoDialogViz::finish();
  SoXt::finish();

  return 0;
}


////////////////////// BEGIN INTERFACE MANAGEMENT ////////////////////////////

// DialogViz auditor class to handle user input
class myAuditorClass : public SoDialogAuditor
{
  virtual void dialogCheckBox (SoDialogCheckBox* cpt);
};

// Auditor method for checkBox input
void
myAuditorClass::dialogCheckBox(SoDialogCheckBox* cpt)
{
  // change current test
  if (cpt->auditorID.getValue() == "ShowHorizon")
  {
    SbBool value = cpt->state.getValue();
    SoSwitch* horizonSwitch = searchName<SoSwitch>(g_rootSceneGraph,"HorizonSwitch");
    if ( horizonSwitch )
      horizonSwitch->whichChild = (value==TRUE)?(-3):(-1);
  }
  // change current compute mode
  else if (cpt->auditorID.getValue() == "ShowTopology")
  {
    SbBool value = cpt->state.getValue();
    SoLDMGlobalResourceParameters::setVisualFeedbackParam(SoLDMGlobalResourceParameters::DRAW_TOPOLOGY, value);
  }
  // 
  else if (cpt->auditorID.getValue() == "ClippingMode")
  {
    SbBool value = cpt->state.getValue();
    SoSwitch* clippingSwitch = searchName<SoSwitch>(g_rootSceneGraph,"ClippingSwitch");
    if ( clippingSwitch )
      clippingSwitch->whichChild = (value==TRUE)?(-3):(1);
  }

}


myAuditorClass *g_myAuditor = NULL;
SoGroup *g_myGroup = NULL;

void
releaseInterface()
{
  delete g_myAuditor;
  SO_UNREF_RESET(g_myGroup);
}

Widget
buildInterface(Widget window, const char* filename, const char* viewer, SoTopLevelDialog** myTopLevelDialog)
{
  SoInput myInput;
  if (! myInput.openFile( filename ))
    return NULL;

  g_myGroup = SoDB::readAll( &myInput );
  if (! g_myGroup)
    return NULL;
  g_myGroup->ref();

  *myTopLevelDialog = (SoTopLevelDialog *)g_myGroup->getChild( 0 );

  SoDialogCustom *customNode = (SoDialogCustom *)(*myTopLevelDialog)->searchForAuditorId(SbString(viewer));

  // Create and register auditor to handle user input
  g_myAuditor = new myAuditorClass;
  (*myTopLevelDialog)->addAuditor(g_myAuditor);

  (*myTopLevelDialog)->buildDialog( window, customNode != NULL );
  (*myTopLevelDialog)->show();

  // Elevation axis detection
  SoUniformGridClipping* clipingNode = searchNode<SoUniformGridClipping>(g_rootSceneGraph);
  SoUniformGridClipping::Axis axis = SoUniformGridClipping::Y;
  if ( clipingNode )
    axis = (SoUniformGridClipping::Axis)clipingNode->axis.getValue();

  SoVolumeData* volumeData = searchNode<SoVolumeData>(g_rootSceneGraph);

  // Connect Thickness slider to the thickness clipping
  SoDialogRealSlider *thicknessSlider =(SoDialogRealSlider *)( (*myTopLevelDialog)->searchForAuditorId(SbString("Thickness")) );
  if ( thicknessSlider )
  {
    {
      SoUniformGridProjectionClipping* clippingNode =NULL;

      clippingNode= searchName<SoUniformGridProjectionClipping>(g_rootSceneGraph,"clip1");
      if ( clippingNode )
        clippingNode->thickness.connectFrom(&thicknessSlider->value);

      clippingNode= searchName<SoUniformGridProjectionClipping>(g_rootSceneGraph,"clip2");
      if ( clippingNode )
        clippingNode->thickness.connectFrom(&thicknessSlider->value);
    }
  }

  // Combobox to select which object to show
  SoDialogComboBox* renderModeCombo = (SoDialogComboBox*)( (*myTopLevelDialog)->searchForAuditorId(SbString("RenderMode")) );
  if ( renderModeCombo != NULL )
  {
    SoSwitch* renderSwitch = searchName<SoSwitch>(g_rootSceneGraph,"VolumeRenderingSwitch");
    if ( renderSwitch )
    {
      for ( int i = 0; i < renderSwitch->getNumChildren(); ++i)
      {
        SoNode* child = renderSwitch->getChild(i);
        renderModeCombo->items.set1Value(i, child->getName().getString());
      }
      renderSwitch->whichChild.connectFrom( &renderModeCombo->selectedItem );
    }
    else
      renderModeCombo->enable = FALSE;
  }

  return customNode ? customNode->getWidget() : window;
}

///////////////////////////////////////////////////////////////////////
// Create a wireframe box showing bounding box of a SoVolumeData node
///////////////////////////////////////////////////////////////////////

#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoPickStyle.h>

SoSeparator *makeVolBBox( const SbBox3f &volSize )
{
  SoTransform* transform = new SoTransform;
  transform->translation = volSize.getCenter();
  transform->scaleFactor = volSize.getSize()/2.0f;

  // The box will be easier to see without lighting
  SoLightModel *pLModel = new SoLightModel;
  pLModel->model = SoLightModel::BASE_COLOR;

  // The box should be unpickable so manip can be used inside it
  SoPickStyle *pPickable = new SoPickStyle;
  pPickable->style = SoPickStyle::UNPICKABLE;

  SoDrawStyle* cubeStyle = new SoDrawStyle;
  cubeStyle->style = SoDrawStyle::LINES;

  SoCube *pCube = new SoCube;

  SoSeparator *pBoxSep = new SoSeparator;
  pBoxSep->addChild(transform);
  pBoxSep->addChild( pLModel );
  pBoxSep->addChild( pPickable );
  pBoxSep->addChild( cubeStyle );
  pBoxSep->addChild( pCube );
  return pBoxSep;
}

// search a node by type
template<typename T>
T* 
searchNode(SoNode* scene)
{
  T* ret=NULL;
  SoSearchAction* sa = new SoSearchAction;
  sa->setFind(SoSearchAction::TYPE);
  sa->setSearchingAll(true);
  sa->setType(T::getClassTypeId());
  sa->apply(scene);

  SoPath* path = sa->getPath();
  if ( !path )
  {
    std::cerr << T::getClassTypeId().getName().getString() << " not found" << std::endl;
    ret = NULL;
  }
  else
  {
    ret = dynamic_cast<T*>(path->getTail());
  }
  delete sa;
  return ret;
}

// search a node by name
template<typename T>
T* 
searchName(SoNode* scene, SbName name)
{
  T* ret = NULL;
  if (!scene)
  {
    T* node=(T*)SoNode::getByName(name);
    if ( node )
      return node;
  }
  else
  {
    SoSearchAction* sa = new SoSearchAction;
    sa->setFind(SoSearchAction::NAME);
    sa->setSearchingAll(true);
    sa->setType(T::getClassTypeId());
    sa->setName(name);
    sa->apply(scene);

    SoPath* path = sa->getPath();
    if ( path )
      ret = dynamic_cast<T*>(path->getTail());
    delete sa;
  }
  if (ret==NULL)
    std::cerr << T::getClassTypeId().getName().getString() << " not found" << std::endl;

  return ret;
}



/*----------------------------------------------------------------------------------------
Example program.
Purpose : Demonstrate how to create and use a simple grid clipping node
March 2009
----------------------------------------------------------------------------------------*/

//header files
#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTransformSeparator.h>
#include <Inventor/nodes/SoTextureUnit.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/manips/SoJackManip.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include <VolumeViz/nodes/SoVolumeData.h>
#include <VolumeViz/nodes/SoVolumeRender.h>
#include <VolumeViz/nodes/SoUniformGridClipping.h>
#include <LDM/nodes/SoDataRange.h>
#include <LDM/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeRenderingQuality.h>
#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <Inventor/helpers/SbFileHelper.h>

#include <vector>

SoSeparator* generateGeometry(SoUniformGridClipping* grid);

int main(int, char **argv)
{
  Widget myWindow = SoXt::init(argv[0]);
  if (!myWindow)
    return 0;

  SoVolumeRendering::init();

  // Node to hold the volume data
  SoVolumeData* pVolData = new SoVolumeData();
  pVolData->fileName =  "$OIVHOME/examples/source/VolumeViz/Data/COLT.VOL";

  // If necessary, specify the actual range of the data values.
  //    By default VolumeViz maps the entire range of the voxel data type
  //    (e.g. 0..65535 for unsigned short) into the colormap.  This works
  //    great for byte (8 bit) voxels, but not so well for 16 bit voxels
  //    and not at all for floating point voxels. So it's not actually
  //    necessary for this data set, but shown here for completeness.
  //    NOTE: Min/max values are stored in the header for LDM format
  //    files, but for other formats the getMinMax query can take a
  //    long time because VolumeViz has to examine every voxel.
  SoDataRange *pRange = new SoDataRange();
  int voxelSize = pVolData->getDataSize();
  if (voxelSize > 1) {
    double minval, maxval;
    pVolData->getMinMax( minval, maxval );
    pRange->min = minval;
    pRange->max = maxval;
  }

  // Use a predefined colorMap with the SoTransferFunction
  SoTransferFunction* pTransFunc = new SoTransferFunction;
  pTransFunc->predefColorMap = SoTransferFunction::SEISMIC;
  pTransFunc->minValue = 10;
  pTransFunc->maxValue = 250;

  // Node in charge of drawing the volume
  SoVolumeRender* pVolRender = new SoVolumeRender;
  pVolRender->samplingAlignment = SoVolumeRender::VIEW_ALIGNED;
  pVolRender->numSlicesControl = SoVolumeRender::MANUAL;
  pVolRender->numSlices = 200;
  pVolRender->lowResMode = SoVolumeRender::DECREASE_SCREEN_RESOLUTION;
  pVolRender->lowScreenResolutionScale = 2;

  //Add a SoVolumeRenderingQuality to make it nicer
  SoVolumeRenderingQuality* vrq = new SoVolumeRenderingQuality;
  vrq->preIntegrated = TRUE;
  vrq->jittering = true;

  SoPickStyle* pickStyle = new SoPickStyle;
  pickStyle->style = SoPickStyle::UNPICKABLE;

  //The grid will be on the Y axis
  SoUniformGridClipping* gridCliping = new SoUniformGridClipping;
  gridCliping->filename = "$OIVHOME/examples/source/VolumeViz/examples/simpleGridClipping/clipping.png";
  gridCliping->axis = SoUniformGridClipping::Y;

  //Create an indexed face set using height values of the clipping grid
  SoSeparator* terrain = generateGeometry(gridCliping);

  //Add a manipulator on geometry
  SoSeparator* geom = new SoSeparator;
  SoJackManip* jackManip = new SoJackManip;
  geom->addChild(jackManip);
  geom->addChild(terrain);

  //Compute geometry's bounding box
  geom->ref();
  SbViewportRegion vp;
  SoGetBoundingBoxAction bba(vp);
  bba.apply(terrain);
  geom->unrefNoDelete();

  //Give the clipping grid extent smae value as geometry's bounding box
  gridCliping->extent = bba.getBoundingBox();
  pVolData->extent = bba.getBoundingBox();

  //Put the clipping grid in texture unit 3
  SoTextureUnit* tu3 = new SoTextureUnit;
  tu3->unit = 3;

  SoTextureUnit* tu0 = new SoTextureUnit;
  tu0->unit = 0;

  //Transform separator to manipulate the clipping plane freely
  SoTransformSeparator* sepClipping = new SoTransformSeparator;
  sepClipping->addChild( jackManip );
  sepClipping->addChild( tu3 );
  sepClipping->addChild( gridCliping );
  sepClipping->addChild( tu0 );

  // Assemble the scene graph
  SoSeparator *root = new SoSeparator;
  root->ref();
  root->addChild( geom );
  root->addChild( sepClipping );
  root->addChild( pVolData );
  root->addChild( pRange   );
  root->addChild( pTransFunc );
  root->addChild( vrq );
  root->addChild( pickStyle );
  root->addChild( pVolRender );

  // Set up viewer:
  SoXtExaminerViewer *myViewer = new SoXtExaminerViewer(myWindow);
  myViewer->setSceneGraph(root);
  myViewer->setTitle("Volume rendering");
  myViewer->show();

  SoXt::show(myWindow);
  SoXt::mainLoop();
  delete myViewer;
  root->unref();
  SoVolumeRendering::finish();
  SoXt::finish();

  return 0;
}


//Create an indexed face set using height values of the clipping grid
SoSeparator*
generateGeometry(SoUniformGridClipping* grid)
{
  SbVec2i32 imgSize;
  int ImgNC;
  const unsigned char* myImageBuff = grid->image.getValue(imgSize,ImgNC);

  SbVec2f minE = SbVec2f(-1, -1);
  SbVec2f maxE = SbVec2f(1, 1);

  std::vector<SbVec3f> vert;
  SbVec3f p(minE[0], 0, maxE[1]);
  SbVec2f dxDy = SbVec2f((maxE[0]-minE[0])/(imgSize[0]-1),
                         (maxE[1]-minE[1])/(imgSize[1]-1));
  for ( int j = 0; j < imgSize[1]; j++ )
  {
    p[0] = minE[0];
    for ( int i = 0; i < imgSize[0]; i++ )
    {
      p[1] = myImageBuff[ImgNC*(i+j*imgSize[0])]/255.f-0.5f;
      vert.push_back(p);
      p[0] += dxDy[0];
    }
    p[2] -= dxDy[1];
  }

  std::vector<int> indices;
  for ( int j = 0; j < imgSize[1]-1; j++ )
  {
    for ( int i = 0; i < imgSize[0]-1; i++ )
    {
      indices.push_back(i+(j+1)*imgSize[0]);
      indices.push_back(i+j*imgSize[0]);
    }
    indices.push_back(SO_END_STRIP_INDEX);
  }


  SoIndexedTriangleStripSet* ifs = new SoIndexedTriangleStripSet;
  ifs->coordIndex.setNum(static_cast<int>(indices.size()));
  ifs->coordIndex.setValues(0, static_cast<int>(indices.size()), &indices[0]);

  SoVertexProperty* vp = new SoVertexProperty;
  ifs->vertexProperty = vp;
  vp->vertex.setNum(static_cast<int>(vert.size()));
  vp->vertex.setValues(0, static_cast<int>(vert.size()), &vert[0]);

  SoSeparator* sep =new SoSeparator;
  sep->addChild(ifs);

  return sep;
}



#include "BlobFinder.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/DescImage.hh"
#include "ami/data/Entry.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/EntryFactory.hh"

#include "ami/data/Cds.hh"
#include "ami/data/Expression.hh"

#include <stdio.h>

//
//  Martin Beye's code starts here
//
namespace Ami {
  namespace BlobFinding {

    template <class T>
    class Node
    {
    public:
      Node() {myObject = 0; myNext = 0;}
      Node(T * theObject): myObject(theObject) {myNext = 0;}
      Node(T * theObject, Node<T> * theNext): myObject(theObject), myNext(theNext\
                                                                          ) {}
      ~Node() {if (myNext != 0) delete myNext; if (myObject != 0) delete myObject\
                                                 ; myNext = 0; myObject = 0;}
      Node<T> *  Insert (T * theObject)
      {
        if (myObject != 0) return new Node<T> (theObject, this);
        else {myObject = theObject; return this;}
      }
      Node<T> * GetNext () const {return myNext;}
      T * GetThingPointer() const {return myObject;}

    private:
      T * myObject;
      Node<T> * myNext;
    };

    template <class T>
    class LinkedList
    {
    public:
      LinkedList() {myFirst = new Node<T>();}
      LinkedList(T * theObject) {myFirst = new Node<T>(theObject);}
      ~LinkedList() {delete myFirst; myFirst = 0;}
      void Insert (T * theObject)  {myFirst = myFirst->Insert (theObject);}
      int GetNumberOfObjects ();
      T * GetThing (int objectIndex);

    private:
      Node<T> * myFirst;
    };

    class Position
    {
    public:
      Position (float xPosition = 0, float yPosition = 0): myXPosition(xPosition), myYPosition(yPosition) {}
      Position (Position * thePosition): myXPosition( thePosition->GetXPosition() ), myYPosition( thePosition->GetYPosition() ) {}
      ~Position () {}
      float GetXPosition () const {return myXPosition;}
      float GetYPosition () const {return myYPosition;}
      void SetXPosition (float xPosition) {myXPosition = xPosition;}
      void SetYPosition (float yPosition) {myYPosition = yPosition;}

    private:
      float myXPosition, myYPosition;
    };

    class BlobFindingMain;

    class Pixel
    {
    public:
      Pixel (int position, u_short zvalue): myPosition(position), myNeighboursChecked(false), myZValue(zvalue) {}
      ~Pixel () { }
      u_short GetZValue () const {return myZValue;}
      int GetPosition () const {return myPosition;}
      bool CheckAllNeighbours (LinkedList<Pixel> & pixelList, BlobFindingMain&);
      bool GetNeighboursChecked () {return myNeighboursChecked;}

    private:
      int myPosition;
      bool myNeighboursChecked;
      u_short myZValue;

      bool CheckNeighbour (LinkedList<Pixel> & pixelList, int anotherPosition, BlobFindingMain&);
    };

    class Blob
    {
    public:
      Blob (int firstPixel, BlobFindingMain& finder);
      ~Blob () {delete myPosition;}
      Position * GetPosition ();
      int FindArea();
      int GetArea();
      void RecreateArea();

    private:
      int myArea;
      bool myAreaFound;
      LinkedList<Pixel> myPixelList;
      Position * myPosition;
      BlobFindingMain& _finder;
    };

    class BlobFindingMain
    {
    public:
      BlobFindingMain (uint16_t* spectrum,
                       unsigned  width,
                       unsigned  height,
                       unsigned  threshold,
                       unsigned  cluster_size) :
        _spectrum    (spectrum),
        _width       (width),
        _height      (height),
        _threshold   (threshold),
        _cluster_size(cluster_size)
      {}
      ~BlobFindingMain () {}

      int GetBlobPositions (LinkedList<Position>*, bool recreateArea=false);

    private:
      int* GetBlobPositionsPart (LinkedList<Position>*, int startPixel, int endPixel, bool recreateArea);
    public:
      uint16_t* _spectrum;
      unsigned  _width;
      unsigned  _height;
      unsigned  _threshold;
      unsigned  _cluster_size;
      LinkedList<Position> _blobs;
    };
  };
};

using namespace Ami::BlobFinding;

template <class T>
int LinkedList<T>::GetNumberOfObjects()
{
    Node<T> * item = myFirst;
    int i=0;

    if (item->GetThingPointer() == 0) return 0;
    while (item != 0)
    {
        item = item->GetNext();
	++i;
    }
    return i;
}

template <class T>
T * LinkedList<T>::GetThing (int objectIndex)
{
    Node<T> * item = myFirst;

    int i = 0;
    while (i < objectIndex)
    {
        item = item->GetNext();
	++i;
    }
    return item->GetThingPointer();
}

bool Pixel::CheckAllNeighbours (LinkedList<Pixel> & pixelList, BlobFindingMain& finder)
{
  uint16_t* spectrum = finder._spectrum;
  int       width    = finder._width;
  int       height   = finder._height;

  bool haveFoundOtherBlobPixels = false;

  // Set pixel in image to 0, so that it will not be found twice, store value before
  spectrum[myPosition] =  0;

  //Check above
  if (myPosition < width) ;
  else
    {
      haveFoundOtherBlobPixels |= CheckNeighbour(pixelList, myPosition - width, finder );
    }

  //Check below
  if (myPosition >= (height-1) * width) ;
  else
    {
      haveFoundOtherBlobPixels |= CheckNeighbour(pixelList, myPosition + width, finder );
    }

  //Check left
  if ((myPosition % width)== 0) ;
  else
    {
      haveFoundOtherBlobPixels |= CheckNeighbour(pixelList, myPosition - 1, finder);
    }

  //Check right
  if ( (myPosition % width) == (width -1)) ;
  else
    {
      haveFoundOtherBlobPixels |= CheckNeighbour(pixelList, myPosition + 1, finder);
    }

  myNeighboursChecked = true;
  return haveFoundOtherBlobPixels;
}

//just a help function, for not writing the same code for every check
bool Pixel::CheckNeighbour (LinkedList<Pixel> & pixelList, int anotherPosition, BlobFindingMain& finder)
{
  if (finder._spectrum[anotherPosition] > finder._threshold )
    {
      Pixel * anotherBlobPixel = new Pixel(anotherPosition, finder._spectrum[anotherPosition]);
      pixelList.Insert(anotherBlobPixel);
      finder._spectrum[anotherPosition] =  0;
      return true;
    }
  else return false;
}

Blob::Blob (int firstPixel, BlobFindingMain& finder): 
  myArea(1), 
  myAreaFound(false), 
  myPixelList(new Pixel(firstPixel,finder._spectrum[firstPixel])), 
  myPosition(new Position()), 
  _finder(finder) 
{ _finder._spectrum[firstPixel] = 0; }

//Function for restoring image as pixels in a blob are set to zero
void Blob::RecreateArea()
{
  Pixel * pixelPointer;

  if (!myAreaFound) FindArea();
  for (int i = 0; i < myArea; ++i)
    {
      pixelPointer = myPixelList.GetThing(i);
      _finder._spectrum[ pixelPointer->GetPosition() ] = pixelPointer->GetZValue();
    }
}

//function that takes the pixellist generated by FindArea() and computes the center of mass
Position * Blob::GetPosition()
{
  unsigned width = _finder._width;

  float x = 0, y = 0, integratedArea = 0;
  Pixel * pixelPointer;
  int pixelPosition;
  float z;

  if (!myAreaFound) FindArea();
  for (int i = 0; i < myArea; ++i)
    {
      pixelPointer = myPixelList.GetThing(i);
      pixelPosition = pixelPointer->GetPosition();
      z = pixelPointer->GetZValue();
      integratedArea += z;
      x += (pixelPosition % width) * z;
      y += (static_cast<int>(pixelPosition / width )) * z;
    }
  myPosition->SetXPosition(x / integratedArea);
  myPosition->SetYPosition(y / integratedArea);
  return myPosition;
}

//function that does a lot of work in BlobFinding:
//scans the pixellist and checks, if every pixel has checked its neighbourhood
//if not, it makes the pixel check its neighbourhood and restarts with scanning the pixellist
//(if there are new pixels inserted)
int Blob::FindArea()
{
  while (!myAreaFound)
    {
      Pixel * pixelPointer;

      myAreaFound = true;
      myArea = myPixelList.GetNumberOfObjects();
      for (int i = 0; i < myArea; ++i)
        {
          pixelPointer = myPixelList.GetThing(i);
          if (!pixelPointer->GetNeighboursChecked())
            {
              if ( pixelPointer->CheckAllNeighbours(myPixelList, _finder) ) //CheckAllNeighbours is true, if other blob-pixels are found
                {
                  myAreaFound = false;
                  break;
                }
            }
        }
    }
  myArea = myPixelList.GetNumberOfObjects();
  return myArea;
}

//Just returns the number of pixels in a blob
int Blob::GetArea()
{
  if (!myAreaFound) FindArea();
  return myArea;
}
int BlobFindingMain::GetBlobPositions(LinkedList<Position> * myBlobCenterList, bool recreateArea)
{
  //here starts variable definition for BlobFinding
  int firstHalfBeginning = 0,
    firstHalfEnd = (static_cast<int> (_height / 2)) * _width -1,
    bufferBeginning = firstHalfEnd + 1,
    bufferEnd = firstHalfEnd + _width,
    secondHalfBeginning = bufferEnd + 1,
    secondHalfEnd = _width * _height,
    ret;
  
  int * nrOfBufferBlobs;
  int * nrOfBlobsOne;
  int * nrOfBlobsTwo;
  LinkedList<Position> *  list_one = new LinkedList<Position>;
  LinkedList<Position> *  list_two = new LinkedList<Position>;

  nrOfBufferBlobs = GetBlobPositionsPart (myBlobCenterList, bufferBeginning, bufferEnd, recreateArea);
  nrOfBlobsOne = GetBlobPositionsPart (list_one, firstHalfBeginning, firstHalfEnd, recreateArea);
  nrOfBlobsTwo = GetBlobPositionsPart (list_two, secondHalfBeginning, secondHalfEnd, recreateArea);

  for (int i = 0; i < *nrOfBlobsOne; ++i)
    {
      Position * pos = new Position ( list_one->GetThing(i) );
      myBlobCenterList->Insert( pos );
    }
  for (int j = 0; j < *nrOfBlobsTwo; ++j)
    {
      Position * pos = new Position ( list_two->GetThing(j) );
      myBlobCenterList->Insert( pos );
    }
  delete list_one;  list_one = 0;
  delete list_two;  list_two = 0;
  ret = *nrOfBufferBlobs + *nrOfBlobsOne + *nrOfBlobsTwo;
  delete nrOfBufferBlobs;  nrOfBufferBlobs = 0;
  delete nrOfBlobsOne;  nrOfBlobsOne = 0;
  delete nrOfBlobsTwo;  nrOfBlobsTwo = 0;

  return ret;
}

int* BlobFindingMain::GetBlobPositionsPart(LinkedList<Position>* myBlobCenterList, int startPixel, int endPixel, bool recreateArea)
{
  LinkedList<Blob>* myBlobList = new LinkedList<Blob>;
  int lastPixel = _width*_height;
  int * nrOfBigBlobs = new int(0);

  if (endPixel > lastPixel) endPixel = lastPixel;

  while ( startPixel < endPixel )
    {
      for (unsigned i = startPixel+1 ; i < (startPixel + _width - 1) ; i+=2 )
        {
          if (_spectrum[i] > _threshold )
            {
              if (_spectrum[i+1] > _threshold)
                {
                  Blob * blobPointer = new Blob(i, *this);
                  blobPointer->FindArea();
                  myBlobList->Insert(blobPointer);
                }
            }
        }
      startPixel += (2*_width);
    }

  int nrOfBlobs = myBlobList->GetNumberOfObjects();
  for (int j = 0; j < nrOfBlobs; ++j)
    {
      Blob * blobPointer = myBlobList->GetThing(j);
      if (blobPointer->GetArea() >= int(_cluster_size) )
        {
          ++(*nrOfBigBlobs);
          Position * pos = new Position( blobPointer->GetPosition() );
          myBlobCenterList->Insert( pos );
        }
      if (recreateArea) blobPointer->RecreateArea();
    }

  delete myBlobList;
  myBlobList = 0;

  return nrOfBigBlobs;
}
//
//  Martin Beye's code ends here
//


using namespace Ami;

BlobFinder::BlobFinder(unsigned roi_top,
                       unsigned roi_bottom,
                       unsigned roi_left,
                       unsigned roi_right,
                       unsigned threshold,
                       unsigned cluster_size,
                       bool   accumulate) :
  AbsOperator(AbsOperator::BlobFinder),
  _roi_top     (roi_top),
  _roi_bottom  (roi_bottom),
  _roi_left    (roi_left),
  _roi_right   (roi_right),
  _threshold   (threshold),
  _cluster_size(cluster_size),
  _accumulate  (accumulate),
  _spectrum    (0),
  _output_entry(0)
{
}

#define CASETERM(type) case DescEntry::type:                            \
  t = new Entry##type##Term(static_cast<const Entry##type&>(*entry),_index); break;

static const char* _advance(const char*& p, unsigned size) { const char* o=p; p+=size; return o; }
#define EXTRACT(p, type) *(reinterpret_cast<const type*>(_advance(p,sizeof(type))))

BlobFinder::BlobFinder(const char*& p) :
  AbsOperator  (AbsOperator::BlobFinder),
  _roi_top     (EXTRACT(p, uint16_t)),
  _roi_bottom  (EXTRACT(p, uint16_t)),
  _roi_left    (EXTRACT(p, uint16_t)),
  _roi_right   (EXTRACT(p, uint16_t)),
  _threshold   (EXTRACT(p, uint16_t)),
  _cluster_size(EXTRACT(p, uint16_t)),
  _accumulate  (EXTRACT(p, uint32_t)),
  _spectrum    (0),
  _output_entry(0)
{
}

BlobFinder::BlobFinder(const char*& p, const DescEntry& e) :
  AbsOperator  (AbsOperator::BlobFinder),
  _roi_top     (EXTRACT(p, uint16_t)),
  _roi_bottom  (EXTRACT(p, uint16_t)),
  _roi_left    (EXTRACT(p, uint16_t)),
  _roi_right   (EXTRACT(p, uint16_t)),
  _threshold   (EXTRACT(p, uint16_t)),
  _cluster_size(EXTRACT(p, uint16_t)),
  _accumulate  (EXTRACT(p, uint32_t))
{
  unsigned nbinsx = _roi_right  - _roi_left + 1;
  unsigned nbinsy = _roi_bottom - _roi_top  + 1;
  int ppxbin = static_cast<const DescImage&>(e).ppxbin();
  int ppybin = static_cast<const DescImage&>(e).ppybin();
  DescImage desc(e.name(),nbinsx,nbinsy,ppxbin,ppybin);
  _output_entry = static_cast<EntryImage*>(EntryFactory::entry(desc));
  _output_entry->info(0,EntryImage::Pedestal);
  int ppbin = ppxbin*ppybin;
  _threshold    *= ppbin;
  _cluster_size /= ppbin;
  if (_cluster_size<=0)
    _cluster_size=1;
  _spectrum = new uint16_t[nbinsx*nbinsy];
}

BlobFinder::~BlobFinder()
{
  if (_output_entry)
    delete _output_entry;
  if (_spectrum)
    delete[] _spectrum;
}

DescEntry& BlobFinder::_routput   () const { return _output_entry->desc(); }

void*      BlobFinder::_serialize(void* p) const
{
  _insert(p, &_roi_top     , sizeof(_roi_top     ));
  _insert(p, &_roi_bottom  , sizeof(_roi_bottom  ));
  _insert(p, &_roi_left    , sizeof(_roi_left    ));
  _insert(p, &_roi_right   , sizeof(_roi_right   ));
  _insert(p, &_threshold   , sizeof(_threshold   ));
  _insert(p, &_cluster_size, sizeof(_cluster_size));
  _insert(p, &_accumulate  , sizeof(_accumulate));
  return p;
}

Entry&     BlobFinder::_operate(const Entry& e) const
{
  if (e.valid()) {

    const EntryImage& entry = static_cast<const EntryImage&>(e);
    const DescImage& d = _output_entry->desc();
    const unsigned nx = d.nbinsx();
    const unsigned ny = d.nbinsy();
    const unsigned q  = d.ppxbin()*d.ppybin();
    if (!_accumulate)
      memset(_output_entry->contents(), 0, sizeof(unsigned)*nx*ny);

    //  Find the blobs

    //  collapse into a 1d array
    unsigned p = entry.info(EntryImage::Pedestal);
    int k=0;
    for(int i=_roi_top; i<= _roi_bottom; i++)
      for(int j=_roi_left; j<= _roi_right; j++) {
        if (entry.content(j,i) > p)
          _spectrum[k++] = entry.content(j,i) - p;
        else
          _spectrum[k++] = 0;
      }

    Ami::BlobFinding::BlobFindingMain finder(_spectrum,
                                             _roi_right-_roi_left+1,
                                             _roi_bottom-_roi_top+1,
                                             _threshold,
                                             _cluster_size);

    Ami::BlobFinding::LinkedList<Position> * bloblist = new Ami::BlobFinding::LinkedList<Position>;
    int numberofblobs = finder.GetBlobPositions(bloblist);
    for(int i=0; i<numberofblobs; i++) {
      Ami::BlobFinding::Position* p = bloblist->GetThing(i);
      _output_entry->addcontent(q,int(p->GetXPosition()+0.5),int(p->GetYPosition()+0.5));
    }

    delete bloblist;
  }

  _output_entry->valid(e.time());
  return *_output_entry;
}


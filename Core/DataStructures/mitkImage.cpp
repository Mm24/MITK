#include "mitkImage.h"
#include <vtkImageData.h>

//##ModelId=3DCBC2B50345
const mitk::PixelType& mitk::Image::GetType(int n) const
{
	return m_PixelType;
}

//##ModelId=3DCBC5AA0112
unsigned int mitk::Image::GetDimension() const
{
	return m_Dimension;
}

//##ModelId=3DCBC6040068
unsigned int mitk::Image::GetDimension(int i) const
{
	if((i>=0) && (i<m_Dimension))
		return m_Dimensions[i];
	return -1;
}

//##ModelId=3E0B494802D6
void* mitk::Image::GetData()
{
	if(m_Initialized==false)
	{
		if(GetSource()==NULL)
			return NULL;
		GetSource()->UpdateOutputInformation();
	}
	m_CompleteData=GetChannelData();
	return m_CompleteData->GetData();
}

//##ModelId=3DCBEF2902C6
vtkImageData* mitk::Image::GetVtkImageData()
{
	if(m_Initialized==false)
	{
		if(GetSource()==NULL)
			return NULL;
		GetSource()->UpdateOutputInformation();
	}
	m_CompleteData=GetChannelData();
	if(m_CompleteData==NULL) 
		return NULL;
	float* spacing = const_cast<float*>(GetGeometry()->GetSpacing());
	m_CompleteData->GetVtkImageData()->SetSpacing(spacing);
	return m_CompleteData->GetVtkImageData();
}

//##ModelId=3DCBE2B802E4
ipPicDescriptor* mitk::Image::GetPic()
{
	if(m_Initialized==false)
	{
		if(GetSource()==NULL)
			return NULL;
		GetSource()->UpdateOutputInformation();
	}
	m_CompleteData=GetChannelData();
	if(m_CompleteData==NULL) 
		return NULL;
	return m_CompleteData->GetPicDescriptor();
}

//##ModelId=3E0B4A6A01EC
mitk::ImageDataItem::Pointer mitk::Image::GetSliceData(int s, int t, int n)
{
	if(IsValidSlice(s,t,n)==false) return NULL;

	// slice directly available?
	int pos=GetSliceIndex(s,t,n);
	if(m_Slices[pos]!=NULL)
		return m_Slices[pos];

	// is slice available as part of a volume that is available?
	ImageDataItemPointer sl, ch, vol;
	vol=m_Volumes[GetVolumeIndex(t,n)];
	if((vol!=NULL) && (vol->IsComplete()))
	{
		sl=new ImageDataItem(*vol, 2, s*m_OffsetTable[2]*m_PixelType.GetBpe()/8);
		sl->SetComplete(true);
		return m_Slices[pos]=sl;
	}

	// is slice available as part of a channel that is available?
	ch=m_Channels[n];
	if((ch!=NULL) && (ch->IsComplete()))
	{
		sl=new ImageDataItem(*ch, 2, (s*m_OffsetTable[2]+t*m_OffsetTable[3])*m_PixelType.GetBpe()/8);
		sl->SetComplete(true);
		return m_Slices[pos]=sl;
	}

	// slice is unavailable. Can we calculate it?
	if(GetSource()!=NULL)
	{
		// ... wir m�ssen rechnen!!! ....
		m_RequestedRegion.SetIndex(0, 0);
		m_RequestedRegion.SetIndex(1, 0);
		m_RequestedRegion.SetIndex(2, s);
		m_RequestedRegion.SetIndex(3, t);
		m_RequestedRegion.SetIndex(4, n);
		m_RequestedRegion.SetSize(0, m_Dimensions[0]);
		m_RequestedRegion.SetSize(1, m_Dimensions[1]);
		m_RequestedRegion.SetSize(2, 1);
		m_RequestedRegion.SetSize(3, 1);
		m_RequestedRegion.SetSize(4, 1);
		m_RequestedRegionInitialized=true;
		GetSource()->Update();
		if(IsSliceSet(s,t,n))
			//yes: now we can call ourselves without the risk of a endless loop (see if above)
			return GetSliceData(s,t,n);
		else
			return NULL;
	}
	else
	{
		return AllocateSliceData(s,t,n);
	}
}

//##ModelId=3E0B4A82001A
mitk::ImageDataItem::Pointer mitk::Image::GetVolumeData(int t, int n)
{
	if(IsValidVolume(t,n)==false) return NULL;

	ImageDataItemPointer ch, vol;

	// volume directly available?
	int pos=GetVolumeIndex(t,n);
	vol=m_Volumes[pos];
	if((vol!=NULL) && (vol->IsComplete()))
		return vol;

	// is volume available as part of a channel that is available?
	ch=m_Channels[n];
	if((ch!=NULL) && (ch->IsComplete()))
	{
		vol=new ImageDataItem(*ch, 3, (t*m_OffsetTable[3])*m_PixelType.GetBpe()/8);
		return m_Volumes[pos]=vol;
	}

	// let's see if all slices of the volume are set, so that we can (could) combine them to a volume
	bool complete=true;
	unsigned int s;
	for(s=0;s<m_Dimensions[2];++s)
		if(m_Slices[GetSliceIndex(s,t,n)]==NULL)
		{
			complete=false;
			break;
		}
	if(complete)
	{
        vol=m_Volumes[pos];
		// ok, let's combine the slices!
        if(vol==NULL)
		    vol=new ImageDataItem(m_PixelType, 3, m_Dimensions);
		vol->SetComplete(true);
		int size=m_OffsetTable[2]*m_PixelType.GetBpe()/8;
		for(s=0;s<m_Dimensions[2];++s)
		{
			int posSl;
			ImageDataItemPointer sl;
			posSl=GetSliceIndex(s,t,n);

			sl=m_Slices[posSl];
            if(sl->GetParent()!=vol)
            {
			    sl=new ImageDataItem(*vol, 2, s*size);
			    sl->SetComplete(true);
			    memcpy(static_cast<char*>(vol->GetData())+sl->GetOffset(), sl->GetData(), size);
			    m_Slices[posSl]=sl;
            }
		}
		return m_Volumes[pos]=vol;
	}

	// volume is unavailable. Can we calculate it?
	if(GetSource()!=NULL)
	{
		// ... wir m�ssen rechnen!!! ....
		m_RequestedRegion.SetIndex(0, 0);
		m_RequestedRegion.SetIndex(1, 0);
		m_RequestedRegion.SetIndex(2, 0);
		m_RequestedRegion.SetIndex(3, t);
		m_RequestedRegion.SetIndex(4, n);
		m_RequestedRegion.SetSize(0, m_Dimensions[0]);
		m_RequestedRegion.SetSize(1, m_Dimensions[1]);
		m_RequestedRegion.SetSize(2, m_Dimensions[2]);
		m_RequestedRegion.SetSize(3, m_Dimensions[3]);
		m_RequestedRegion.SetSize(4, 1);
		m_RequestedRegionInitialized=true;
		GetSource()->Update();
		if(IsVolumeSet(t,n))
			//yes: now we can call ourselves without the risk of a endless loop (see if above)
			return GetVolumeData(t,n);
		else
			return NULL;
	}
	else
	{
		return AllocateVolumeData(t,n);
	}
}

//##ModelId=3E0B4A9100BC
mitk::ImageDataItem::Pointer mitk::Image::GetChannelData(int n)
{
	if(IsValidChannel(n)==false) return NULL;
	ImageDataItemPointer ch, vol;
	ch=m_Channels[n];
	if((ch!=NULL) && (ch->IsComplete()))
		return ch;

	// let's see if all volumes are set, so that we can (could) combine them to a channel
	if(IsChannelSet(n))
	{
		// if there is only one time frame we do not need to combine anything
		if(m_Dimensions[3]<=1)
		{
			vol=GetVolumeData(0,n);
			ch=new ImageDataItem(*vol, 3);
			ch->SetComplete(true);
		}
		else
		{
            ch=m_Channels[n];
			// ok, let's combine the volumes!
            if(ch==NULL)
			    ch=new ImageDataItem(m_PixelType, m_Dimension, m_Dimensions);
			ch->SetComplete(true);
			int size=m_OffsetTable[m_Dimension-1]*m_PixelType.GetBpe()/8;
			unsigned int t;
			for(t=0;t<m_Dimensions[3];++t)
			{
				int posVol;
				ImageDataItemPointer vol;

				posVol=GetVolumeIndex(t,n);
				vol=GetVolumeData(t,n);

                if(vol->GetParent()!=ch)
                {
				    // copy data of volume in channel
				    memcpy(static_cast<char*>(ch->GetData())+vol->GetOffset(), vol->GetData(), size);

				    // replace old volume with reference to channel
				    vol=new ImageDataItem(*vol, 3, t*m_OffsetTable[3]*m_PixelType.GetBpe()/8);
				    m_Volumes[posVol]=vol;
                }
			}
		}
		return m_Channels[n]=ch;
	}
	
	// channel is unavailable. Can we calculate it?
	if(GetSource()!=NULL)
	{
		// ... wir m�ssen rechnen!!! ....
		m_RequestedRegion.SetIndex(0, 0);
		m_RequestedRegion.SetIndex(1, 0);
		m_RequestedRegion.SetIndex(2, 0);
		m_RequestedRegion.SetIndex(3, 0);
		m_RequestedRegion.SetIndex(4, n);
		m_RequestedRegion.SetSize(0, m_Dimensions[0]);
		m_RequestedRegion.SetSize(1, m_Dimensions[1]);
		m_RequestedRegion.SetSize(2, m_Dimensions[2]);
		m_RequestedRegion.SetSize(3, m_Dimensions[3]);
		m_RequestedRegion.SetSize(4, m_ChannelNumber);
		m_RequestedRegionInitialized=true;
		GetSource()->Update();
		// did it work?
		if(IsChannelSet(n))
			//yes: now we can call ourselves without the risk of a endless loop (see if above)
			return GetChannelData(n);
		else
			return NULL;
	}
	else
	{
		return AllocateChannelData(n);
	}
}

//##ModelId=3E1012990305
bool mitk::Image::IsSliceSet(int s, int t, int n) const
{
	if(IsValidSlice(s,t,n)==false) return false;

	if(m_Slices[GetSliceIndex(s,t,n)]!=NULL)
		return true;

	ImageDataItemPointer ch, vol;
	vol=m_Volumes[GetVolumeIndex(t,n)];
	if((vol!=NULL) && (vol->IsComplete()))
		return true;
	ch=m_Channels[n];
	if((ch!=NULL) && (ch->IsComplete()))
		return true;
	return false;
}

//##ModelId=3E10139001BF
bool mitk::Image::IsVolumeSet(int t, int n) const
{
	if(IsValidVolume(t,n)==false) return false;
	ImageDataItemPointer ch, vol;

	// volume directly available?
	vol=m_Volumes[GetVolumeIndex(t,n)];
	if((vol!=NULL) && (vol->IsComplete()))
		return true;

	// is volume available as part of a channel that is available?
	ch=m_Channels[n];
	if((ch!=NULL) && (ch->IsComplete()))
		return true;

	// let's see if all slices of the volume are set, so that we can (could) combine them to a volume
	unsigned int s;
	for(s=0;s<m_Dimensions[2];++s)
		if(m_Slices[GetSliceIndex(s,t,n)]==NULL)
			return false;
	return true;
}

//##ModelId=3E1550E700E2
bool mitk::Image::IsChannelSet(int n) const
{
	if(IsValidChannel(n)==false) return false;
	ImageDataItemPointer ch, vol;
	ch=m_Channels[n];
	if((ch!=NULL) && (ch->IsComplete()))
		return true;
	// let's see if all volumes are set, so that we can (could) combine them to a channel
	unsigned int t;
	for(t=0;t<m_Dimensions[3];++t)
		if(IsVolumeSet(t,n)==false)
			return false;
	return true;
}

//##ModelId=3E10148003D7
bool mitk::Image::SetSlice(void *data, int s, int t, int n)
{
	if(IsValidSlice(s,t,n)==false) return false;
	ImageDataItemPointer sl;
	if(IsSliceSet(s,t,n))
	{
		sl=GetSliceData(s,t,n);
		memcpy(sl->GetData(), data, m_OffsetTable[2]*m_PixelType.GetBpe()/8);
        sl->Modified();
		//we have changed the data: call Modified()! 
		Modified();
	}
	else
	{
		sl=AllocateSliceData(s,t,n);
		if(sl==NULL) return false;
		memcpy(sl->GetData(), data, m_OffsetTable[2]*m_PixelType.GetBpe()/8);		
		//we just added a missing slice, which is not regarded as modification.
		//Therefore, we do not call Modified()!
	}
	return true;
}

//##ModelId=3E1014A00211
bool mitk::Image::SetVolume(void *data, int t, int n)
{
	if(IsValidVolume(t,n)==false) return false;
	ImageDataItemPointer vol;
	if(IsVolumeSet(t,n))
	{
		vol=GetVolumeData(t,n);
		memcpy(vol->GetData(), data, m_OffsetTable[3]*m_PixelType.GetBpe()/8);
        vol->Modified();
		vol->SetComplete(true);
		//we have changed the data: call Modified()! 
		Modified();
	}
	else
	{
		vol=AllocateVolumeData(t,n);
		if(vol==NULL) return false;
		memcpy(vol->GetData(), data, m_OffsetTable[3]*m_PixelType.GetBpe()/8);
		vol->SetComplete(true);
		//we just added a missing Volume, which is not regarded as modification.
		//Therefore, we do not call Modified()!
	}
	return true;
}

//##ModelId=3E1027F8023D
bool mitk::Image::SetPicSlice(ipPicDescriptor *pic, int s, int t, int n)
{
	if(pic==NULL) return false;
	if(pic->dim!=2) return false;
	if((pic->n[0]!=m_Dimensions[0]) || (pic->n[1]!=m_Dimensions[1])) return false;
	return SetSlice(pic->data,s,t,n); //FIXME: add geometry!
}

//##ModelId=3E102818024D
bool mitk::Image::SetPicVolume(ipPicDescriptor *pic, int t, int n)
{
	if(pic==NULL) return false;
    if((pic->dim==2) && ((m_Dimension==2) || ((m_Dimension>2) && (m_Dimensions[2]==1)))) return SetPicSlice(pic, 0, t, n);
	if(pic->dim!=3) return false;
	if((pic->n[0]!=m_Dimensions[0]) || (pic->n[1]!=m_Dimensions[1]) || (pic->n[2]!=m_Dimensions[2])) return false;
	return SetVolume(pic->data,t,n); //FIXME: check dim; add geometry!
}

//##ModelId=3E102AE9004B
void mitk::Image::Initialize(const mitk::PixelType& type, unsigned int dimension, unsigned int *dimensions, unsigned int channels)
{
	m_Dimension=dimension;
	if(m_Dimensions!=NULL)
		delete m_Dimensions;
	m_Dimensions=new unsigned int[m_Dimension>4?m_Dimension:4];
	memcpy(m_Dimensions, dimensions, sizeof(unsigned int)*m_Dimension);
	if(m_Dimension<4)
	{
		unsigned int i, *p;
		for(i=0,p=m_Dimensions+m_Dimension;i<4-m_Dimension;++i, ++p)
			*p=1;
	}
	m_ChannelNumber=channels;

	m_PixelType=type;
	m_Geometry3D=new Geometry3D(dimension, dimensions);

	ImageDataItemPointer dnull=NULL;

	m_Channels.reserve(m_ChannelNumber);
	m_Channels.assign(m_ChannelNumber, dnull);

	m_Volumes.reserve(m_ChannelNumber*m_Dimensions[3]);
	m_Volumes.assign(m_ChannelNumber*m_Dimensions[3], dnull);

	m_Slices.reserve(m_ChannelNumber*m_Dimensions[3]*m_Dimensions[2]);
	m_Slices.assign(m_ChannelNumber*m_Dimensions[3]*m_Dimensions[2], dnull);

	ComputeOffsetTable();

    SetRequestedRegionToLargestPossibleRegion();

	m_Initialized = true;
}

//##ModelId=3E102D2601DF
void mitk::Image::Initialize(ipPicDescriptor* pic, int channels, int tDim, int sDim)
{
	if(pic==NULL) return;

	m_Dimension=pic->dim;
	if(m_Dimensions!=NULL)
		delete m_Dimensions;
	m_Dimensions=new unsigned int[m_Dimension>4?m_Dimension:4];
	memcpy(m_Dimensions, pic->n, sizeof(unsigned int)*m_Dimension);
	if(m_Dimension<4)
	{
		unsigned int i, *p;
		for(i=0,p=m_Dimensions+m_Dimension;i<4-m_Dimension;++i, ++p)
			*p=1;
	}
	m_ChannelNumber=channels;

	if((m_Dimension>2) && (sDim>=0))
		m_Dimensions[2]=sDim;
	if((m_Dimension>3) && (tDim>=0))
		m_Dimensions[3]=tDim;

	m_PixelType=PixelType(pic);
	m_Geometry3D=new Geometry3D(m_Dimension, m_Dimensions);

    itkWarningMacro("Initialize() by pic assumes a plane, evenly spaced geometry starting at (0,0,0)."); 
    m_Geometry3D->SetSpacing(pic);
    m_Geometry3D->SetGeometry2D(pic, 0, 0);
    m_Geometry3D->SetEvenlySpaced();

	ImageDataItemPointer dnull=NULL;

	m_Channels.reserve(m_ChannelNumber);
	m_Channels.assign(m_ChannelNumber, dnull);

	m_Volumes.reserve(m_ChannelNumber*m_Dimensions[3]);
	m_Volumes.assign(m_ChannelNumber*m_Dimensions[3], dnull);

	m_Slices.reserve(m_ChannelNumber*m_Dimensions[3]*m_Dimensions[2]);
	m_Slices.assign(m_ChannelNumber*m_Dimensions[3]*m_Dimensions[2], dnull);

	ComputeOffsetTable();

    SetRequestedRegionToLargestPossibleRegion();

	// initialize level-window
	ipPicTSV_t *tsv = ipPicQueryTag( pic, "LEVEL/WINDOW" );
	if( tsv != NULL )
	{
		double level;
		double window;
#define GET_C_W( type, tsv, C, W )			\
		level = ((type *)tsv->value)[0];    \
		window = ((type *)tsv->value)[1];
		
		ipPicFORALL_2( GET_C_W, tsv, level, window );

		m_LevelWindow.SetLevelWindow( level, window );
	}

	m_Initialized = true;
}

//##ModelId=3E155CF000F6
bool mitk::Image::IsValidSlice(int s, int t, int n) const
{
	return ((s>=0) && (s<m_Dimensions[2]) && (t>=0) && (t<m_Dimensions[3]) && (n>=0) && (n<m_ChannelNumber));
}

//##ModelId=3E155D2501A7
bool mitk::Image::IsValidVolume(int t, int n) const
{
	return IsValidSlice(0, t, n);
}

//##ModelId=3E157C53030B
bool mitk::Image::IsValidChannel(int n) const
{
	return IsValidSlice(0, 0, n);
}

//##ModelId=3E19EA110396
bool mitk::Image::IsInitialized() const
{
	return m_Initialized;
}

//##ModelId=3E155E7A0374
void mitk::Image::ComputeOffsetTable()
{
	if(m_OffsetTable!=NULL)
		delete m_OffsetTable;

	m_OffsetTable=new unsigned int[m_Dimension>4 ? m_Dimension+1 : 4+1];
	
	unsigned int i;
	int num=1;
	m_OffsetTable[0] = 1;
	for (i=0; i < m_Dimension; ++i)
	{
		num *= m_Dimensions[i];
		m_OffsetTable[i+1] = num;
	}
	for (;i < 4; ++i)
		m_OffsetTable[i+1] = num;
}

//##ModelId=3E155C940248
int mitk::Image::GetSliceIndex(int s, int t, int n) const
{
	if(IsValidSlice(s,t,n)==false) return false;
	return s+t*m_Dimensions[2]+n*m_Dimensions[3]*m_Dimensions[2]; //??
}

//##ModelId=3E155C76012D
int mitk::Image::GetVolumeIndex(int t, int n) const
{
	if(IsValidVolume(t,n)==false) return false;
	return t+n*m_Dimensions[3]; //??
}

//##ModelId=3E1569310328
mitk::ImageDataItem::Pointer mitk::Image::AllocateSliceData(int s, int t, int n)
{
	int pos;
	pos=GetSliceIndex(s,t,n);

	// is slice available as part of a volume that is available?
	ImageDataItemPointer sl, ch, vol;
	vol=m_Volumes[GetVolumeIndex(t,n)];
	if(vol!=NULL)
	{
		sl=new ImageDataItem(*vol, 2, s*m_OffsetTable[2]*m_PixelType.GetBpe()/8);
		sl->SetComplete(true);
		return m_Slices[pos]=sl;
	}

	// is slice available as part of a channel that is available?
	ch=m_Channels[n];
	if(ch!=NULL)
	{
		sl=new ImageDataItem(*ch, 2, (s*m_OffsetTable[2]+t*m_OffsetTable[3])*m_PixelType.GetBpe()/8);
		sl->SetComplete(true);
		return m_Slices[pos]=sl;
	}

	// allocate new volume (instead of a single slice to keep data together!)
	m_Volumes[GetVolumeIndex(t,n)]=vol=AllocateVolumeData(t,n);
	sl=new ImageDataItem(*vol, 2, s*m_OffsetTable[2]*m_PixelType.GetBpe()/8);
	sl->SetComplete(true);
	return m_Slices[pos]=sl;

	////ALTERNATIVE:
	//// allocate new slice
	//sl=new ImageDataItem(m_PixelType, 2, m_Dimensions);
	//m_Slices[pos]=sl;
	//return vol;
}

//##ModelId=3E15694500EC
mitk::ImageDataItem::Pointer mitk::Image::AllocateVolumeData(int t, int n)
{
	int pos;
	pos=GetVolumeIndex(t,n);

	// is volume available as part of a channel that is available?
	ImageDataItemPointer ch, vol;
	ch=m_Channels[n];
	if(ch!=NULL)
	{
		vol=new ImageDataItem(*ch, 3, (t*m_OffsetTable[3])*m_PixelType.GetBpe()/8);
		return m_Volumes[pos]=vol;
	}

	// allocate new volume
	vol=new ImageDataItem(m_PixelType, 3, m_Dimensions);
	m_Volumes[pos]=vol;
	return vol;
}

//##ModelId=3E1569500322
mitk::ImageDataItem::Pointer mitk::Image::AllocateChannelData(int n)
{
	// allocate new channel
	ImageDataItemPointer ch=new ImageDataItem(m_PixelType, m_Dimension, m_Dimensions);
	m_Channels[n]=ch;
	return ch;
}

//##ModelId=3E15F6C60103
mitk::Image::Image() : 
	m_Dimension(0), m_Dimensions(NULL), m_OffsetTable(NULL),
		m_CompleteData(NULL), m_PixelType(NULL), m_Initialized(false)
{

}

//##ModelId=3E15F6CA014F
mitk::Image::~Image()
{

}

//##ModelId=3E1A11530384
unsigned int* mitk::Image::GetDimensions() const
{
	return m_Dimensions;
}

//##ModelId=3ED91D060027
const mitk::LevelWindow& mitk::Image::GetLevelWindow() const
{
    return m_LevelWindow;
}

//##ModelId=3ED91D060085
float mitk::Image::GetScalarValueMin() const
{
    return -1.0f;
}

//##ModelId=3ED91D0600E2
float mitk::Image::GetScalarValueMax() const
{
    return -1.0f;
}

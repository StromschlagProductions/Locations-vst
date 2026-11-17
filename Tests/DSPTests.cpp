#include "LocationsDSP.h"
#include "TestSupport.h"
#include <limits>
static std::vector<float> render(double fs,int place,bool split)
{
 loc::Engine e;e.prepare(fs);loc::Params p;p.placeA=place;p.placeB=0;p.distance=.8f;p.echoTime=4;
 std::vector<float> l(17003),r(l.size());for(size_t i=0;i<l.size();++i)l[i]=r[i]=float(.1*sin(i*.03));
 for(size_t at=0;at<l.size();){int n=(int)std::min(l.size()-at,split?size_t(137):l.size());{NoAllocations guard;e.process(l.data()+at,r.data()+at,n,p);}at+=(size_t)n;}
 for(float x:l)CHECK(std::isfinite(x)&&std::abs(x)<10);return l;
}
int main(){NoDenormals denormals;
 for(double fs:{22050.,44100.,48000.,96000.,192000.})for(int place=0;place<loc::kNumPlaces;++place)CHECK(render(fs,place,false)==render(fs,place,true));
 loc::Engine e;e.prepare(48000);{NoAllocations guard;e.reset();}
 loc::Params p;p.echoTime=std::numeric_limits<float>::quiet_NaN();p.roomDb=p.blend=p.distance=p.door=p.pan=p.echoTime;p.placeA=999;
 float l=std::numeric_limits<float>::quiet_NaN(),r=l;e.process(&l,&r,1,p);CHECK(std::isfinite(l)&&std::isfinite(r));
 // Delay scaling affects the first outdoor echo; indoor echo remains unchanged.
 for(int index:{14,15,16}){loc::Room room;room.prepare(48000);room.setEchoScale(4);room.want(index);int first=-1;
 for(int i=0;i<100000;++i){float a,b;room.process(i==0?1.f:0.f,0,1,a,b);if(first<0&&std::abs(a)>1e-5f)first=i;}
 CHECK(first>0);CHECK(std::abs(first-int(loc::place(index).echoMs*.001f*48000*4))<=2);}
 puts("Locations: rates, all places, block partition, outdoor echo scaling, finite controls and allocation checks passed");}

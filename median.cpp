#include "StdAfx.h"
#include "median.h"
#include "util.h"
#include "types.h"
#include "getOpt.h"
#include <list>
#include <xmmintrin.h>

namespace {

struct MortonOrder
{
	template<class TArray, class TCoord> static int offset(TArray const& array, TCoord x, TCoord y)
	{
		return y*array.w+x;
	}
};

struct Vec2s { short x, y; Vec2s(short x, short y) : x(x), y(y) {} };

//float lengthSquared(vec3 const& v) { return std::max(v.x,std::max(v.y,v.z)); }
//float lengthSquared(vec3 const& v) { return v.x+v.y+v.z; }
float lengthSquared(vec3 const& v) { return v.lengthSquared(); }

struct Lum { float luminance; Vec2s pos;
	Lum(float l, Vec2s p) :luminance(l), pos(p){}
};

bool lessl(Lum const& a, Lum const& b) {
	return reinterpret_cast<int const&>(a.luminance)<reinterpret_cast<int const&>(b.luminance);
}

bool morel(Lum const& a, Lum const& b) {
	return reinterpret_cast<int const&>(a.luminance)>reinterpret_cast<int const&>(b.luminance);
}

template<class T>
struct pvector
{
private:
	vector<T> data;
	int _size;
	T* basePtr;
public:
	pvector(T const& first) { data.reserve(100); _size=0;data.push_back(first); _size = 0; basePtr = data.data() + 1; }
	typename vector<T>::iterator begin() { auto i = data.begin(); ++i; return i; }
	typename vector<T>::iterator end() { return data.end(); }
	int size() const { return _size; }
	void push_back(T const& obj) { data.push_back(obj); _size++; }
	T& operator[](int index) { return basePtr[index]; }
};

// desc order
template<class T>
void InsertionSort(pvector<T> &spots, int numSortedElements, int percentile)
{
	for(int j = numSortedElements; j < spots.size(); j++)
	{
		T key = spots[j];
		T const& median = spots[percentile];
		if(lessl(key, median))
			continue;
		T* p = &spots[j];
		T* next = p - 1;
		while(lessl(*next, key))
		{
			*p = *next;
			p = next;
			--next;
		}
		*p = key;
	}
}

// desc order
template<class T>
void InsertionSortNew(pvector<T> &spots, int numSortedElements, int percentile)
{
	int firstOfUnsorted = numSortedElements;
	for(int j = numSortedElements; j < spots.size(); j++)
	{
		T key = spots[j];
		T const& median = spots[percentile];
		if(lessl(key, median))
			continue;
		T* p = &spots[j];
		T* next = p - 1;
		//firstOfUnsorted
		while(lessl(*next, key))
		{
			*p = *next;
			p = next;
			--next;
		}
		*p = key;
	}
}

} // namespace

void median(int msize, Image& image)
{
	auto copy = image;
	Array2D<float> luminances(image.w, image.h);
	forxy(luminances) luminances(p) = lengthSquared(image(p));
	GETFLOAT(fpercentile, "min=0 max=1 step=0.1", .5);
	GETBOOL(useNew, "", false);
	ivec2 p;
	p.x = msize;
	for(p.y = msize; p.y < image.h -msize; p.y++)
	{
		p.x = msize;
		pvector<Lum> spots(Lum(FLT_MAX, Vec2s(0, 0)));
		for(int i=-msize;i<=msize;i++)for(int j=-msize;j<=msize;j++)
			//if(vec2(i,j).length() < msize)
			{
				Vec2s pos(p.x+i, p.y+j);
				spots.push_back(Lum(luminances(pos.x, pos.y), pos));
			}
		std::sort(spots.begin(), spots.end(), morel);
		int percentile=fpercentile*(spots.size()-1);
		for(p.x = msize; p.x < image.w -msize-1; p.x++)
		{
			(useNew?InsertionSortNew<Lum>:InsertionSort<Lum>)(spots, spots.size() - (msize*2+1), percentile);
			image(p) = copy(spots[percentile].pos.x, spots[percentile].pos.y);
			Lum* takeIndex = &spots[0]; // offset to the right
			Lum* putIndex = &spots[0];
			const short A = p.x - msize; Lum *const B = &spots[spots.size() - 1];
			Lum* end = &spots[spots.size()];
			for(; takeIndex < end; takeIndex++)
			{
				if(takeIndex->pos.x != A)
				{
					*putIndex = *takeIndex;
					putIndex++;
				}
			}
			int zBegin = putIndex - &spots[0];
			for(int z = zBegin; z < spots.size(); z++)
			{
				int j = -msize + z - zBegin;
				Vec2s pos(p.x + msize + 1, p.y + j);
				spots[z] = Lum(luminances(pos.x, pos.y), pos);
			}
		}
	}
}

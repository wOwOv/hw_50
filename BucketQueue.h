#ifndef __BUCKETQUEUE__
#define __BUCKETQUEUE__

#include <windows.h>

//배열형 락프리큐
template <class DATA>
class BucketQueue
{
	struct Bucket
	{
		DATA* _data;
	};
public:
	BucketQueue(int size):_headIndex(0),_tailIndex(0),_maxCount(size-1), _num(0)
	{
		_bucketArray = new Bucket[size];
	}
	~BucketQueue()
	{
		int i = _headIndex;
		while (1)
		{
			if (i == _tailIndex)
			{
				break;
			}
			DATA* ptr = _bucketArray[i]._data;
			while (1)
			{
			
			}
			

		
			i=(i+1)%(maxCount+1)
		}
	}

	void Enqueue(DATA* data)
	{
		InterlockedCompareExchangeAcquire()





		/*Node* newnode = _nodepool.Alloc();
		newnode->_data = data;
		unsigned long long temp = InterlockedIncrement16(&_key);
		unsigned long long countnode = (unsigned long long)newnode;
		countnode |= (temp << 48);
		Node* oldtop;
		do
		{
			oldtop = _top;
			newnode->_next = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)countnode, (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_num);*/
	}

	void Pop(DATA* data)
	{
		Node* oldtop;
		Node* newtop;
		Node* realadr;
		unsigned long long tempadr;
		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr <<= 16;
			tempadr >>= 16;
			realadr = (Node*)tempadr;
			newtop = realadr->_next;
			*data = realadr->_data;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);
		InterlockedDecrement(&_num);

		_nodepool.Free(realadr);
	}

	unsigned long long GetSize()
	{
		return _num;
	}
	 GetNodeCapacity()
	{
		return _nodepool.GetCapacityCount();
	}

private:
	Bucket* _bucketArray;					//배열
	unsigned long long _headIndex;			//head인덱스
	unsigned long long _tailIndex;			//tail인덱스

	unsigned long long _maxCount;			//배열 총 칸 개수

	unsigned long long _num;				//사용 중인 칸 개수

};





#endif

#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>
#include "BucketStack.h"

static long cookie = 0x01010100;

template <class DATA>
class TlsMemoryPool
{
private:
	struct Node
	{
		Node* _retbucket;
		int _underguard;
		DATA _data;
		int _overguard;
	};

	struct Bucket
	{
		Node* node=nullptr;
		unsigned long _max=0;
		unsigned long _index=0;
		unsigned long _return=0;
	};


public:

	TlsMemoryPool(int bunchsize=100,bool PlacementNew = false)
	{
		_tlsIndex = TlsAlloc();
		if (_tlsIndex == TLS_OUT_OF_INDEXES)
		{
			DebugBreak();
		}

		_cookie = InterlockedIncrement(&cookie);

		_pnFlag = PlacementNew;

		_bunchsize = bunchsize;

	}
	~TlsMemoryPool()
	{
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			DebugBreak();
		}

		

		TlsFree(_tlsIndex);
	}

	DATA* Alloc(void)
	{
		Node* allocated;

		Bucket* bucket= (Bucket*)TlsGetValue(_tlsIndex);
		if (bucket == nullptr)
		{
			bucket = new Bucket;

			/*
			//������ ȣ���� ���·� ������
			if (_pnFlag == 0)
			{
				for (int i = 0; i < _maxcount; i++)
				{
					Node* node = new Node;
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}
			else//������ ȣ�� ���� ������
			{
				for (int i = 0; i < _maxcount; i++)
				{
					Node* node = (Node*)malloc(sizeof(Node));
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}

			tpool->_nodeCount = _maxcount;
			tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;
*/
			TlsSetValue(_tlsIndex, (LPVOID)bucket);
		}


		if (bucket->_max == 0)
		{
			//_top���� ��������


		}

			//������ ȣ���� ���·� ������
			if (_pnFlag == 0)
			{
				for (int i = 0; i < _maxcount; i++)
				{
					Node* node = new Node;
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}
			else//������ ȣ�� ���� ������
			{
				for (int i = 0; i < _maxcount; i++)
				{
					Node* node = (Node*)malloc(sizeof(Node));
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}

			tpool->_nodeCount = _maxcount;
			tpool->_storedCount = tpool->_nodeCount+tpool->_freeCount;

			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//����Ϸ��� �����ϰ� �ִ� �� ��尡 �ִٸ� 
		if (tpool->_nodeCount > 0)
		{
			allocated = tpool->_nodelist;
			tpool->_nodelist = allocated->_next;
			//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_nodeCount--;
		}
		//�� ���� ���� ��ȯ�Ϸ��� �����ص� ��尡 �ִٸ�
		else if (tpool->_freeCount > 0)
		{
			allocated = tpool->_freelist;
			tpool->_freelist = allocated->_next;
			//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
			if ( _pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_freeCount--;
		}
		//����Ǯ���� �޾ƿ;��Ѵٸ�
		else
		{
			Node* nodebunch = (Node*)_bucketstack.GetBucket();
			//����Ǯ���� ��幭�� �޾ƿ�
			if (nodebunch != nullptr)
			{
				//�� ��帮��Ʈ�� ����
				tpool->_nodelist = nodebunch;

				//��� ���� �ֱ�
				allocated = tpool->_nodelist;
				tpool->_nodelist = allocated->_next;
				//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
				if ( _pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				tpool->_nodeCount =  _bunchsize - 1;
			}
			//����Ǯ������ �� �޾ƿ���
			else
			{
				//��¥ �Ҵ��ؼ� �����
				allocated = new Node;
				allocated->_underguard =  _cookie;
				allocated->_overguard = _cookie;
			}
			
		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		return &(allocated->_data);
	}




	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////��ȯ�� Data*��ó�� Node�Ҵ� �ּҸ� ã�Ƽ� �װ� node�鿡 �����־������.
	bool Free(DATA* data)
	{
		//����Ҵ� �ּ� ã��
		char* address = (char*)data;
		address -=  _position;
		Node* retnode = (Node*)address;

		//���.�����÷ο� Ȯ�� �� �´� ��Ұ� ���Դ��� Ȯ��
		if (retnode->_underguard !=  _cookie || retnode->_overguard !=  _cookie)
		{
			return false;
		}

		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			tpool = new TlsPool;
			tpool->_nodelist = nullptr;
			tpool->_nodeCount = 0;
			tpool->_freelist = nullptr;
			tpool->_freeCount = 0;
			tpool->_storedCount = 0;

			//������ ȣ���� ���·� ������
			if (_pnFlag == 0)
			{
				for (int i = 0; i < _maxcount; i++)
				{
					Node* node = new Node;
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}
			else//������ ȣ�� ���� ������
			{
				for (int i = 0; i < _maxcount; i++)
				{
					Node* node = (Node*)malloc(sizeof(Node));
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}

			tpool->_nodeCount = _maxcount;
			tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//_pnFlag�� 1�̶�� �Ҹ��� ȣ���ؼ� ����
		if ( _pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		//nodelist �ڸ��� �ִٸ�
		if (tpool->_nodeCount <  _bunchsize)
		{
			retnode->_next = tpool->_nodelist;
			tpool->_nodelist = retnode;
			tpool->_nodeCount++;
		}
		//nodelist�� �ڸ��� ���ٸ� freelist��
		else
		{
			retnode->_next = tpool->_freelist;
			tpool->_freelist = retnode;
			tpool->_freeCount++;

			//freelist�� �� á�ٸ�
			if (tpool->_freeCount ==  _bunchsize)
			{
				_bucketstack.ReturnBucket(tpool->_freelist);
				tpool->_freelist = nullptr;
				tpool->_freeCount = 0;
			}
		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount(void)
	{
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			DebugBreak();
		}
		return tpool->_storedCount;
	}


private:
	int _cookie;
	bool _pnFlag;
	unsigned int _bunchsize;

	DWORD _tlsIndex=0;

	Bucket* _top;
	unsigned long _num;
	short _key;

};




#endif


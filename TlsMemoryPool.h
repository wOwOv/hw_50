#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>
#include "BucketStack.h"

template <class DATA>
class TlsMemoryPool
{
private:
	const unsigned long long ADRMASK = 0x0000ffffffffffff;
	const unsigned long long TAGMASK = 0xffff000000000000;
	const unsigned long long MAKETAG = 0x0001000000000000;

	struct Node
	{
		int _underguard;
		DATA _data;
		int _overguard;
		Node* _next;
	};


public:

	TlsMemoryPool(int BlockNum=100, int bunchsize=100,bool PlacementNew = false, bool maxflag = false)
	{
		_nodelist = nullptr;
		_nodeCount = 0;

		_freelist = nullptr;
		_freeCount = 0;

		_cookie = _bucketstack.GetCommoncookie();
	
		Node* node = new Node;
		_position = (long long)&node->_data - (long long)&node->_underguard;
		delete node;

		_key = 0;

		_pnFlag = PlacementNew;
		_maxcount = BlockNum;
		_maxFlag = maxflag;

		_bunchsize = bunchsize;


		//������ ȣ���� ���·� ������
		if (_pnFlag == 0)
		{
			for (int i = 0; i < BlockNum; i++)
			{
				Node* node = new Node;
				node->_underguard = _cookie;
				node->_overguard = _cookie;

				node->_next = _nodelist;
				_nodelist = node;
			}
		}
		else//������ ȣ�� ���� ������
		{
			for (int i = 0; i < BlockNum; i++)
			{
				Node* node = (Node*)malloc(sizeof(Node));
				node->_underguard = _cookie;
				node->_overguard = _cookie;

				node->_next = _nodelist;
				_nodelist = node;
			}
		}


		_storedCount = BlockNum;


	}
	~TlsMemoryPool()
	{
		Node* node = _nodelist;
		Node* temp = node;
		//�̹� ������ ȣ��Ǿ��ִ� ���·� �������. �Ҹ��� ȣ�� �Ǿ����
		if (_pnFlag == 0)
		{
			while (1)
			{
				if (node != nullptr)
				{
					unsigned long long tempadr = (unsigned long long)node;
					tempadr &= ADRMASK;
					Node* realadr = (Node*)tempadr;
					temp = realadr->_next;
					delete realadr;
					node = temp;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			while (1)
			{
				if (node != nullptr)
				{
					unsigned long long tempadr = (unsigned long long)node;
					tempadr &= ADRMASK;
					Node* realadr = (Node*)tempadr;
					temp = realadr->_next;
					free (realadr);
					node = temp;
				}
				else
				{
					break;
				}
			}
		}

		node = _freelist;
		temp = node;
		//�̹� ������ ȣ��Ǿ��ִ� ���·� �������. �Ҹ��� ȣ�� �Ǿ����
		if (_pnFlag == 0)
		{
			while (1)
			{
				if (node != nullptr)
				{
					unsigned long long tempadr = (unsigned long long)node;
					tempadr &= ADRMASK;
					Node* realadr = (Node*)tempadr;
					temp = realadr->_next;
					delete realadr;
					node = temp;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			while (1)
			{
				if (node != nullptr)
				{
					unsigned long long tempadr = (unsigned long long)node;
					tempadr &= ADRMASK;
					Node* realadr = (Node*)tempadr;
					temp = realadr->_next;
					free(realadr);
					node = temp;
				}
				else
				{
					break;
				}
			}
		}
	}

	DATA* Alloc(void)
	{
		Node* allocated;

		//����Ϸ��� �����ϰ� �ִ� �� ��尡 �ִٸ� 
		if (_nodeCount > 0)
		{
			allocated = _nodelist;
			_nodelist = allocated->_next;
			//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			_nodeCount--;
		}
		//�� ���� ���� ��ȯ�Ϸ��� �����ص� ��尡 �ִٸ�
		else if (_freelist > 0)
		{
			allocated = _freelist;
			_freelist = allocated->_next;
			//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			_freeCount--;
		}
		//����Ǯ���� �޾ƿ;��Ѵٸ�
		else
		{
			Node* nodebunch = _bucketstack.GetBucket();
			//����Ǯ���� ��幭�� �޾ƿ�
			if (nodebunch != nullptr)
			{
				//�� ��帮��Ʈ�� ����
				_nodelist = nodebunch;

				//��� ���� �ֱ�
				allocated = _nodelist;
				_nodelist = allocated->_next;
				//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
				if (_pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				_nodeCount = _bunchsize - 1;

			}
			//����Ǯ������ �� �޾ƿ���
			else
			{
				//��¥ �Ҵ��ؼ� �����
				allocated = new Node;
				allocated->_underguard = _cookie;
				allocated->_overguard = _cookie;
			}
			
		}

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
		address -= _position;
		Node* retnode = (Node*)address;

		//���.�����÷ο� Ȯ�� �� �´� ��Ұ� ���Դ��� Ȯ��
		if (retnode->_underguard != _cookie || retnode->_overguard != _cookie)
		{
			return false;
		}

		//_pnFlag�� 1�̶�� �Ҹ��� ȣ���ؼ� ����
		if (_pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		//nodelist �ڸ��� �ִٸ�
		if (_nodeCount < _bunchsize)
		{
			retnode->_next = _nodelist;
			_nodelist = retnode;
			_nodeCount++;
		}
		//nodelist�� �ڸ��� ���ٸ� freelist��
		else
		{
			retnode->_next = _freelist;
			_freelist = retnode;
			_freeCount++;

			//freelist�� �� á�ٸ�
			if (_freeCount == _bunchsize)
			{
				_bucketstack.ReturnBucket(_freelist);
				_freelist = nullptr;
				_freeCount = 0;
			}
		}

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
		return _nodeCount+_freeCount;
	}


private:
	Node* _nodelist;
	unsigned int _nodeCount;

	Node* _freelist;
	unsigned int _freeCount;

	int _storedCount;
	int _cookie;
	long long _position;
	short _key;

	bool _pnFlag;
	int _maxcount;
	bool _maxFlag;

	unsigned int _bunchsize;
	//static ����Ŷ ���� ����
	static BucketStack _bucketstack;
};

template<class DATA>
BucketStack TlsMemoryPool<DATA>::_bucketstack;




#endif


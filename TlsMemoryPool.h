#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>

static int key = 0xaaaa;



#define ADRMASK 0x0000ffffffffffff;
#define TAGMASK 0xffff000000000000;
#define MAKETAG 0x0001000000000000;



template <class DATA>
class MemoryPool
{
private:
	struct Node
	{
		int _underguard;
		DATA _data;
		int _overguard;
		Node* _next;
	};


public:

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ʱ� �� ����.
	//				(bool) Alloc �� ������ / Free �� �ı��� ȣ�� ����
	// Return:
	//////////////////////////////////////////////////////////////////////////
	MemoryPool(int BlockNum, bool PlacementNew = false, bool maxflag = false,bool sharedflag=false,int arraycount=100)
	{
		_head = nullptr;
		_cookie = key;
		key++;
		_pnFlag = PlacementNew;

		_maxFlag = maxflag;

		_sharedflag=sharedflag
		_key = 0;

		Node* node = new Node;
		_position = (long long)&node->_data - (long long)&node->_underguard;
		delete node;

		//������ ȣ���� ���·� ������
		if (_pnFlag == 0)
		{
			for (int i = 0; i < BlockNum; i++)
			{
				Node* node = new Node;
				node->_underguard = _cookie;
				node->_overguard = _cookie;

				node->_next = _head;
				_head = node;
			}
		}
		else//������ ȣ�� ���� ������
		{
			for (int i = 0; i < BlockNum; i++)
			{
				Node* node = (Node*)malloc(sizeof(Node));
				node->_underguard = _cookie;
				node->_overguard = _cookie;

				node->_next = _head;
				_head = node;
			}
		}


		_capacity = BlockNum;
		_usingCount = 0;


	}
	virtual	~MemoryPool()
	{
		Node* node = _head;
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
					free realadr;
					node = temp;
				}
				else
				{
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// �� �ϳ��� �Ҵ�޴´�.  
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ �� ������.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		Node* allocated;
		Node* oldhead;
		Node* newhead;
		Node* realadr;
		unsigned long long tempadr;
		do
		{
			oldhead = _head;
			if (oldhead == nullptr)
			{
				if (_maxFlag == true)
				{
					if (_maxcount == _capacity)
					{
						return nullptr;
					}
				}
				allocated = new Node;
				allocated->_underguard = _cookie;
				allocated->_overguard = _cookie;
				realadr = allocated;
				InterlockedIncrement(&_capacity);
				break;
			}

			tempadr = (unsigned long long)oldhead;
			tempadr &= ADRMASK;
			realadr = (Node*)tempadr;
			newhead = realadr->_next;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)newhead, (__int64)oldhead) != (__int64)oldhead);
		allocated = realadr;
		//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
		if (_pnFlag != 0)
		{
			new(&(allocated->_data)) DATA;
		}

		InterlockedIncrement(&_usingCount);

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

		unsigned long long countnode = (unsigned long long)retnode;
		countnode &= ADRMASK;
		Node* oldhead;
		do
		{
			oldhead = _head;
			unsigned long long tag = oldhead;
			tag &= TAGMASK;
			tag += MAKETAG;
			countnode |= tag;
			retnode->_next = oldhead;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)countnode, (__int64)oldhead) != (__int64)oldhead);

		InterlockedDecrement(&_usingCount);
		
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
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� ������� �� ������ ��´�.
	//
	// Parameters: ����.
	// Return: (int) ������� �� ����.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount(void)
	{
		return _usingCount;
	}


	void SetMaxCount(int maxcount)
	{
		_maxcount = maxcount;
	}

	// ���� ������� ��ȯ�� (�̻��) ������Ʈ ���� ����.


private:
	Node* _head;
	int _cookie;
	int _capacity;
	int _usingCount;
	bool _pnFlag;

	long long _position;

	int _maxcount;
	bool _maxFlag;

	short _key;

	//���� ����Ŷ���� ���� ����
	bool _sharedflag
	//static //����Ŷ ���� ����
	Node* _freelist;
	unsigned long _freeCount;
};




template <class DATA>
thread_local MemoryPool<DATA> pool;








#endif
#pragma once

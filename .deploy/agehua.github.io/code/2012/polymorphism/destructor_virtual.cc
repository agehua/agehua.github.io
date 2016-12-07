// 分析析构函数为虚函数时的特性

#include <iostream>

using namespace std;

class base{
public:
	virtual	~base(){ cout << "in base::~base()" << endl; }
};

class derived: public base{
public:
	virtual ~derived(){ cout << "in derived::~derived" << endl; }
};

class base1 {
public:
	~base1(){ cout << "in base1::~base1()" << endl; }
};

class derived1: public base1{
public:
	~derived1(){ cout << "in derived1::~derived1" << endl; }
};

int main(void){
	base *pbase = new derived();
	delete pbase;
	cout << "--------" << endl;
	base1 *pbase1 = new derived1();
	delete pbase1;
}

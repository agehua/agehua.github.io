// test virtual functions

#include <iostream>

using namespace std;

class father_b1 {
public:
	virtual void f(){ cout << "in father of base1:f" << endl;}
	virtual void fbf(){ cout << "in father of base1:fbf" << endl;}
};

class father1_b1 {
public:
	virtual void f(){ cout << "in father1 of base1:f" << endl;}
	virtual void f1bf(){ cout << "in father1 of base1:f1bf" << endl;}
};

class base {
public:
	virtual void f(){ cout << "in base:f" << endl;}
	virtual void bf0(){ cout << "in base:f0" << endl;}
};

class base1: public father_b1, public father1_b1{
public:
	virtual void f(){ cout << "in base1:f" << endl;}
	virtual void b1f0(){ cout << "in base1:f0" << endl;}
};

class derived: public base1, public base {
public:
	virtual void f(){ cout << "in derived:f" << endl;}
	virtual void f1(){ cout << "in derived:f1" << endl;}
};

typedef void(*fun)(void);

#define VTAB(pclass) ((long*)(*(long*)(pclass)))
#define VTAB1(pclass) ((long*)(*(((long*)(pclass)) + 1)))
#define VTAB2(pclass) ((long*)(*(((long*)(pclass)) + 2)))


int main(void) {
	derived* pd = new derived();

	cout << "--- vtable ---" << endl;
	fun pfun = (fun)*VTAB(pd);
	pfun();
	pfun = (fun)*(VTAB(pd)+1);
	pfun();
	pfun = (fun)*(VTAB(pd)+2);
	pfun();
	pfun = (fun)*(VTAB(pd)+3);
	pfun();

	cout << endl; 
	cout << "--- vtable + 1 ---" << endl;
	pfun = (fun)*VTAB1(pd);
	pfun();
	pfun = (fun)*(VTAB1(pd)+1);
	pfun();

	cout << endl; 
	cout << "--- vtable + 2 ---" << endl;
	pfun = (fun)*VTAB2(pd);
	pfun();
	pfun = (fun)*(VTAB2(pd)+1);
	pfun();
}

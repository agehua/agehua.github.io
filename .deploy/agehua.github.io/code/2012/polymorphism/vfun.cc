/*
 * test virtual functions
 */

#include <iostream>

using namespace std;

class base {
public:
	virtual void f(){ cout << "in base:f" << endl;}
};

class derived: public base {
public:
	virtual void f(){ cout << "in derived:f" << endl;}
};

typedef void(*fun)(void);
#define VTAB(pclass) ((long*)(*(long*)(pclass)))

int main(void) {
	base* pb = new base();
	pb->f();
	base* pd = (base*) new derived();
	pd->f();

	cout << "--- call through vtable ---" << endl;
	fun pfun = (fun)*VTAB(pd);
	pfun();
	pfun = (fun)*VTAB(pb);
	pfun();
	derived* pd_fake = (derived*)pb;
	pfun = (fun)*VTAB(pd_fake);
	pfun();

	pfun = (fun)*VTAB(pd);
	cout << "value of *pfun is: " << *pfun <<  endl;
	pfun = (fun)*(VTAB(pd) + 1);
	cout << "value of *pfun is: " << *pfun <<  endl;
}

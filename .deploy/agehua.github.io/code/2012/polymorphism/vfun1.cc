// test virtual functions

#include <iostream>

using namespace std;

class base {
public:
	virtual void f(){ cout << "in base:f" << endl;}
	virtual void f0(){ cout << "in base:f0" << endl;}
};
class derived: public base {
public:
	virtual void f(){ cout << "in derived:f" << endl;}
	virtual void f1(){ cout << "in derived:f1" << endl;}
};

typedef void(*fun)(void);

#define VTAB(pclass) ((long*)(*(long*)(pclass)))

int main(void) {
	base* pb_real = new base();
	pb_real->f();
	base* pb = (base*) new derived();
	pb->f();
	pb->f0();

	cout << "--- call function through vtable ---" << endl;
	fun pfun = (fun)*VTAB(pb);
	pfun();
	pfun = (fun)*(VTAB(pb)+1);
	pfun();
	pfun = (fun)*(VTAB(pb)+2);
	pfun();
}

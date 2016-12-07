/*
 * polymorphism.cc: example of polymorphism
 */

#include <iostream>

using namespace std;

class base {
public:
	virtual void f(){ cout << "in base:f" << endl;}
};

class derived1: public base {
public:
	virtual void f(){ cout << "in derived1:f" << endl;}
};

class derived2: public base {
public:
	virtual void f(){ cout << "in derived2:f" << endl;}
};

void call_func(base* pb) {
	pb->f();
}

int main(void) {
	base* pb = new base();
	base* pd1 = new derived1();
	base* pd2 = new derived2();
	call_func(pb);
	call_func(pd1);
	call_func(pd2);
}

/*
 * 测试c++隐藏规则：
 * （1）如果派生类的函数与基类的函数同名，但是参数不同。
 *      不论有无virtual关键字，基类的函数将被隐藏（注意别与重载混淆）。
 * （2）如果派生类的函数与基类的函数同名，并且参数也相同，但是基类函数
 *      没有virtual关键字。此时，基类的函数被隐藏（注意别与覆盖混淆）。
 */

#include <iostream>

using namespace std;

class Base {
public:
	virtual void f(float x){ cout << "Base::f(float) " << x << endl; }
	void g(float x){ cout << "Base::g(float) " << x << endl; }
	void h(float x){ cout << "Base::h(float) " << x << endl; }
	void m(float x){ cout << "Base::m(float) " << x << endl; }
};

class Derived : public Base {
public:
	virtual void f(float x){ cout << "Derived::f(float) " << x << endl; }
	void g(int x){ cout << "Derived::g(int) " << x << endl; }
	void h(float x){ cout << "Derived::h(float) " << x << endl; }
	void m(int x){ cout << "Derived::m(int) " << x << endl; }
};

int main(void) {
	Derived d;
	Base *pb = &d;
	Derived *pd = &d;
	// Good : behavior depends solely on type of the object
	pb->f(3.14f); // Derived::f(float) 3.14
	pd->f(3.14f); // Derived::f(float) 3.14
	// Bad : behavior depends on type of the pointer
	pb->g(3.14f); // Base::g(float) 3.14
	pd->g(3.14f); // Derived::g(int) 3 (surprise!)
	// Bad : behavior depends on type of the pointer
	pb->h(3.14f); // Base::h(float) 3.14 (surprise!)
	pd->h(3.14f); // Derived::h(float) 3.14
	// Bad : behavior depends on type of the pointer
	pb->m(3.14f); // Base::m(float) 3.14
	pd->m(3.14f); // Derived::m(int) 3 (surprise!)
}

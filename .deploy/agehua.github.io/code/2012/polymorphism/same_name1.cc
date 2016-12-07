// 测试隐藏规则：
// 子类重载同名函数, 无论是否声明为虚函数，父类的所有同名函数将会被隐藏 

#include <iostream>

using namespace std;

class Base{
public:
	virtual void f(float x){ cout << "Base::f(float) " << x << endl; }
	virtual void f(float x, float y){ cout << "Base::f(float, float) " << x << endl; }
};
 
class Derived : public Base{
public:
	virtual void f(float x){ cout << "Derived::f(float) " << x << endl; }
};
 
int main(void){
	Derived d;
	Base *pb = &d;
	Derived *pd = &d;
	pb->f(3.14f, 3.14f); 
	pd->f(3.14f, 3.14f); // Compile error
}


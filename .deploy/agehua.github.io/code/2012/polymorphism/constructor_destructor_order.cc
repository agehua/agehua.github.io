/*
 * 研究构造函数的调用顺序
 *  1. 首先调用基类的构造函数（如果有基类）；如果有多个基类，则按基类被列出的顺序调用；
 *  2. 调用这个类的成员对象的构造函数（如果有的话）；如果有多个成员对象，则按成员对象定义的顺序被调用（与参数列表中列出的顺序无关）；
 *  3. 最后调用这个类自身的构造函数；
 *  4. 析构函数正好相反;
 *  注意：如果有虚基类，则先调用虚基类的构造函数。在调用基类的构造函数，如果有多个虚基类，则按列出的顺序调用； 
*/

#include <iostream>

using namespace std;

class father_base_a{
public:
   father_base_a(){cout<<"This is father_base_a;"<<endl;}
   virtual ~father_base_a() { cout << "This is ~father_base_a" << endl; }
};

class base_a: public father_base_a{
public:
   base_a(){cout<<"This is base_a;"<<endl;}
   virtual ~base_a() { cout << "This is ~base_a" << endl; }
};

class base_b{
public:
   base_b(){cout<<"This is base_b;"<<endl;}
   virtual ~base_b() { cout << "This is ~base_b" << endl; }
};

class member_a{
public:
   member_a(){cout<<"This is meber_a;"<<endl;}
   virtual ~member_a(){cout<<"This is ~meber_a;"<<endl;}
};

class member_b{
public:
   member_b(){cout<<"This is member_b;"<<endl;}
   virtual ~member_b(){cout<<"This is ~meber_b;"<<endl;}
};

class derived : public base_b, public base_a
{
public:
   member_a ma;
   derived(){
      cout<<"This is derived;"<<endl;
   }
   virtual ~derived() { cout << "this is ~derived()" << endl; }

private:
   member_b mb;
};

int main(void)
{
  derived de;
  cout << "---------" << endl;
}

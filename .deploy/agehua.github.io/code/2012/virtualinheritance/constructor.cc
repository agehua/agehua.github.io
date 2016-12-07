// 虚基类的构造, 中间类的构造函数被抑制

#include <iostream>
#include <string>

using namespace std;

class top{
    public:
        top(string name):_name(name){
            cout<<"This is top(name); name is "<< _name << endl;
        }
        top():_name("top") {
            cout<<"This is top(); name is "<< _name << endl;
        }
        void printself(){cout << "this is in top::printself, name is " << _name << endl;}
    private:
        string _name;
};

class middle1: virtual public top{
    public:
        middle1(string name):top(name){
            cout << "this is middle1, name is " << name << endl;
        };
};

class middle2: virtual public top{
    public:
        middle2(string name):top(name){;
            cout << "this is middle2, name is " << name << endl;
        }
};

class bottom: public middle1, public middle2{
    public:
        bottom():middle1("bottom1"), middle2("bottom2"){cout<<"This is bottom;"<<endl;}
};

int main(void){
    bottom bo;
    bo.printself();
}

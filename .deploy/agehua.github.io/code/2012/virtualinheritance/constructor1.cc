// 最终类显示调用虚基类的构造函数

#include <iostream>
#include <string>

using namespace std;

class top{
    public:
        top(string name):_name(name){
            cout<<"This is top; name is "<< _name << endl;
        }
        top() {};
        void printself(){
            cout << "this is in top::printself, name is " << _name << endl;
        }
    private:
        string _name;
};

class middle1: virtual public top{
    public:
        middle1(string name):top(name){
            cout << "this is middle1, name is " << name << endl;
        }
    protected:
        middle1(){};
};

class middle2: virtual public top{
    public:
        middle2(string name):top(name){
            cout << "this is middle2, name is " << name << endl;
        }
    protected:
        middle2(){};

};

class bottom: public middle1, public middle2{
    public:
        bottom():top("bottom"){cout<<"This is bottom;"<<endl;}
};

class bottom1: public middle1, public middle2{
    public:
        bottom1():middle1("bazinga"),middle2("bazinga"), top("bottom1"){
            cout<<"This is bottom1;"<<endl;
        }
};

int main(void){
    bottom bo;
    bo.printself();
    cout << "------" << endl;
    bottom1 bo1;
    bo1.printself();
}

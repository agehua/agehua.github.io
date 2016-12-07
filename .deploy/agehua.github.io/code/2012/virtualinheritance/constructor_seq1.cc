// 虚基类的构造顺序

#include <iostream>
#include <string>

using namespace std;

class top{
    public:
        top(string name):_name(name){
            cout<<"This is top; name is "<< _name << endl;
        }
        top() {};
        void printself(){cout << "this is in top::printself, name is " << _name << endl;}
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

class top_b{
    public:
        top_b(){ cout<<"This is top_b"<< endl;}
};

class middle_b: virtual public top_b{
    public:
        middle_b(){cout << "this is middle_b" << endl;}
};

class bottom: public middle1, public middle2, public middle_b{
    public:
        bottom():middle1("bazinga"),middle2("bazinga"), top("bottom1"){cout<<"This is bottom;"<<endl;}
};

int main(void){
    bottom bo;
}

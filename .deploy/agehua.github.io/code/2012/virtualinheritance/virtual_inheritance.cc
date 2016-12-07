// 使用虚拟继承的菱形继承

#include <iostream>

using namespace std;

class top{
    public:
        top(){cout<<"This is top;"<<endl;}
        void printself(){cout << "this is in top::printself" << endl;}
};

class middle1: virtual public top{
};

class middle2: virtual public top{
};

class bottom: public middle1, public middle2{
    public:
        bottom(){cout<<"This is bottom;"<<endl;}
};

int main(void){
    bottom bo;
    bo.printself();
}

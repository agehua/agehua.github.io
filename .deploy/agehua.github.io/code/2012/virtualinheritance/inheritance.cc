// 普继承关系下的菱形继承

#include <iostream>

using namespace std;

class top{
    public:
        top(){cout<<"This is top;"<<endl;}
        void printself(){cout << "this is in top::printself" << endl;}
};

class middle1: public top{
};

class middle2: public top{
};

class bottom: public middle1, public middle2{
    public:
        bottom(){cout<<"This is bottom;"<<endl;}
};

int main(void){
    bottom bo;
    bo.printself();
}

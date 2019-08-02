#include <iostream>
#include "packed.h"

class FirstPointer {
public:
    using VarType=int*;
};

class SecondPointer {
public:
    using VarType=float*;
};

class Boolean {
public:
    using VarType=bool;
};

class SelfPointer;
using SelfPointerExampleClass=Packed<6, Param<SelfPointer, 0, 6>>;
class SelfPointer {
public:
    using VarType=SelfPointerExampleClass*;
};


int main() {
    Packed<13,
           Param<FirstPointer, 0, 6>,
           Param<SecondPointer, 6, 12>,
           Param<Boolean, 12, 13>> packed;
    int a = 5;
    float b = 10;
    packed.Set<FirstPointer>(&a);
    packed.Set<SecondPointer>(&b);
    packed.Set<Boolean>(true);
    std::cout << "size = " << sizeof(packed) << std::endl;
    std::cout << *packed.Get<FirstPointer>() << " " << *packed.Get<SecondPointer>() << " " << packed.Get<Boolean>() << std::endl;
    SelfPointerExampleClass self_pointer_class;
    self_pointer_class.Set<SelfPointer>(&self_pointer_class);
    std::cout << &self_pointer_class << " " << self_pointer_class.Get<SelfPointer>() << std::endl;
    return 0;
}

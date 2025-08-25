// Simple test file with no standard library includes
// Testing basic C++ constructs

class SimpleClass {
private:
    int value;
public:
    SimpleClass(int v) : value(v) {}
    virtual ~SimpleClass() {}
    
    virtual int getValue() const { return value; }
    void setValue(int v) { value = v; }
};

class DerivedClass : public SimpleClass {
public:
    DerivedClass(int v) : SimpleClass(v) {}
    
    int getValue() const override { 
        return SimpleClass::getValue() * 2; 
    }
};

void testFunction() {
    SimpleClass* obj = new DerivedClass(42);
    int result = obj->getValue();
    delete obj;
}

int main() {
    testFunction();
    return 0;
}

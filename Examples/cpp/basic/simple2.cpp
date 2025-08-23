// Second simple test file with no standard library includes

namespace TestNamespace {
    enum Color { RED, GREEN, BLUE };
    
    struct Point {
        int x, y;
        Point(int x = 0, int y = 0) : x(x), y(y) {}
    };
}

template<typename T>
class Container {
private:
    T* data;
    int size;
public:
    Container() : data(nullptr), size(0) {}
    ~Container() { delete[] data; }
    
    void add(const T& item) {
        // Simple add implementation
    }
};

void anotherTestFunction() {
    TestNamespace::Point p(10, 20);
    Container<int> container;
    container.add(42);
}

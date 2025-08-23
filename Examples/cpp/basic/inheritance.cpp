/**
 * @file example_inheritance.cpp
 * @brief Comprehensive example demonstrating C++ inheritance relationships and virtual functions
 * 
 * This example showcases:
 * - Single and multiple inheritance
 * - Virtual functions and polymorphism
 * - Abstract base classes and pure virtual functions
 * - Protected and private inheritance
 * - Virtual destructors
 * - Method overriding and access specifiers
 */
#include <string>

// Base class with virtual functions
class Animal {
private:
    std::string name;
protected:
    int age;
public:
    Animal(const std::string& n, int a) : name(n), age(a) {}
    virtual ~Animal() = default;
    
    // Pure virtual function
    virtual void makeSound() const = 0;
    
    // Virtual function with implementation
    virtual void move() const {
        // Base implementation
    }
    
    // Non-virtual function
    std::string getName() const { return name; }
    
    // Protected virtual for derived access
protected:
    virtual void grow() { age++; }
};

// Single inheritance with public base
class Mammal : public Animal {
protected:
    bool hasFur;
public:
    Mammal(const std::string& n, int a, bool fur) : Animal(n, a), hasFur(fur) {}
    
    // Override pure virtual
    void makeSound() const override {
        // Mammal sound
    }
    
    // Override virtual function
    void move() const override {
        // Mammal movement
    }
    
    // New virtual function
    virtual void breathe() const {
        // Mammal breathing
    }
};

// Multiple inheritance
class Flyable {
public:
    virtual ~Flyable() = default;
    virtual void fly() const = 0;
    virtual double getMaxAltitude() const { return 1000.0; }
};

class Swimmer {
public:
    virtual ~Swimmer() = default;
    virtual void swim() const = 0;
    virtual double getMaxDepth() const { return 100.0; }
};

// Multiple inheritance from three classes
class Bat : public Mammal, public Flyable {
public:
    Bat(const std::string& n, int a) : Mammal(n, a, true) {}
    
    // Override from Animal/Mammal
    void makeSound() const override {
        // Bat sound (echolocation)
    }
    
    // Override from Flyable
    void fly() const override {
        // Bat flight
    }
    
    // Override altitude for bats
    double getMaxAltitude() const override { return 3000.0; }
    
    // Final override - cannot be overridden further
    void breathe() const final override {
        // Bat breathing
    }
};

// Virtual inheritance to solve diamond problem
class WaterBird : public virtual Animal, public Flyable, public Swimmer {
public:
    WaterBird(const std::string& n, int a) : Animal(n, a) {}
    
    void makeSound() const override {
        // Water bird sound
    }
    
    void fly() const override {
        // Water bird flight
    }
    
    void swim() const override {
        // Water bird swimming
    }
};

// Private inheritance
class Duck : private WaterBird {
public:
    Duck(const std::string& n, int a) : Animal(n, a), WaterBird(n, a) {}
    
    // Expose specific functions through public interface
    using WaterBird::fly;
    using WaterBird::swim;
    
    // Override with covariant return type
    virtual Duck* clone() const {
        return new Duck(*this);
    }
};

// Protected inheritance
class Penguin : protected WaterBird {
public:
    Penguin(const std::string& n, int a) : Animal(n, a), WaterBird(n, a) {}
    
    // Cannot fly, so override to do nothing
    void fly() const override {
        // Penguins cannot fly
    }
    
    void swim() const override {
        // Penguin swimming
    }
    
    // Make some protected members public
    using WaterBird::makeSound;
};

// Abstract class with pure virtual destructor
class Shape {
public:
    virtual ~Shape() = 0;
    virtual double area() const = 0;
    virtual double perimeter() const = 0;
};

// Implementation needed even for pure virtual destructor
Shape::~Shape() = default;

// Template inheritance
template<typename T>
class Rectangle : public Shape {
private:
    T width, height;
public:
    Rectangle(T w, T h) : width(w), height(h) {}
    
    double area() const override {
        return static_cast<double>(width * height);
    }
    
    double perimeter() const override {
        return 2.0 * static_cast<double>(width + height);
    }
    
    // Template member function
    template<typename U>
    Rectangle<U> convert() const {
        return Rectangle<U>(static_cast<U>(width), static_cast<U>(height));
    }
};

// Example demonstrating inheritance relationships
void demonstrateInheritance() {
    // Single inheritance
    Bat bat("Bruce", 2);
    bat.makeSound();
    bat.fly();
    
    // Private inheritance - cannot cast to base
    Duck duck("Donald", 3);
    duck.fly();  // Available through using declaration
    duck.swim(); // Available through using declaration
    
    // Template inheritance
    Rectangle<int> intRect(5, 3);
    Rectangle<double> doubleRect = intRect.convert<double>();
    
    double area1 = intRect.area();
    double area2 = doubleRect.area();
    
    // Virtual inheritance
    Penguin penguin("Tux", 1);
    penguin.swim();
    penguin.makeSound(); // Available through using declaration
}

// Function to demonstrate polymorphism
void demonstratePolymorphism() {
    // Only use classes that have public inheritance from Animal
    Bat bat("Vampire", 1);
    bat.makeSound();
    bat.move();
    
    // Note: Duck uses private inheritance, cannot cast to Animal*
    // Note: Penguin uses protected inheritance, cannot cast to Animal*
}

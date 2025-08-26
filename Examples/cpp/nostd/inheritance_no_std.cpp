// Example demonstrating inheritance relationships and virtual functions
// No standard library includes

// Simple string class to replace std::string
class SimpleString {
private:
    char* data;
    int length;
public:
    SimpleString(const char* str = "") {
        length = 0;
        while (str[length] != '\0') length++; // Calculate length
        data = new char[length + 1];
        for (int i = 0; i <= length; i++) {
            data[i] = str[i];
        }
    }
    
    SimpleString(const SimpleString& other) {
        length = other.length;
        data = new char[length + 1];
        for (int i = 0; i <= length; i++) {
            data[i] = other.data[i];
        }
    }
    
    ~SimpleString() { delete[] data; }
    
    const char* c_str() const { return data; }
};

// Base class with virtual functions
class Animal {
private:
    SimpleString name;
protected:
    int age;
public:
    Animal(const SimpleString& n, int a) : name(n), age(a) {}
    virtual ~Animal() = default;
    
    // Pure virtual function
    virtual void makeSound() const = 0;
    
    // Virtual function with implementation
    virtual void move() const {
        // Base implementation
    }
    
    // Non-virtual function
    SimpleString getName() const { return name; }
    
    // Protected virtual for derived access
protected:
    virtual void grow() { age++; }
};

// Single inheritance with public base
class Mammal : public Animal {
protected:
    bool hasFur;
public:
    Mammal(const SimpleString& n, int a, bool fur) : Animal(n, a), hasFur(fur) {}
    
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
    Bat(const SimpleString& n, int a) : Mammal(n, a, true) {}
    
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
    WaterBird(const SimpleString& n, int a) : Animal(n, a) {}
    
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
    Duck(const SimpleString& n, int a) : Animal(n, a), WaterBird(n, a) {}
    
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
    Penguin(const SimpleString& n, int a) : Animal(n, a), WaterBird(n, a) {}
    
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

// Example demonstrating inheritance
void demonstrateInheritance() {
    // Single inheritance
    Bat bat("Bruce", 2);
    bat.makeSound();
    bat.fly();
    
    // Private inheritance - cannot cast to base
    Duck duck("Donald", 3);
    duck.fly();  // Available through using declaration
    duck.swim(); // Available through using declaration
    
    // Protected inheritance
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

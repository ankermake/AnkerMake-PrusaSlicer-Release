#ifndef slic3r_SIGLETON_hpp_
#define slic3r_SIGLETON_hpp_


template <typename T>
class Singleton {
public:
    static T& getInstance() {
        static T instance;
        return instance;
    }

private:
    Singleton() {}
    ~Singleton() {}
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};


#endif
#include "core/PrecompiledHeader.hpp"
#ifndef _KEYHANDLING
#define _KEYHANDLING



class KeyObserver;

class KeyHandling {
   public:
    void addObserver(KeyObserver* observer);
    void removeObserver(KeyObserver* observer);
    void processContinousInput(double deltaTime);
    bool isKeyPressed(int key) const;
    void keyboard_callback(int key, int scancode, int action, int mods);

    void printObservers();

   private:
    std::vector<KeyObserver*> observers;
    std::unordered_map<int, bool> keyStates;
};

#endif

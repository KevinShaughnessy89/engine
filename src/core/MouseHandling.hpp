#include "core/PrecompiledHeader.hpp"
#ifndef _MOUSEHANDLING
#define _MOUSEHANDLING



class MouseObserver;

struct MouseEventData {
  int button = -1;  // -1 indicates no button press
  int action = -1;   // -1 indicates no action
  int mods = 0;     // Modifier keys

  // For cursor movement
  double xposIn = -1.0;
  double yposIn = -1.0;
};


class MouseHandling {

    public:
        void mouseButton_callback(int button, int action, int mods);
        void mouseCursor_callback(double xposIn, double yposIn);
        void addObserver(MouseObserver* observer);
        void removeObserver(MouseObserver* observer);
        void notify(MouseEventData data);
        void printObservers();
        
    private:
    std::vector<MouseObserver*> observers;
};

#endif

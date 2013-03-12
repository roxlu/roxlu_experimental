#include <glfw_wrapper/SimulationBase.h>
#include <roxlu/Roxlu.h>
#include <bmfont/BMFont.h>

class Simulation : public SimulationBase {
 public:
  Simulation();
  void setup();
  void update();
  void draw();
  void onMouseDown(int x, int y, int buton);
  void onMouseUp(int x, int y, int button);
  void onMouseDrag(int x, int y, int dx, int dy, int button);
  void onMouseMove(int x, int y);
  void onChar(int ch); 
  void onKeyDown(int key);
  void onKeyUp(int key);
  void onWindowClose();
 public:
  FPS fps;
  BMFont<> font;
};

cmake_minimum_required(VERSION 2.8)

include(${CMAKE_CURRENT_LIST_DIR}/../../../../../lib/build/cmake/CMakeLists.txt) # roxlu cmake

roxlu_enable_opengl()

roxlu_add_addon(OpenGL)
roxlu_add_addon(Protractor)
roxlu_add_addon(LeapMotion)

roxlu_app_initialize("protractor_gesture_recognizer")
   # ---------------------------------------------
   roxlu_app_add_source_file(main.cpp)
   roxlu_app_add_source_file(Simulation.cpp)
   # ---------------------------------------------
roxlu_install_app()

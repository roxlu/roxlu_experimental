#ifndef ROXLU_UVSPHEREH
#define ROXLU_UVSPHEREH

#include "VertexData.h"

namespace roxlu {

class UVSphere {
public:
	UVSphere();
	~UVSphere();
	void create(float radius, int phi, int theta, VertexData& vertex_data);

private:
};

} // roxlu

#endif
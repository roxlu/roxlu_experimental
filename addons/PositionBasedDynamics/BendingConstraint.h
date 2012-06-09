#ifndef ROXLU_BENDING_CONSTRAINTH
#define ROXLU_BENDING_CONSTRAINTH

class Particle;
class BendingConstraint {
public:
	BendingConstraint(Particle& a, Particle& b, Particle& c, float max);
	void update(const float& dt);
	
	Particle& a;
	Particle& b;
	Particle& c;
	float max;
	float k;
};

#endif
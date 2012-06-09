#include "Snot.h"
#include "PBD.h"

Snot::Snot(Particles& ps)
	:ps(ps)
	,connect_dist(50)
	,break_dist(100)
{
}

void Snot::update() {
	float break_sq = break_dist * break_dist;
	float connect_sq = connect_dist * connect_dist;
	for(int i = 0; i < ps.size(); ++i) {
		Particle& a = *ps[i];
		
		
		for(int j = 0; j < ps.size(); ++j) {
			if(j == i) {
				continue;
			}
		
			// connect			
			Particle& b = *ps[j];
			float ds = (b.position - a.position).lengthSquared();
			if(a.num_springs < 7) {
			
				if(ds < connect_sq) {
					if(!ps.springExists(ps[i], ps[j])) {
						Spring* s = ps.createSpring(ps[i], ps[j]);
						ps.addSpring(s);
					}
				}
			}
			
			if(ds > break_sq) {
				ps.removeSpring(ps[i], ps[j]);
			}
		}
	}
	
}
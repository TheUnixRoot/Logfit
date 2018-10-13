/*****************************************************************************
 * Data Structure
 * **************************************************************************/
#define CELL = 0;
#define BODY = 1;
#define UNUSED = 0;
#define USED = 1;

typedef struct {
	int type; // CELL is 0,  BODY is 1
	int index;
	int used; // O  = unused 1 = used
} OctTreeNode;

typedef struct {
	float mass;
	float posx;
	float posy;
	float posz;
	OctTreeNode more;
	OctTreeNode child[8]; //index of OctTreeNode array.
	float dsq;
	OctTreeNode next;
} OctTreeInternalNode;

typedef struct {
	float mass;
	float posx;
	float posy;
	float posz;
	float velx;
	float vely;
	float velz;
	float accx;
	float accy;
	float accz;
	float dsq;
	OctTreeNode next;
} OctTreeLeafNode;

/*****************************************************************************
* Kernels
* **************************************************************************/

__kernel void sample(int i){

	printf("Hola desde gpu %d", i);

}
											
		__kernel void IterativeForce(__global OctTreeLeafNode *bodies,						
				__global OctTreeInternalNode *tree,										
				int step,																	
				float epssq,																
				float dthf,
				int begin,
				int end														
				) {																		
					float drx, dry, drz, drsq, nphi, scale, idr;							
					float ax, ay, az;													
					int idx = get_global_id(0) + begin; 
															
					if (idx >= end) return;														
						ax = bodies[idx].accx;													
						ay = bodies[idx].accy;													
						az = bodies[idx].accz;													
																							
						bodies[idx].accx = 0.0;													
						bodies[idx].accy = 0.0;													
						bodies[idx].accz = 0.0;													
																							
						OctTreeNode current;													
						current.index = 0;														
						current.type = 0;														
						current.used = 1;														
						//OctTreeNode current = node;											
							while (current.index != -1) {										
								if (current.type == 0) {										
									OctTreeInternalNode n = tree[current.index];				
									drx = n.posx - bodies[idx].posx;							
									dry = n.posy - bodies[idx].posy;							
									drz = n.posz - bodies[idx].posz;							
								} else {														
									OctTreeLeafNode l = bodies[current.index];					
									drx = l.posx - bodies[idx].posx;							
									dry = l.posy - bodies[idx].posy;							
									drz = l.posz - bodies[idx].posz;							
								}																
								drsq = drx * drx + dry * dry + drz * drz;						
								if (current.type == 0) { //The current node is a CELL			
									OctTreeInternalNode n2 = tree[current.index];				
									if (drsq < n2.dsq) { //We go deep using more pointers		
										current = n2.more;										
									} else { //we dont have to go deeper, so next pointer		
										drsq += epssq;											
										idr = 1 / sqrt(drsq);									
										nphi = n2.mass * idr;									
										scale = nphi * idr * idr;								
										bodies[idx].accx += drx * scale;						
										bodies[idx].accy += dry * scale;						
										bodies[idx].accz += drz * scale;						
																							
										current = n2.next;										
									}															
								} else { //the current node is a leaf							
									OctTreeLeafNode l2 = bodies[current.index];					
									if (/*l2*/current.index != idx/*bodies[idx]*/) {			
										drsq += epssq;											
										idr = 1 / sqrt(drsq);									
										nphi = l2.mass * idr;									
										scale = nphi * idr * idr;								
										bodies[idx].accx += drx * scale;						
										bodies[idx].accy += dry * scale;						
										bodies[idx].accz += drz * scale;						
									}															
									current = l2.next;											
								}																
							}																	
						if (step > 0) {															
							bodies[idx].velx += (bodies[idx].accx - ax) * dthf;					
							bodies[idx].vely += (bodies[idx].accy - ay) * dthf;					
							bodies[idx].velz += (bodies[idx].accz - az) * dthf;					
						}																	
				}

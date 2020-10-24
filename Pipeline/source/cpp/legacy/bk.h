
/*****************************************************************************
 * Data Structure
 * **************************************************************************/
const int CELL = 0;
const int BODY = 1;
const int UNUSED = 0;
const int USED = 1;

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
 * Global Variables
 * **************************************************************************/
OctTreeLeafNode *bodies, *bodies2, *aux; // array of bodies
OctTreeInternalNode *tree; //array of internal nodes

/*Global OpenCL variables*/
cl_mem d_tree;
cl_mem d_bodies;

int nbodies; // number of bodies
int timesteps; // number of steps
int nthreads; // number of threads
float dtime; // differential time
float eps; // potential softening parameter
float tol; // should be less than 0.57 for 3D case to bound error
float dthf, epssq, itolsq;
int step;
float gdiameter;
int groot;
int num_cells; //current num_cell used
int num_cmax; //num_cell max to be used

/*****************************************************************************
 * Functions
 * **************************************************************************/

void initialize_tree() {

	//Tree allocation in memory
	tree = (OctTreeInternalNode *) malloc(
			sizeof(OctTreeInternalNode) * nbodies);
	num_cells = 0;
	num_cmax = nbodies;
}

void print_body(OctTreeLeafNode *i) {
	printf("%f %f %f %f %f %f %f %f %f %f\n", i->mass, i->posx, i->posy,
			i->posz, i->velx, i->vely, i->velz, i->accx, i->accy, i->accz);

}

void print_bodies() {
	int i;

	for (i = 0; i < nbodies; i++) {
		print_body(&bodies[i]);
	}
}

void print_tree(OctTreeInternalNode *r) {
	printf("%f %f %f %f\n", r->mass, r->posx, r->posy, r->posz);
	int i = 0;
	while (i < 8 && r->child[i].used == 1) {
		if (r->child[i].type == 0) {
			print_tree(&tree[r->child[i].index]);
		} else {
			print_body(&bodies[r->child[i].index]);
		}
		i++;
	}
}

inline void copy_aux_array() {
	aux = bodies;
	bodies = bodies2;
	bodies2 = aux;
}

inline void copy_body(OctTreeLeafNode *a, OctTreeLeafNode *ch) {
	a->mass = ch->mass;
	a->posx = ch->posx;
	a->posy = ch->posy;
	a->posz = ch->posz;
	a->velx = ch->velx;
	a->vely = ch->vely;
	a->velz = ch->velz;
	a->accx = ch->accx;
	a->accy = ch->accy;
	a->accz = ch->accz;
}

inline void setVelocity(OctTreeLeafNode *b, const float x, const float y, const float z) {
	b->velx = x;
	b->vely = y;
	b->velz = z;
}

inline void ReadInput(char *filename) {
	register float vx, vy, vz;
	register FILE *f;
	register int i;
	OctTreeLeafNode *b;

	f = fopen(filename, "r+t");
	if (f == NULL) {
		fprintf(stderr, "file not found: %s\n", filename);
		exit(-1);
	}

	fscanf(f, "%d", &nbodies);
	fscanf(f, "%d", &timesteps);
	fscanf(f, "%f", &dtime);
	fscanf(f, "%f", &eps);
	fscanf(f, "%f", &tol);

	dthf = 0.5 * dtime;
	epssq = eps * eps;
	itolsq = 1.0 / (tol * tol);

	if (bodies == NULL) {
		fprintf(stderr, "configuration: %d bodies, %d time steps, %d threads\n",
				nbodies, timesteps, nthreads);
		bodies = (OctTreeLeafNode *) malloc(sizeof(OctTreeLeafNode) * nbodies);
		bodies2 = (OctTreeLeafNode *) malloc(sizeof(OctTreeLeafNode) * nbodies);

	}

	for (i = 0; i < nbodies; i++) {
		b = &bodies[i];
		fscanf(f, "%fE", &(b->mass));
		fscanf(f, "%fE", &(b->posx));
		fscanf(f, "%fE", &(b->posy));
		fscanf(f, "%fE", &(b->posz));
		fscanf(f, "%fE", &vx);
		fscanf(f, "%fE", &vy);
		fscanf(f, "%fE", &vz);
		setVelocity(b, vx, vy, vz);
		b->accx = 0.0;
		b->accy = 0.0;
		b->accz = 0.0;
	}
	fclose(f);
}

inline void Printfloat(float d) {
	register int i;
	char str[16];

	sprintf(str, "%.4lE", (double)d);

	i = 0;
	while ((i < 16) && (str[i] != 0)) {
		if ((str[i] == 'E') && (str[i + 1] == '-') && (str[i + 2] == '0')
				&& (str[i + 3] == '0')) {
			printf("E00");
			i += 3;
		} else if (str[i] != '+') {
			printf("%c", str[i]);
		}
		i++;
	}
}

void ComputeCenterAndDiameter(const int n, float & diameter, float & centerx, float & centery, float & centerz) {
	register float minx, miny, minz;
	register float maxx, maxy, maxz;
	register float posx, posy, posz;
	register int i;
	OctTreeLeafNode *b;
	minx = (float) 1.0E32;
	miny = (float) 1.0E32;
	minz = (float) 1.0E32;
	maxx = (float) -1.0E32;
	maxy = (float) -1.0E32;
	maxz = (float) -1.0E32;

	for (i = 0; i < n; i++) {
		b = &bodies[i];
		posx = b->posx;
		posy = b->posy;
		posz = b->posz;

		if (minx > posx)
			minx = posx;
		if (miny > posy)
			miny = posy;
		if (minz > posz)
			minz = posz;

		if (maxx < posx)
			maxx = posx;
		if (maxy < posy)
			maxy = posy;
		if (maxz < posz)
			maxz = posz;
	}

	diameter = maxx - minx;
	if (diameter < (maxy - miny))
		diameter = (maxy - miny);
	if (diameter < (maxz - minz))
		diameter = (maxz - minz);

	centerx = (maxx + minx) * 0.5;
	centery = (maxy + miny) * 0.5;
	centerz = (maxz + minz) * 0.5;
}

int NewNode(const float px, const float py, const float pz) {

	int i;
	OctTreeInternalNode * ptr;
	if (num_cells < num_cmax) {
		ptr = &tree[num_cells];
		ptr->mass = 0.0;
		ptr->posx = px;
		ptr->posy = py;
		ptr->posz = pz;
		for (i = 0; i < 8; i++) {
			ptr->child[i].used = 0;
		}
		num_cells++;
		return num_cells - 1;
	} else {
		fprintf(stderr,
				"You have not enough space for storing a new cell node into tree\n");
		exit(-1);
	}
}

void RecycleTree() { //could be more efficient, just walking throught real used nodes
	//register int i, j;

	for (int i = 0; i < num_cells; i++) {
		for (int j = 0; j < 8; j++) {
			tree[i].child[j].used = 0;
		}
	}
	num_cells = 0;
}

void Insert(OctTreeInternalNode *n, int b, const float r) {
	register OctTreeInternalNode *newcell;
	register OctTreeLeafNode *leaf;
	register int i = 0, ind;
	register float x = 0.0, y = 0.0, z = 0.0;
	register float rh;

	leaf = &bodies[b];
	if (n->posx < leaf->posx) {
		i = 1;
		x = r;
	}
	if (n->posy < leaf->posy) {
		i += 2;
		y = r;
	}
	if (n->posz < leaf->posz) {
		i += 4;
		z = r;
	}
	if (n->child[i].used == 0) { //insert body in empty grid
		n->child[i].used = 1; //used
		n->child[i].index = b; //index of the body
		n->child[i].type = 1; //body
	} else if (n->child[i].type == 0) { //the body it's going to be allocated in a subcell
		Insert(&tree[n->child[i].index], b, 0.5 * r);
	} else { //there is a body where the current body must be
		rh = 0.5 * r;
		ind = NewNode(n->posx - rh + x, n->posy - rh + y, n->posz - rh + z);
		newcell = &tree[ind];
		Insert(newcell, b, rh);
		Insert(newcell, n->child[i].index, rh);
		n->child[i].type = 0; //type=cell
		n->child[i].index = ind;
	}
}

// cargamos en los nodos internos la informacion de los subarboles => solucionar
int ComputeCenterOfMass(OctTreeInternalNode *n, int curr) {
	register float m, px = 0.0, py = 0.0, pz = 0.0;
	register int ch;
	register int i, j = 0;
	OctTreeLeafNode *b;
	OctTreeInternalNode *c;

	n->mass = 0.0;
	for (i = 0; i < 8; i++) {
		ch = n->child[i].index;
		if (n->child[i].used == 1) { //this child is used
			// move non-NULL children to the front (needed to make other code faster)
			n->child[i].used = 0;
			n->child[j].index = n->child[i].index;
			n->child[j].type = n->child[i].type;
			n->child[j].used = 1;

			if (n->child[i].type == 1) { //type == body
				// sort bodies in tree order (approximation of putting nearby nodes together for locality)
				copy_body(&bodies2[curr], &bodies[ch]);
				n->child[j].index = curr;
				curr = curr + 1;
			} else {
				curr = ComputeCenterOfMass(&tree[ch], curr);
			}

			if (n->child[i].type == 0) { //cell
				c = &tree[ch];
				m = tree[ch].mass;
				n->mass += m;
				px += c->posx * m;
				py += c->posy * m;
				pz += c->posz * m;

			} else { //body
				b = &bodies[ch];
				m = b->mass;
				n->mass += m;
				px += b->posx * m;
				py += b->posy * m;
				pz += b->posz * m;
			}
			j = j + 1;
		}
	}

	m = 1.0 / n->mass;
	n->posx = px * m;
	n->posy = py * m;
	n->posz = pz * m;

	return curr;
}

void ComputeOpeningCriteriaForEachCell(OctTreeInternalNode *node, float dsq) {
	int i = 0;

	node->dsq = dsq;
	dsq *= 0.25;

	while (i < 8 && node->child[i].used == USED) {
		if (node->child[i].type == CELL) {
			ComputeOpeningCriteriaForEachCell(&tree[node->child[i].index], dsq);
		} else {
			bodies[node->child[i].index].dsq = dsq;
		}
		i++;
	}

}

int count_sons_number(OctTreeInternalNode *father) {
	int i = 0;
	int res = 0;

	while (i < 8) {
		if (father->child[i].used == USED) {
			res++;
		}
		i++;
	}
	return res;
}

void ComputeNextMorePointers(OctTreeInternalNode *node, OctTreeNode next) {

	int number_sons = 0, i;

	//setting the next pointer
	node->next = next;
	//setting the more pointer
	node->more = node->child[0];

	number_sons = count_sons_number(node);
	for (i = 0; i < number_sons - 1; i++) {
		if (node->child[i].type == CELL) {
			ComputeNextMorePointers(&tree[node->child[i].index],
					node->child[i + 1]);
		} else {
			bodies[node->child[i].index].next = node->child[i + 1];
		}
	}
	if (node->child[i].type == CELL) {
		ComputeNextMorePointers(&tree[node->child[i].index], next);
	} else {
		bodies[node->child[i].index].next = next;
	}

}

int nodeCounter(int n) {

	int acumulator = 0;
	int i;

	for (i = 0; i < 8; i++) {
		if (tree[n].child[i].used != 0) {
			if (tree[n].child[i].type == 0) { //it's a cell
				acumulator = acumulator + nodeCounter(tree[n].child[i].index);
			} else { //it's a body
				acumulator = acumulator + 1;
			}
		}
	}
	return 1 + acumulator;
}

// actualizamos la velocidad y posicion de un cuerpo de un cuerpo en un paso
void Advance(OctTreeLeafNode *b) {
	register float dvelx, dvely, dvelz;
	register float velhx, velhy, velhz;

	dvelx = b->accx * dthf;
	dvely = b->accy * dthf;
	dvelz = b->accz * dthf;

	velhx = b->velx + dvelx;
	velhy = b->vely + dvely;
	velhz = b->velz + dvelz;

	b->posx += velhx * dtime;
	b->posy += velhy * dtime;
	b->posz += velhz * dtime;

	b->velx = velhx + dvelx;
	b->vely = velhy + dvely;
	b->velz = velhz + dvelz;
}

// recursivamente calculamos la fuerza que ejercen todos lo planetas sobre uno
void RecurseForce(int n, int t, OctTreeLeafNode *b, float dsq) {
	register float drx, dry, drz, drsq, nphi, scale, idr;
	OctTreeInternalNode *cell;
	OctTreeLeafNode *leaf;

	if (t == 0) { //n is a cell
		cell = &tree[n];
		drx = cell->posx - b->posx;
		dry = cell->posy - b->posy;
		drz = cell->posz - b->posz;

		drsq = drx * drx + dry * dry + drz * drz;
		if (drsq < dsq) {

			dsq *= 0.25;
			if (cell->child[0].used == 1) {
				RecurseForce(cell->child[0].index, cell->child[0].type, b, dsq);
				if (cell->child[1].used == 1) {
					RecurseForce(cell->child[1].index, cell->child[1].type, b,
							dsq);
					if (cell->child[2].used == 1) {
						RecurseForce(cell->child[2].index, cell->child[2].type,
								b, dsq);
						if (cell->child[3].used == 1) {
							RecurseForce(cell->child[3].index,
									cell->child[3].type, b, dsq);
							if (cell->child[4].used == 1) {
								RecurseForce(cell->child[4].index,
										cell->child[4].type, b, dsq);
								if (cell->child[5].used == 1) {
									RecurseForce(cell->child[5].index,
											cell->child[5].type, b, dsq);
									if (cell->child[6].used == 1) {
										RecurseForce(cell->child[6].index,
												cell->child[6].type, b, dsq);
										if (cell->child[7].used == 1) {
											RecurseForce(cell->child[7].index,
													cell->child[7].type, b,
													dsq);
										}
									}
								}
							}
						}
					}
				}
			}

		} else { // node is far enough away, don't recurse any deeper
			drsq += epssq;
			idr = 1 / sqrt(drsq);
			nphi = cell->mass * idr;
			scale = nphi * idr * idr;
			b->accx += drx * scale;
			b->accy += dry * scale;
			b->accz += drz * scale;
		}

	} else { //n is a body
		leaf = &bodies[n];
		drx = leaf->posx - b->posx;
		dry = leaf->posy - b->posy;
		drz = leaf->posz - b->posz;

		drsq = drx * drx + dry * dry + drz * drz;
		if (drsq < dsq) {
			if (leaf != b) {
				drsq += epssq;
				idr = 1 / sqrt(drsq);
				nphi = leaf->mass * idr;
				scale = nphi * idr * idr;
				b->accx += drx * scale;
				b->accy += dry * scale;
				b->accz += drz * scale;
			}
		} else { // node is far enough away, don't recurse any deeper

			drsq += epssq;
			idr = 1 / sqrt(drsq);
			nphi = leaf->mass * idr;
			scale = nphi * idr * idr;
			b->accx += drx * scale;
			b->accy += dry * scale;
			b->accz += drz * scale;
		}
	}
}

void IterativeForce(OctTreeLeafNode *body) {
	register float drx, dry, drz, drsq, nphi, scale, idr;

	//root node
	OctTreeNode node;
	node.index = 0;
	node.type = CELL;
	node.used = 1;

	OctTreeNode *current = &node;

	while (current->index != -1) {
		//distance's calculation
		if (current->type == CELL) {
			OctTreeInternalNode *n = &tree[current->index];
			drx = n->posx - body->posx;
			dry = n->posy - body->posy;
			drz = n->posz - body->posz;
		} else {
			OctTreeLeafNode *l = &bodies[current->index];
			drx = l->posx - body->posx;
			dry = l->posy - body->posy;
			drz = l->posz - body->posz;
		}

		drsq = drx * drx + dry * dry + drz * drz;

		if (current->type == CELL) { //The current node is a CELL
			OctTreeInternalNode *n2 = &tree[current->index];
			if (drsq < n2->dsq) { //We go deep using more pointers
				current = &n2->more;
			} else { //we dont have to go deeper, so we use next pointer
				drsq += epssq;
				idr = 1 / sqrt(drsq);
				nphi = n2->mass * idr;
				scale = nphi * idr * idr;
				body->accx += drx * scale;
				body->accy += dry * scale;
				body->accz += drz * scale;

				current = &n2->next;
			}
		} else { //the current node is a leaf
			OctTreeLeafNode * l2 = &bodies[current->index];
			if (l2 != body) {
				drsq += epssq;
				idr = 1 / sqrt(drsq);
				nphi = l2->mass * idr;
				scale = nphi * idr * idr;
				body->accx += drx * scale;
				body->accy += dry * scale;
				body->accz += drz * scale;
			}
			current = &l2->next;
		}
	}
}

// calculamos la velocidad y posicion de un cuerpo para un paso de tiempo
void ComputeForce(int root, OctTreeLeafNode *b, const float size) {
	register float ax, ay, az;

	ax = b->accx;
	ay = b->accy;
	az = b->accz;

	b->accx = 0.0;
	b->accy = 0.0;
	b->accz = 0.0;

	//RecurseForce(root, 0, b, size * size * itolsq);
	//IterativeForce(root, 0, b, size * size * itolsq);
	IterativeForce(b);

	if (step > 0) {
		b->velx += (b->accx - ax) * dthf;
		b->vely += (b->accy - ay) * dthf;
		b->velz += (b->accz - az) * dthf;
	}
}

#endif
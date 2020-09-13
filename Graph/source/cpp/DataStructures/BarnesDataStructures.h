//
// Created by juanp on 8/7/19.
//

#ifndef BARNESLOGFIT_BARNESDATASTRUCTURES_H
#define BARNESLOGFIT_BARNESDATASTRUCTURES_H

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <math.h>
#include "ProvidedDataStructures.h"


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
using dim_range = std::vector<size_t>;
using t_index = struct _t_index{
    int begin, end;
};
using buffer_OctTreeLeafNode = tbb::flow::opencl_buffer<OctTreeLeafNode>;
using buffer_OctTreeInternalNode = tbb::flow::opencl_buffer<OctTreeInternalNode>;
using type_gpu_node = tbb::flow::tuple<t_index, buffer_OctTreeLeafNode, buffer_OctTreeInternalNode, int, float, float>;

namespace BarnesHutDataStructures {
/*****************************************************************************
 * Global Variables
 * **************************************************************************/
OctTreeLeafNode *access_bodies;
OctTreeInternalNode *tree; //array of internal nodes

/*Global OpenCL variables*/
buffer_OctTreeInternalNode d_tree;
buffer_OctTreeLeafNode bodies, bodies2;

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

void RecycleTree() { //could be more efficient, just walking throught real used nodes
    //register int i, j;

    for (int i = 0; i < num_cells; i++) {
        for (int j = 0; j < 8; j++) {
            d_tree[i].child[j].used = UNUSED;
        }
    }
    num_cells = 0;
}

void initialize_tree() {

    //Tree allocation in memory
    num_cells = nbodies;
    num_cmax = nbodies;
    d_tree = buffer_OctTreeInternalNode(num_cmax);
    tree = d_tree.data();
    RecycleTree();
}

void print_body(const OctTreeLeafNode &i) {
    printf("%f %f %f %f %f %f %f %f %f %f\n", i.mass, i.posx, i.posy,
           i.posz, i.velx, i.vely, i.velz, i.accx, i.accy, i.accz);

}

void print_bodies() {
    for (int i = 0; i < nbodies; i++) {
        print_body(bodies[i]);
    }
}

void print_tree(const OctTreeInternalNode &r) {
    printf("%f %f %f %f\n", r.mass, r.posx, r.posy, r.posz);
    int i = 0;
    while (i < 8 && r.child[i].used == 1) {
        if (r.child[i].type == CELL) {
            print_tree(d_tree[r.child[i].index]);
        } else {
            print_body(bodies[r.child[i].index]);
        }
        i++;
    }
}

inline void copy_to_bodies() {
    for (size_t i = 0; i < nbodies; ++i) {
        bodies[i] = bodies2[i];
    }
}

inline void copy_body(OctTreeLeafNode &a, const OctTreeLeafNode &ch) {
    a.mass = ch.mass;
    a.posx = ch.posx;
    a.posy = ch.posy;
    a.posz = ch.posz;
    a.velx = ch.velx;
    a.vely = ch.vely;
    a.velz = ch.velz;
    a.accx = ch.accx;
    a.accy = ch.accy;
    a.accz = ch.accz;
}

inline void ReadInput(char *filename) {
    register FILE *f;
    register int i;

    if ((f = fopen(filename, "r+t")) == NULL) {
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

    if (access_bodies == NULL) {
        fprintf(stdout, "configuration: %d bodies, %d time steps, %d threads\n",
                nbodies, timesteps, nthreads);
        bodies = buffer_OctTreeLeafNode(nbodies);
        access_bodies = bodies.data();
        bodies2 = buffer_OctTreeLeafNode(nbodies);
//        bodies2 = d_bodies2.data();

    }

    for (i = 0; i < nbodies; i++) {
        OctTreeLeafNode &b = bodies[i];
        fscanf(f, "%fE", &(b.mass));
        fscanf(f, "%fE", &(b.posx));
        fscanf(f, "%fE", &(b.posy));
        fscanf(f, "%fE", &(b.posz));
        fscanf(f, "%fE", &(b.velx));
        fscanf(f, "%fE", &(b.vely));
        fscanf(f, "%fE", &(b.velz));
        b.accx = 0.0;
        b.accy = 0.0;
        b.accz = 0.0;
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
    minx = (float) 1.0E32;
    miny = (float) 1.0E32;
    minz = (float) 1.0E32;
    maxx = (float) -1.0E32;
    maxy = (float) -1.0E32;
    maxz = (float) -1.0E32;

    for (i = 0; i < n; i++) {
        OctTreeLeafNode &b = bodies[i];
        posx = b.posx;
        posy = b.posy;
        posz = b.posz;

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
    OctTreeInternalNode &ptr = d_tree[num_cells];
    if (num_cells < num_cmax) {
        ptr.mass = 0.0;
        ptr.posx = px;
        ptr.posy = py;
        ptr.posz = pz;
        num_cells++;
        return num_cells - 1;
    } else {
        fprintf(stderr,
                "You have not enough space for storing a new cell node into tree\n");
        exit(-1);
    }
}

void Insert(OctTreeInternalNode &n, int index, const float radius) {
    register int i = 0, ind;
    register float x = 0.0, y = 0.0, z = 0.0;
    register float rh;

    OctTreeLeafNode &leaf = bodies[index];
    if (n.posx < leaf.posx) {
        i = 1;
        x = radius;
    }
    if (n.posy < leaf.posy) {
        i += 2;
        y = radius;
    }
    if (n.posz < leaf.posz) {
        i += 4;
        z = radius;
    }
    if (n.child[i].used == UNUSED) { //insert body in empty grid
        n.child[i].used = USED; //used
        n.child[i].index = index; //index of the body
        n.child[i].type = BODY; //body
    } else if (n.child[i].type == CELL) { //the body it's going to be allocated in a subcell
        Insert(d_tree[n.child[i].index], index, 0.5 * radius);
    } else { //there is a body where the current body must be
        rh = 0.5 * radius;
        ind = NewNode(n.posx - rh + x, n.posy - rh + y, n.posz - rh + z);
        OctTreeInternalNode &newcell = d_tree[ind];
        Insert(newcell, index, rh);
        Insert(newcell, n.child[i].index, rh);
        n.child[i].type = CELL; //type=cell
        n.child[i].index = ind;
    }
}

// cargamos en los nodos internos la informacion de los subarboles => solucionar
int ComputeCenterOfMass(OctTreeInternalNode &n, int curr) {
    register float m, px = 0.0, py = 0.0, pz = 0.0;
    register int ch;
    register int i, j = 0;

    n.mass = 0.0;
    for (i = 0; i < 8; i++) {
        ch = n.child[i].index;
        if (n.child[i].used == USED) { //this child is used
            // move non-NULL children to the front (needed to make other code faster)
            n.child[i].used = UNUSED;
            n.child[j].index = n.child[i].index;
            n.child[j].type = n.child[i].type;
            n.child[j].used = USED;

            if (n.child[i].type == BODY) { //type == body
                // sort bodies in tree order (approximation of putting nearby nodes together for locality)
                copy_body(bodies2[curr], bodies[ch]);
                n.child[j].index = curr;
                curr = curr + 1;
            } else {
                curr = ComputeCenterOfMass(d_tree[ch], curr);
            }

            if (n.child[i].type == CELL) { //cell
                OctTreeInternalNode &c = d_tree[ch];
                m = d_tree[ch].mass;
                n.mass += m;
                px += c.posx * m;
                py += c.posy * m;
                pz += c.posz * m;

            } else { //body
                OctTreeLeafNode &b = bodies[ch];
                m = b.mass;
                n.mass += m;
                px += b.posx * m;
                py += b.posy * m;
                pz += b.posz * m;
            }
            j = j + 1;
        }
    }

    m = 1.0 / n.mass;
    n.posx = px * m;
    n.posy = py * m;
    n.posz = pz * m;

    return curr;
}

void ComputeOpeningCriteriaForEachCell(OctTreeInternalNode &node, float dsq) {
    int i = 0;

    node.dsq = dsq;
    dsq *= 0.25;

    while (i < 8 && node.child[i].used == USED) {
        if (node.child[i].type == CELL) {
            ComputeOpeningCriteriaForEachCell(d_tree[node.child[i].index], dsq);
        } else {
            bodies[node.child[i].index].dsq = dsq;
        }
        i++;
    }

}

int count_sons_number(const OctTreeInternalNode &father) {
    int i = 0;
    int res = 0;

    while (i < 8) {
        if (father.child[i].used == USED) {
            res++;
        }
        i++;
    }
    return res;
}

void ComputeNextMorePointers(OctTreeInternalNode &node, OctTreeNode &next) {

    int number_sons = 0, i;

    //setting the next pointer
    node.next = next;
    //setting the more pointer
    node.more = node.child[0];

    number_sons = count_sons_number(node);
    for (i = 0; i < number_sons - 1; i++) {
        if (node.child[i].type == CELL) {
            ComputeNextMorePointers(d_tree[node.child[i].index],
                                    node.child[i + 1]);
        } else {
            bodies[node.child[i].index].next = node.child[i + 1];
        }
    }
    if (node.child[i].type == CELL) {
        ComputeNextMorePointers(d_tree[node.child[i].index], next);
    } else {
        bodies[node.child[i].index].next = next;
    }

}

int nodeCounter(int n) {

    int acumulator = 0;
    int i;

    for (i = 0; i < 8; i++) {
        if (d_tree[n].child[i].used == USED) {
            if (d_tree[n].child[i].type == CELL) { //it's a cell
                acumulator = acumulator + nodeCounter(d_tree[n].child[i].index);
            } else { //it's a body
                acumulator = acumulator + 1;
            }
        }
    }
    return 1 + acumulator;
}

// actualizamos la velocidad y posicion de un cuerpo de un cuerpo en un paso
void Advance(OctTreeLeafNode &b) {
    register float dvelx, dvely, dvelz;
    register float velhx, velhy, velhz;

    dvelx = b.accx * dthf;
    dvely = b.accy * dthf;
    dvelz = b.accz * dthf;

    velhx = b.velx + dvelx;
    velhy = b.vely + dvely;
    velhz = b.velz + dvelz;

    b.posx += velhx * dtime;
    b.posy += velhy * dtime;
    b.posz += velhz * dtime;

    b.velx = velhx + dvelx;
    b.vely = velhy + dvely;
    b.velz = velhz + dvelz;
}

}
// calculamos la velocidad y posicion de un cuerpo para un paso de tiempo
void ComputeForce(OctTreeLeafNode *body, OctTreeLeafNode *bodies, OctTreeInternalNode *tree,
                  const float step, const float epssq, const float dthf) {
    float ax, ay, az;

    ax = body->accx;
    ay = body->accy;
    az = body->accz;

    body->accx = 0.0;
    body->accy = 0.0;
    body->accz = 0.0;

    //RecurseForce(root, 0, body, size * size * itolsq);
    //IterativeForce(root, 0, body, size * size * itolsq);
    float drx, dry, drz, drsq, nphi, scale, idr;

    //root node
    OctTreeNode node;
    node.index = 0;
    node.type = CELL;
    node.used = USED;

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
    if (step > 0) {
        body->velx += (body->accx - ax) * dthf;
        body->vely += (body->accy - ay) * dthf;
        body->velz += (body->accz - az) * dthf;
    }
}

#endif //BARNESLOGFIT_BARNESDATASTRUCTURES_H

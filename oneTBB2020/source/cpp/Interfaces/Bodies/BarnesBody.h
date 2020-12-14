//
// Created by juanp on 8/7/19.
//

#ifndef LOGFIT_BARNESBODY_H
#define LOGFIT_BARNESBODY_H

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <math.h>
#include <utils/Utils.h>
#include <iostream>
/*****************************************************************************
 * Shared Data Structure
 * **************************************************************************/
using space_type = enum {
    CELL = 0, BODY = 1
};
using used_type = enum {
    UNUSED = 0, USED = 1
};

using oct_tree_node = struct {
    space_type type; // CELL is 0,  BODY is 1
    int index;
    used_type used; // O  = UNUSED 1 = USED
};

using oct_tree_cell = struct {
    float mass;
    float posx;
    float posy;
    float posz;
    float dsq;
    oct_tree_node child[8];
    oct_tree_node *more;
    oct_tree_node *next;
};

using oct_tree_body = struct {
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
    oct_tree_node *next;
};
inline unsigned long ReadNBodies(char *filename) {
    FILE *f;
    int i;
    if ((f = fopen(filename, "r+t")) == NULL) {
        fprintf(stderr, "file not found: %s\n", filename);
        exit(-1);
    }
    unsigned long nbodies;
    fscanf(f, "%ld", &nbodies);
    fclose(f);
    return nbodies;
}
/*****************************************************************************
 * class Body
 * **************************************************************************/
template<typename body_vector, typename tree_vector>
class BarnesBody {

public:
    body_vector bodies, bodies2, aux; // array of bodies
    tree_vector tree; //array of internal nodes
    int nbodies; // number of bodies
    float dthf, epssq, itolsq;
    int step;

    int timesteps; // number of steps
    int nthreads; // number of threads
    float dtime; // differential time
    float eps; // potential softening parameter
    float tol; // should be less than 0.57 for 3D case to bound error
    float gdiameter;
    int groot;
    int num_cells; //current num_cell used
    int num_cmax; //num_cell max to be used
    bool firsttime;
    static void ComputeForce(oct_tree_body &body, const body_vector &bodies, const tree_vector &tree,
                             const float step, const float epssq, const float dthf) {
        float ax, ay, az;

        ax = body.accx;
        ay = body.accy;
        az = body.accz;

        body.accx = 0.0;
        body.accy = 0.0;
        body.accz = 0.0;

        //RecurseForce(root, 0, body, size * size * itolsq);
        //IterativeForce(root, 0, body, size * size * itolsq);
        float drx, dry, drz, drsq, nphi, scale, idr;

        //root node
        oct_tree_node node;
        node.index = 0;
        node.type = CELL;
        node.used = USED;

        oct_tree_node *current = &node;

        while (current->index != -1) {
            //distance's calculation
            if (current->type == CELL) {
                const oct_tree_cell &n = tree[current->index];
                drx = n.posx - body.posx;
                dry = n.posy - body.posy;
                drz = n.posz - body.posz;
            } else {
                const oct_tree_body &l = bodies[current->index];
                drx = l.posx - body.posx;
                dry = l.posy - body.posy;
                drz = l.posz - body.posz;
            }

            drsq = drx * drx + dry * dry + drz * drz;

            if (current->type == CELL) { //The current node is a CELL
                const oct_tree_cell &n2 = tree[current->index];
                if (drsq < n2.dsq) { //We go deep using more pointers
                    current = n2.more;
                } else { //we dont have to go deeper, so we use next pointer
                    drsq += epssq;
                    idr = 1 / sqrt(drsq);
                    nphi = n2.mass * idr;
                    scale = nphi * idr * idr;
                    body.accx += drx * scale;
                    body.accy += dry * scale;
                    body.accz += drz * scale;
                    current = n2.next;
                }
            } else { //the current node is a leaf
                const oct_tree_body &l2 = bodies[current->index];
                if (&l2 != &body) {
                    drsq += epssq;
                    idr = 1 / sqrt(drsq);
                    nphi = l2.mass * idr;
                    scale = nphi * idr * idr;
                    body.accx += drx * scale;
                    body.accy += dry * scale;
                    body.accz += drz * scale;
                }
                current = l2.next;
            }
        }
        if (step > 0) {
            body.velx += (body.accx - ax) * dthf;
            body.vely += (body.accy - ay) * dthf;
            body.velz += (body.accz - az) * dthf;
        }
    }
    BarnesBody(bool firsttime, Params p, unsigned long nbodies) : firsttime{firsttime},
        nthreads{static_cast<int>(p.numcpus+p.numgpus)}, num_cells{0}, num_cmax{static_cast<int>(nbodies)} { }

    void ComputeCenterAndDiameter(float &diameter, float &centerx, float &centery, float &centerz) {
        float minx, miny, minz;
        float maxx, maxy, maxz;
        float posx, posy, posz;
        int i;
        minx = (float) 1.0E32;
        miny = (float) 1.0E32;
        minz = (float) 1.0E32;
        maxx = (float) -1.0E32;
        maxy = (float) -1.0E32;
        maxz = (float) -1.0E32;

        for (i = 0; i < nbodies; i++) {
            const oct_tree_body &b = bodies[i];
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

    int NewCell(const float px, const float py, const float pz) {
        if (num_cells < num_cmax) {
            tree[num_cells].mass = 0.0;
            tree[num_cells].posx = px;
            tree[num_cells].posy = py;
            tree[num_cells].posz = pz;
            return num_cells++;
        } else {
            fprintf(stderr,
                    "You have not enough space for storing a new cell node into tree\n");
            exit(-1);
        }
    }

    void Insert(oct_tree_cell &root, int index_in_bodies, const float radius) {
        int i = 0;
        float x = 0.0, y = 0.0, z = 0.0;
        float rh;

        const oct_tree_body &body = bodies[index_in_bodies];
        if (root.posx < body.posx) {
            i = 1;
            x = radius;
        }
        if (root.posy < body.posy) {
            i += 2;
            y = radius;
        }
        if (root.posz < body.posz) {
            i += 4;
            z = radius;
        }
        if (root.child[i].used == UNUSED) { //insert body in empty grid
            root.child[i].used = USED; //used
            root.child[i].index = index_in_bodies; //index_in_bodies of the body
            root.child[i].type = BODY; //body
        } else if (root.child[i].type == CELL) { //the body it's going to be allocated in a subcell
            Insert(tree[root.child[i].index], index_in_bodies, 0.5 * radius);
        } else { //there is a body where the current body must be
            rh = 0.5 * radius;
            int new_cell_index = NewCell(root.posx - rh + x, root.posy - rh + y, root.posz - rh + z);
            oct_tree_cell &newcell = tree[new_cell_index];
            Insert(newcell, index_in_bodies, rh);
            Insert(newcell, root.child[i].index, rh);
            root.child[i].type = CELL; //type=cell
            root.child[i].index = new_cell_index;
        }
    }

    int ComputeCenterOfMass(oct_tree_cell &root_cell, int curr) {
        float m, px = 0.0, py = 0.0, pz = 0.0;
        int j = 0;
        root_cell.mass = 0.0;
        for (int i = 0; i < 8; i++) {
            int child_index = root_cell.child[i].index;
            auto & body{bodies[child_index]};
            auto & cell{tree[child_index]};

            if (root_cell.child[i].used == USED) { //this child_index is used
                // move non-NULL children to the front (needed to make other code faster)
                root_cell.child[i].used = UNUSED;
                root_cell.child[j].index = root_cell.child[i].index;
                root_cell.child[j].type = root_cell.child[i].type;
                root_cell.child[j].used = USED;

                if (root_cell.child[i].type == BODY) { //type == body
                    // sort bodies in tree order (approximation of putting nearby nodes together for locality)
                    copy_body_from_to(body, bodies2[curr]);
                    root_cell.child[j].index = curr;
                    curr = curr + 1;
                } else {
                    curr = ComputeCenterOfMass(cell, curr);
                }

                if (root_cell.child[i].type == CELL) {
                    m = cell.mass;
                    root_cell.mass += m;
                    px += cell.posx * m;
                    py += cell.posy * m;
                    pz += cell.posz * m;

                } else {
                    m = body.mass;
                    root_cell.mass += m;
                    px += body.posx * m;
                    py += body.posy * m;
                    pz += body.posz * m;
                }
                j++;
            }
        }

        m = 1.0 / root_cell.mass;
        root_cell.posx = px * m;
        root_cell.posy = py * m;
        root_cell.posz = pz * m;

        return curr;
    }

    inline void copy_from_to_bodies() {
        for (size_t i = 0; i < nbodies; ++i) {
            bodies[i] = bodies2[i];
        }
    }

    void ComputeOpeningCriteriaForEachCell(oct_tree_cell &node, float dsq) {
        int i = 0;

        node.dsq = dsq;
        dsq *= 0.25;

        while (i < 8 && node.child[i].used == USED) {
            if (node.child[i].type == CELL) {
                ComputeOpeningCriteriaForEachCell(tree[node.child[i].index], dsq);
            } else {
                bodies[node.child[i].index].dsq = dsq;
            }
            i++;
        }

    }

    void ComputeNextMorePointers(oct_tree_cell &node, oct_tree_node &next) {

        int number_sons = 0, i;

        //setting the next pointer
        node.next = &next;
        //setting the more pointer
        node.more = &node.child[0];

        number_sons = count_sons_number(node);
        for (i = 0; i < number_sons - 1; i++) {
            if (node.child[i].type == CELL) {
                ComputeNextMorePointers(tree[node.child[i].index],
                                        node.child[i + 1]);
            } else {
                bodies[node.child[i].index].next = &node.child[i + 1];
            }
        }
        if (node.child[i].type == CELL) {
            ComputeNextMorePointers(tree[node.child[i].index], next);
        } else {
            bodies[node.child[i].index].next = &next;
        }
    }

    void RecycleTree(int size) { //could be more efficient, just walking throught real used nodes
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < 8; j++) {
                tree[i].child[j].index = -1;
                tree[i].child[j].type = CELL;
                tree[i].child[j].used = UNUSED;
            }
        }
        num_cells = 0;
    }

    void Advance(oct_tree_body &b) {
        float dvelx, dvely, dvelz;
        float velhx, velhy, velhz;

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
protected:

    virtual inline void ReadInput(char *filename) = 0;

    inline void Printfloat(float d) {
        int i;
        char str[16];

        sprintf(str, "%.4lE", (double) d);

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

    inline void copy_body_from_to(const oct_tree_body &ch, oct_tree_body &a) {
        a.mass = ch.mass;
        a.posx = ch.posx;
        a.posy = ch.posy;
        a.posz = ch.posz;
        a.velx = ch.velx;
        a.vely = ch.vely;
        a.velz = ch.velz;
        a.accx = ch.accx;
        a.accy = ch.accy;
    }

    inline void print_body(const oct_tree_body &i) {
        printf("%f %f %f %f %f %f %f %f %f %f\n", i.mass, i.posx, i.posy,
               i.posz, i.velx, i.vely, i.velz, i.accx, i.accy, i.accz);

    }

    inline void print_bodies() {
        for (int i = 0; i < nbodies; i++) {
            print_body(bodies[i]);
        }
    }

    inline void print_tree(const oct_tree_cell &r) {
        printf("%f %f %f %f\n", r.mass, r.posx, r.posy, r.posz);
        int i = 0;
        while (i < 8 && r.child[i].used == 1) {
            if (r.child[i].type == CELL) {
                print_tree(tree[r.child[i].index]);
            } else {
                print_body(bodies[r.child[i].index]);
            }
            i++;
        }
    }

    inline int count_sons_number(const oct_tree_cell &father) {
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

    inline int nodeCounter(int n) {

        int acumulator = 0;
        int i;

        for (i = 0; i < 8; i++) {
            if (tree[n].child[i].used == USED) {
                if (tree[n].child[i].type == CELL) { //it's a cell
                    acumulator = acumulator + nodeCounter(tree[n].child[i].index);
                } else { //it's a body
                    acumulator = acumulator + 1;
                }
            }
        }
        return 1 + acumulator;
    }
};
#endif //LOGFIT_BARNESBODY_H
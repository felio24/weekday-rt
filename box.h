#ifndef BOX_H
#define BOX_H

#include "hittable.h"
using namespace std;

class box : public hittable {
  public:
    box(const point3& box_min, const point3& box_max, shared_ptr<material> mat)
      : box_min(box_min), box_max(box_max), mat(mat) {}
    
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        double t_enter = ray_t.min;  // running max of all slab entry t values
        double t_exit  = ray_t.max;  // running min of all slab exit  t values
        int    hit_axis = -1;        // which axis produced the final t_enter
        bool   hit_min  = true;      // did we enter through the min face of that axis?

        // For each axis, compute the two t values where the ray crosses that slab.
        // We keep a running [t_enter, t_exit] window; if it ever becomes empty, no hit.
        for (int axis = 0; axis < 3; axis++) {
            double inv_d = 1.0 / r.direction()[axis];
            double origin = r.origin()[axis];

            // t at the near and far face of this slab
            double t0 = (box_min[axis] - origin) * inv_d;
            double t1 = (box_max[axis] - origin) * inv_d;

            // Swap so t0 < t1 (handles negative ray direction on this axis)
            bool swapped = false;
            if (t0 > t1) {
                swap(t0, t1);
                swapped = true;
            }

            // Narrow the hit window
            if (t0 > t_enter) {
                t_enter   = t0;
                hit_axis  = axis;
                // After swap: if swapped, the ray enters through the MAX face;
                // otherwise it enters through the MIN face.
                hit_min   = !swapped;
            }
            t_exit = fmin(t_exit, t1);

            // Early-out: the intervals no longer overlap
            if (t_enter >= t_exit)
                return false;
        }

        // t_enter is the first intersection within ray_t
        rec.t = t_enter;
        rec.p = r.at(rec.t);

        // Build the outward normal: +1 or -1 on whichever axis we entered through
        vec3 outward_normal(0, 0, 0);
        outward_normal[hit_axis] = hit_min ? -1.0 : 1.0;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }
  private:
    point3 box_min;
    point3 box_max;
    shared_ptr<material> mat;
};

#endif
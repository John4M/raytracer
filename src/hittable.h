#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"

class material;

namespace object_ids {
enum : int {
    sky = 0,
    ground,
    matte,
    glass,
    metal,
    count
};
}

class hit_record {
    public:
        point3 p;
        vec3 normal;
        shared_ptr<material> mat;
        int object_id = 0;
        double t;
        bool front_face;

        void set_face_normal(const ray& r, const vec3& outward_normal) {
            front_face = dot(r.direction(), outward_normal) < 0;
            normal = front_face ? outward_normal : -outward_normal;
        }
};

class hittable {
    public:
        virtual ~hittable() = default;

        virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
};

#endif
#include "pathtracer.h"

#include "scene/light.h"
#include "scene/sphere.h"
#include "scene/triangle.h"


using namespace CGL::SceneObjects;

namespace CGL {

PathTracer::PathTracer() {
  gridSampler = new UniformGridSampler2D();
  hemisphereSampler = new UniformHemisphereSampler3D();

  tm_gamma = 2.2f;
  tm_level = 1.0f;
  tm_key = 0.18;
  tm_wht = 5.0f;
}

PathTracer::~PathTracer() {
  delete gridSampler;
  delete hemisphereSampler;
}

void PathTracer::set_frame_size(size_t width, size_t height) {
  sampleBuffer.resize(width, height);
  sampleCountBuffer.resize(width * height);
}

void PathTracer::clear() {
  bvh = NULL;
  scene = NULL;
  camera = NULL;
  sampleBuffer.clear();
  sampleCountBuffer.clear();
  sampleBuffer.resize(0, 0);
  sampleCountBuffer.resize(0, 0);
}

void PathTracer::write_to_framebuffer(ImageBuffer &framebuffer, size_t x0,
                                      size_t y0, size_t x1, size_t y1) {
  sampleBuffer.toColor(framebuffer, x0, y0, x1, y1);
}

Spectrum
PathTracer::estimate_direct_lighting_hemisphere(const Ray &r,
                                                const Intersection &isect) {
  // Estimate the lighting from this intersection coming directly from a light.
  // For this function, sample uniformly in a hemisphere.

  // make a coordinate system for a hit point
  // with N aligned with the Z direction.
  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o = o2w.T();

  // w_out points towards the source of the ray (e.g.,
  // toward the camera if this is a primary ray)
  const Vector3D &hit_p = r.o + r.d * isect.t;
  const Vector3D &w_out = w2o * (-r.d);

  // This is the same number of total samples as
  // estimate_direct_lighting_importance (outside of delta lights). We keep the
  // same number of samples for clarity of comparison.
  int num_samples = scene->lights.size() * ns_area_light;
  Spectrum L_out;

  // TODO (Part 3): Write your sampling loop here
  // TODO BEFORE YOU BEGIN
  // UPDATE `est_radiance_global_illumination` to return direct lighting instead of normal shading

  float pdf;
  Vector3D w_in;

  for (size_t i = 0; i < num_samples; i++) {
    auto l = isect.bsdf->sample_f(w_out, &w_in, &pdf);

    Ray ray_in(hit_p, o2w * w_in);
    ray_in.min_t = EPS_F;

    Intersection isect_in;
    if (!bvh->intersect(ray_in, &isect_in)) {
      continue;
    }

    L_out += isect_in.bsdf->get_emission() * l * w_in.z / pdf;
  }
  L_out /= num_samples;

  return L_out;
}

Spectrum
PathTracer::estimate_direct_lighting_importance(const Ray &r,
                                                const Intersection &isect) {
  // Estimate the lighting from this intersection coming directly from a light.
  // To implement importance sampling, sample only from lights, not uniformly in
  // a hemisphere.

  // make a coordinate system for a hit point
  // with N aligned with the Z direction.
  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o = o2w.T();

  // w_out points towards the source of the ray (e.g.,
  // toward the camera if this is a primary ray)
  const Vector3D &hit_p = r.o + r.d * isect.t;
  const Vector3D &w_out = w2o * (-r.d);
  Spectrum L_out;

  Vector3D light_in;
  float distToLight, pdf;

  for (auto light : scene->lights) {
    size_t num_samples = light->is_delta_light()? 1: ns_area_light;
    for (size_t i = 0; i < num_samples; i++) {
      auto spec = light->sample_L(hit_p, &light_in, &distToLight, &pdf);
      auto w_in = w2o * light_in;

      Ray ray_in(hit_p, light_in);
      ray_in.min_t = EPS_F;
      Intersection isect_in;

      if (!bvh->intersect(ray_in, &isect_in) || isect_in.t > distToLight - EPS_F) {
        L_out += spec * isect.bsdf->f(w_out, w_in) * w_in.z / pdf / num_samples;
      }
    }
  }

  return L_out;
}

Spectrum PathTracer::zero_bounce_radiance(const Ray &r,
                                          const Intersection &isect) {
  // TODO: Part 3, Task 2
  // Returns the light that results from no bounces of light

  return isect.bsdf->get_emission();
}

Spectrum PathTracer::one_bounce_radiance(const Ray &r,
                                         const Intersection &isect) {
  // TODO: Part 3, Task 3
  // Returns either the direct illumination by hemisphere or importance sampling
  // depending on `direct_hemisphere_sample`
  
  return direct_hemisphere_sample? estimate_direct_lighting_hemisphere(r, isect): estimate_direct_lighting_importance(r, isect);
}

Spectrum PathTracer::at_least_one_bounce_radiance(const Ray &r,
                                                  const Intersection &isect) {
  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o = o2w.T();

  Vector3D hit_p = r.o + r.d * isect.t;
  Vector3D w_out = w2o * (-r.d);

  Spectrum L_out = estimate_direct_lighting_importance(r, isect);

  Vector3D w_in;
  float pdf;
  float cpdf = 0.6;

  isect.bsdf->sample_f(w_out, &w_in, &pdf);
  Ray ray_in(hit_p, o2w * w_in);
  ray_in.min_t = EPS_F;
  Intersection isect_in;
  
  if (coin_flip(cpdf) && bvh->intersect(ray_in, &isect_in)) {
    L_out += at_least_one_bounce_radiance(ray_in, isect_in) * isect.bsdf->f(w_out, w_in) * w_in.z / pdf / cpdf;
  }
  
  return L_out;
}

Spectrum PathTracer::est_radiance_global_illumination(const Ray &r) {
  Intersection isect;
  Spectrum L_out;

  // You will extend this in assignment 3-2.
  // If no intersection occurs, we simply return black.
  // This changes if you implement hemispherical lighting for extra credit.

  if (!bvh->intersect(r, &isect))
    return L_out;

  // The following line of code returns a debug color depending
  // on whether ray intersection with triangles or spheres has
  // been implemented.

  // REMOVE THIS LINE when you are ready to begin Part 3.
  //L_out = (isect.t == INF_D) ? debug_shading(r.d) : normal_shading(isect.n);

  // TODO (Part 3): Return the direct illumination.
  L_out = zero_bounce_radiance(r, isect);
  //L_out += one_bounce_radiance(r, isect);

  // TODO (Part 4): Accumulate the "direct" and "indirect"
  // parts of global illumination into L_out rather than just direct
  L_out += at_least_one_bounce_radiance(r, isect);

  return L_out;
}

void PathTracer::raytrace_pixel(size_t x, size_t y) {

  // TODO (Part 1.1):
  // Make a loop that generates num_samples camera rays and traces them
  // through the scene. Return the average Spectrum.
  // You should call est_radiance_global_illumination in this function.
  Spectrum spectrum;
  /*
  for (size_t i = 0; i < ns_aa; i++) {
    auto sample = gridSampler->get_sample();
    auto ray = camera->generate_ray((x + sample.x) / sampleBuffer.w, (y + sample.y) / sampleBuffer.h);
    spectrum += est_radiance_global_illumination(ray) / ns_aa;
  }
  */
  // TODO (Part 5):
  // Modify your implementation to include adaptive sampling.
  // Use the command line parameters "samplesPerBatch" and "maxTolerance"

  double s1 = 0;
  double s2 = 0;
  int num_samples = 1;

  for (; num_samples <= ns_aa; num_samples++)  {
    auto sample = gridSampler->get_sample();
    auto ray = camera->generate_ray((x + sample.x) / sampleBuffer.w, (y + sample.y) / sampleBuffer.h);
    auto s = est_radiance_global_illumination(ray);
    double illum = s.illum();
    s1 += illum;
    s2 += illum * illum;

    spectrum = (1.0 / num_samples) * s + ((num_samples - 1.0) / num_samples) * spectrum;

    if (num_samples > ns_aa / 2 && num_samples > 1) {
      auto mean = s1 / num_samples;
      auto var = (s2 - (s1 * s1 / num_samples)) / (num_samples - 1);

      if (maxTolerance * mean >= confidence * sqrt(var / num_samples)) {
        break;
      }
    }
  }

  //int num_samples = ns_aa;          // total samples to evaluate
  Vector2D origin = Vector2D(x, y); // bottom left corner of the pixel

  sampleBuffer.update_pixel(spectrum, x, y);
  sampleCountBuffer[x + y * sampleBuffer.w] = num_samples;
}

} // namespace CGL
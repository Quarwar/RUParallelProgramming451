/*

Parallel Programming HW4
Author: Joseph Signorile
Cite: ChatGPT for syntax and debugging help (the code itself was mostly written by me)
GitHub: https://github.com/Quarwar/RUParallelProgramming451/tree/main/OpenMPGravSim
Description: Gravity Simulator - runs the position calculations for N bodies in a system for 
a certain number of timesteps. Tests different optimization methods using OpenMP parallelism.

Compile with: g++ -O2 -fopenmp -mavx2 SolarSystem.cpp -o SolarSystem
Run with (testing - N = 1000): ./SolarSystem 5000 .1 1000 0

Runtimes:
Single-threaded: SIMD: 113.59 seconds; No SIMD: 113.29 seconds
2 threads: SIMD: 67.13 seconds; No SIMD: 67.10 seconds
4 threads: SIMD: 46.86 seconds; No SIMD: 47.01 seconds

Note: SIMD does optimize the inner loop (looking at the resulting assembly), but it is negligible due to
expensive division and sqrt operations.

*/

#include <iostream>
#include <cmath>
#include <random>
#include <vector>
#include <omp.h>
#include <chrono>

// Global Testing Parameters:
int numThreads = 4;

/*
    Gravity Simulator
    Given: n=10^6 bodies
    for each body : 10 variables * 8 bytes/double * n = 80MB
        mass
        x,y,z
        vx,vy,vz
        ax,ay,az

    F(a,b) = G m_1 M_2 / (dist(a,b)^2)

    O(n^2)
    a(body) = F(body,bi), for all bi
*/

class Vec3d {
public:
    double x,y,z;

    // Vec3d a(1,2,3);
    // Vec3d b(4,5);
    // Vec3d c(6);
    // Vec3d d; // d = (0,0,0)
    Vec3d(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    // without friend:  a.dist(b,c) WRONG
    friend double distsq(const Vec3d& a, const Vec3d& b) {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double dz = b.z - a.z;
        return dx*dx+dy*dy+dz*dz;
    }
    friend Vec3d operator+(const Vec3d& a, const Vec3d& b) {
        return Vec3d(a.x + b.x, a.y + b.y, a.z + b.z);
    }
    friend Vec3d operator-(const Vec3d& a, const Vec3d& b) {
        return Vec3d(a.x - b.x, a.y - b.y, a.z - b.z);
    }
    friend Vec3d operator*(const Vec3d& a, double s) {
        return Vec3d(a.x * s, a.y * s, a.z * s);
    }

    // remember in C++ ONLY WRITE IT ONCE... let the compiler inline things
    friend Vec3d operator*(double s, const Vec3d& a) {
        return a * s;
    }
    friend Vec3d operator/(const Vec3d& a, double s) {
        return Vec3d(a.x / s, a.y / s, a.z / s);
    }
    Vec3d& operator+=(const Vec3d& b) {
        x += b.x; y += b.y; z += b.z;
        return *this;
    }
    friend double dist(const Vec3d& a, const Vec3d& b) {
        return sqrt(distsq(a,b));
    }
    //   2 << 3     00010  left shift by 3  00010000
    friend std::ostream& operator <<(std::ostream& s, const Vec3d& a) {
        return s << a.x << ',' << a.y << ',' << a.z;
    }
};

/*
    Represent a gravitational body in a solar system
    each body has
        mass
        position
        velocity

*/

class Body { 
public:
    static constexpr double G = 6.674e-11; // universal gravitational constant
    double mass;
    Vec3d  pos;
    Vec3d  v;
    Vec3d  a;
    Body(double m, const Vec3d& pos, const Vec3d& v)
      : mass(m), pos(pos), v(v), a(0,0,0) {}
};


class System {
private:
    std::vector<Body> bodies;
    void add_sun(double mass);
    void add_circular(double mass, double r, double angle);
    void add_population(std::mt19937& rng, int count,
                        double mlo, double mhi, double rmin, double rmax);
public:
    //possible:    System(const char filename[]);
    System(int n); // sun, 5-20 planets, remaining n are asteroids
    void stepForward(double dt);
    void print() const;
};

// v = sqrt(GM/r) keeps a circular orbit around a fixed central mass
static double circular_speed(double central_mass, double r) {
    return std::sqrt(Body::G * central_mass / r);
}

static double random_unit(std::mt19937& rng) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
}

// log-uniform so masses span orders of magnitude, not just the top of the range
static double random_log(std::mt19937& rng, double lo, double hi) {
    return lo * std::pow(hi / lo, random_unit(rng));
}

void System::add_sun(double mass) {
    bodies.emplace_back(mass, Vec3d(0, 0, 0), Vec3d(0, 0, 0));
}

// position on the circle, velocity tangent, same sense for every body (ccw)
void System::add_circular(double mass, double r, double angle) {
    double v = circular_speed(bodies[0].mass, r);
    bodies.emplace_back(
        mass,
        Vec3d(r * std::cos(angle), r * std::sin(angle), 0),
        Vec3d(-v * std::sin(angle), v * std::cos(angle), 0));
}

void System::add_population(std::mt19937& rng, int count,
                            double mlo, double mhi, double rmin, double rmax) {
    const double two_pi = 2.0 * std::acos(-1.0);
    for (int i = 0; i < count; i++) {
        double mass = random_log(rng, mlo, mhi);
        double r = rmin + random_unit(rng) * (rmax - rmin);
        double angle = random_unit(rng) * two_pi;
        add_circular(mass, r, angle);
    }
}

// sun, then 5-20 planets, then the rest of n as asteroids
System::System(int n) {
    bodies.reserve(n + 1);
    const double AU = 1.496e11;
    add_sun(1.9885e30);
    std::mt19937 rng(0); // fixed seed so a run is repeatable
    int nplanets = std::uniform_int_distribution<int>(5, 20)(rng);
    if (nplanets > n)
        nplanets = n;
    // Mercury..Jupiter scale, spread from inside Mercury out past Neptune
    add_population(rng, nplanets, 3.3e23, 1.9e27, 0.4 * AU, 30.0 * AU);
    // small bodies packed into an asteroid belt
    add_population(rng, n - nplanets, 1.0e15, 1.0e20, 2.1 * AU, 3.3 * AU);
}

void System::stepForward(double dt) {
    #pragma omp parallel for num_threads(numThreads)
    for (uint32_t i = 0; i < bodies.size(); i++) {
        const Body& b1 = bodies[i];
        double x1 = b1.pos.x, y1 = b1.pos.y, z1 = b1.pos.z;
        double ax = 0, ay = 0, az = 0;

        #pragma omp simd reduction(+:ax, ay, az)
        for (uint32_t j = 0; j < bodies.size(); j++) {
            if (i == j) continue;
            const Body& b2 = bodies[j];
            double x2 = b2.pos.x, y2 = b2.pos.y, z2 = b2.pos.z;

            // compute acceleration component from bodies[j]
            double dS = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2);
            double d = sqrt(dS);
            double F = Body::G * b2.mass / (dS);
            double a = F / d;
            
            ax += a * (x2 - x1);
            ay += a * (y2 - y1);
            az += a * (z2 - z1);
        }
        bodies[i].a = Vec3d(ax, ay, az);
    }
    /*
      This is correct. Why do we have to do it after?
      this is not a good algorithm. The error is O(dt)
      and the error is cumulative.

      In this homework, you do not have to implement better
      algorithms (but you may). The first simplest algorithm
      is RKF45. This is very complex, and very clever.
      Runge-Kutta-Fehlberg is a family of algorithms.
      RKF45 computes a 4th order and a 5th order solution
      that happen to use some of the same calculations to save time.
      This allows estimating the error by comparing 4th and 5th order
      solutions.      
    */
    #pragma omp parallel for num_threads(numThreads)
    for (uint32_t i = 0; i < bodies.size(); i++) {  
        bodies[i].v += bodies[i].a * dt;
        bodies[i].pos += bodies[i].v * dt;
    }
}

void System::print() const {
    for (uint32_t i = 0; i < bodies.size(); i++)
        std::cout << i << '\t' << bodies[i].mass << '\t'
                  << bodies[i].pos << '\t' << bodies[i].v << '\n';
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <num_bodies> [dt] [num_steps] [num_output]" << std::endl;
        return 1;
    }
    const int num_bodies = std::stoi(argv[1]);
    const double dt = argc > 2 ? std::stod(argv[2]) : 10.0; // 10 second timestep
    const int num_steps = argc > 3 ? std::stoi(argv[3]) : 100;
    const int num_output = argc > 4 ? std::stoi(argv[4]) : 10;
    System system(num_bodies);

    // Benchmark the time it takes to complete simulation, skipping initialization:
    auto start = std::chrono::high_resolution_clock::now();

    for (int time = 0; time < num_steps; time++) {
        system.stepForward(dt);
        if (num_output != 0 && time % num_output == 0) {
            system.print(); // disable printing for benchmarking
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "Simulation completed in " << elapsed << " seconds\n";

    return 0;
}
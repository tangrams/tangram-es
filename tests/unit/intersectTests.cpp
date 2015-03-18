#define CATCH_CONFIG_MAIN 
#include "catch.hpp"

#include "intersect.h"

TEST_CASE( "Test intersection between trapezoids and AABBs", "[Core][AABBIntersectsTrapezoid]" ) {

    glm::vec2 A, B, C, D; //Trapezoid vertices

    glm::vec4 aabb(0.f, 0.f, 1.f, 1.f);

    // Intersections
    // =============

    // Trapezoid entirely contains bbox

    A = {-3.f,  1.f};
    B = { 1.f,  5.f};
    C = { 1.f, -1.f};
    D = { 3.f,  1.f};

    REQUIRE(AABBIntersectsTrapezoid(aabb, A, B, C, D));

    // Trapezoid entirely within bbox

    A = { 0.1f, 0.2f};
    B = { 0.9f, 0.2f};
    C = { 0.4f, 0.7f};
    D = { 0.6f, 0.7f};

    REQUIRE(AABBIntersectsTrapezoid(aabb, A, B, C, D));

    // Trapezoid partially within bbox

    A = { 0.f,  0.f};
    B = { 1.f,  1.f};
    C = { 0.f, -1.f};
    D = { 2.f,  1.f};

    REQUIRE(AABBIntersectsTrapezoid(aabb, A, B, C, D));

    // Non-intersections
    // =================

    // Trapezoid left of bbox

    A = {-5.f,  0.f};
    B = {-1.f,  0.f};
    C = {-4.f,  1.f};
    D = {-2.f,  1.f};

    REQUIRE(!AABBIntersectsTrapezoid(aabb, A, B, C, D));

    // Trapezoid above bbox

    A = { 0.f,  2.f};
    B = { 4.f,  2.f};
    C = { 1.f,  3.f};
    D = { 3.f,  3.f};

    REQUIRE(!AABBIntersectsTrapezoid(aabb, A, B, C, D));

    // Trapezoid oblique to bbox

    A = { 3.f,  0.f};
    B = { 0.f,  3.f};
    C = { 4.f,  0.f};
    D = { 0.f,  4.f};

    REQUIRE(!AABBIntersectsTrapezoid(aabb, A, B, C, D));

}

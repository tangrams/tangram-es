#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include <iostream>
#include "typedMesh.h"

struct Vertex {
    float a;
    float b;
    short c;
    char d;
};

std::shared_ptr<VertexLayout> layout = std::shared_ptr<VertexLayout>(new VertexLayout({
    {"ab", 2, GL_FLOAT, false, 0},
    {"c",  1, GL_SHORT, false, 0},
    {"d",  1, GL_BYTE,  false, 0},
}));

std::shared_ptr<TypedMesh<Vertex>> newMesh(unsigned int size) {
    auto mesh = std::shared_ptr<TypedMesh<Vertex>>(new TypedMesh<Vertex>(layout, GL_TRIANGLES));
    std::vector<Vertex> vertices;
    for (int i = 0; i < size; ++i) {
        vertices.push_back({0,0,0,0});
    }
    mesh->addVertices(std::move(vertices), {});
    mesh->compileVertexBuffer();
    return mesh;
}

void checkBounds(const std::shared_ptr<TypedMesh<Vertex>>& mesh) {

    REQUIRE(mesh->getDirtyOffset() >= 0);
    REQUIRE(mesh->getDirtyOffset() < mesh->numVertices() * sizeof(Vertex));
    REQUIRE(long(mesh->getDirtyOffset() + mesh->getDirtySize()) < mesh->numVertices() * sizeof(Vertex));

}

TEST_CASE( "Simple update on vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 0);

    mesh->updateVertices(0, 4, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 4 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Left merge on vertices with bigger left size", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    mesh->updateVertices(2 * sizeof(Vertex), 2, Vertex());
    mesh->updateVertices(0 * sizeof(Vertex), 8, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 8 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Left merge on vertices with smaller left size", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    mesh->updateVertices(2 * sizeof(Vertex), 2, Vertex());
    mesh->updateVertices(0 * sizeof(Vertex), 1, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 4 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Right merge on vertices dirtiness", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    mesh->updateVertices(2 * sizeof(Vertex), 2, Vertex());
    mesh->updateVertices(4 * sizeof(Vertex), 2, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 2 * sizeof(Vertex));
    REQUIRE(mesh->getDirtySize() == 4 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Update on second attribute of the mesh for n vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    int nVert = 5;
    size_t stride_b = sizeof(float); // stride of a in the struct

    mesh->updateAttribute(stride_b + sizeof(Vertex), nVert, 0.f);

    REQUIRE(mesh->getDirtyOffset() == stride_b + sizeof(Vertex));
    REQUIRE(mesh->getDirtySize() == (nVert - 1) * sizeof(Vertex) + sizeof(float));

    checkBounds(mesh);
}

TEST_CASE( "Update on second attribute of the mesh for 1 vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of a in the struct

    mesh->updateAttribute(stride_b, 1, 0.f);

    REQUIRE(mesh->getDirtyOffset() == stride_b);
    REQUIRE(mesh->getDirtySize() == sizeof(float));

    checkBounds(mesh);
}

TEST_CASE( "Update on second and third attribute of the mesh for n vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of b in the struct
    size_t stride_c = 2 * sizeof(float); // stride of c in the struct
    short c = 0;

    mesh->updateAttribute(stride_b, 5, 0.f);
    mesh->updateAttribute(stride_c + sizeof(Vertex), 8, c);

    REQUIRE(mesh->getDirtyOffset() == stride_b);
    int dist = stride_c - stride_b; // distance between b and c
    REQUIRE(mesh->getDirtySize() == 8 * sizeof(Vertex) + dist + sizeof(short));

    checkBounds(mesh);
}

TEST_CASE( "Update on second and fourth attribute of the mesh for n vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of b in the struct
    size_t stride_d = 2 * sizeof(float) + sizeof(short); // stride of c in the struct
    char d = 0;

    mesh->updateAttribute(stride_b, 1, 0.f);
    mesh->updateAttribute(stride_d + sizeof(Vertex), 7, d);

    REQUIRE(mesh->getDirtyOffset() == stride_b);
    int dist = stride_d - stride_b; // distance between b and c
    REQUIRE(mesh->getDirtySize() == 7 * sizeof(Vertex) + dist + sizeof(char));

    checkBounds(mesh);
}

TEST_CASE( "Check overflow", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of b in the struct

    mesh->updateAttribute(stride_b, 100, 0.f);
    mesh->updateVertices(0, 100, Vertex());
    mesh->updateAttribute(stride_b + sizeof(Vertex) * 10, 1, Vertex());
    mesh->updateAttribute(-sizeof(Vertex) * 100, 10, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 0);

    checkBounds(mesh);
}

#include "catch.hpp"

#include <iostream>
#include "gl/mesh.h"

using namespace Tangram;

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

struct TestMesh : public Mesh<Vertex> {
    using Base = Mesh<Vertex>;
    using Base::Base;

    GLsizei getDirtySize() const { return m_dirtySize; }
    GLintptr getDirtyOffset() const { return m_dirtyOffset; }

    int numVertices() const { return m_nVertices; }
    int numIndices() const { return m_nIndices; }
};

std::shared_ptr<TestMesh> newMesh(unsigned int size) {
    auto mesh = std::make_shared<TestMesh>(layout, GL_TRIANGLES);
    MeshData<Vertex> meshData;

    for (size_t i = 0; i < size; ++i) {
        meshData.vertices.push_back({0,0,0,0});
        meshData.offsets.emplace_back(0, 4);
    }
    mesh->compile(meshData);
    return mesh;
}

void checkBounds(const std::shared_ptr<TestMesh>& mesh) {

    REQUIRE(mesh->getDirtyOffset() >= 0);
    REQUIRE(mesh->getDirtyOffset() < mesh->numVertices() * sizeof(Vertex));
    REQUIRE(long(mesh->getDirtyOffset() + mesh->getDirtySize()) < mesh->numVertices() * sizeof(Vertex));

}

TEST_CASE( "Simple update on vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 0);

    mesh->updateVertices({0, 4}, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 4 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Left merge on vertices with bigger left size", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    mesh->updateVertices({2, 2}, Vertex());
    mesh->updateVertices({0, 8}, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 8 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Left merge on vertices with smaller left size", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    mesh->updateVertices({2, 2}, Vertex());
    mesh->updateVertices({0, 1}, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 4 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Right merge on vertices dirtiness", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);

    mesh->updateVertices({2, 2}, Vertex());
    mesh->updateVertices({4, 2}, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 2 * sizeof(Vertex));
    REQUIRE(mesh->getDirtySize() == 4 * sizeof(Vertex));

    checkBounds(mesh);
}

TEST_CASE( "Update on second attribute of the mesh for n vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    int nVert = 5;
    size_t stride_b = sizeof(float); // stride of a in the struct

    mesh->updateAttribute({1, nVert}, 0.f, stride_b);

    REQUIRE(mesh->getDirtyOffset() == stride_b + sizeof(Vertex));
    REQUIRE(mesh->getDirtySize() == (nVert - 1) * sizeof(Vertex) + sizeof(float));

    checkBounds(mesh);
}

TEST_CASE( "Update on second attribute of the mesh for 1 vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of a in the struct

    mesh->updateAttribute({0, 1}, 0.f, stride_b);

    REQUIRE(mesh->getDirtyOffset() == stride_b);
    REQUIRE(mesh->getDirtySize() == sizeof(float));

    checkBounds(mesh);
}

TEST_CASE( "Update on second and third attribute of the mesh for n vertices", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of b in the struct
    size_t stride_c = 2 * sizeof(float); // stride of c in the struct
    short c = 0;

    mesh->updateAttribute({0, 5}, 0.f, stride_b);
    mesh->updateAttribute({1, 8}, c, stride_c);

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

    mesh->updateAttribute({0, 1}, 0.f, stride_b);
    mesh->updateAttribute({1, 7}, d, stride_d);

    REQUIRE(mesh->getDirtyOffset() == stride_b);
    int dist = stride_d - stride_b; // distance between b and c
    REQUIRE(mesh->getDirtySize() == 7 * sizeof(Vertex) + dist + sizeof(char));

    checkBounds(mesh);
}

TEST_CASE( "Check overflow", "[Core][TypedMesh]" ) {
    auto mesh = newMesh(10);
    size_t stride_b = sizeof(float); // stride of b in the struct

    mesh->updateAttribute({0, 100}, 0.f, stride_b);
    mesh->updateVertices({0, 100}, Vertex());
    mesh->updateAttribute({10, 1}, Vertex(), stride_b);
    mesh->updateAttribute({-100, 10}, Vertex());

    REQUIRE(mesh->getDirtyOffset() == 0);
    REQUIRE(mesh->getDirtySize() == 0);

    checkBounds(mesh);
}

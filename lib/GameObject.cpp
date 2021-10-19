#include <iostream>
#include "../UEngine.hpp"

GameObject::GameObject() {
    rot = glm::mat4(1.0f);
};
GameObject::GameObject(UniverseEngine * e) : GameObject() { 
    this->e = e; 
};
GameObject::GameObject(UniverseEngine * e, std::vector<Vertex> vertices, std::vector<uint32_t> indicies, glm::vec3 pos) {
    this->e = e;
    this->vertecies = vertices;
    this->indicies = indicies;
    this->pos = glm::vec4(pos, 1.0f);
}
void GameObject::tick(void) {
    std::cout << "acc: " << printVec3(acc) << "\n";
    vec = vec + acc;
    std::cout << "vec: " << printVec3(vec) << "\n";
    updatePos(this->pos + vec);
}
void GameObject::tick(glm::vec3 a) {
    acc = acc + a;
    vec = vec + acc;
    updatePos(this->pos + vec);
}
void GameObject::updateVerticeId() {
    for (size_t i = 0; i < vertecies.size(); i++) {
        vertecies.at(i).gameObjId = id;
    }
}
void GameObject::updatePos(glm::vec3 pos) {
    if (this->pos == pos) return;
    this->pos = pos;
    meshChanged = true;
    e->recreateModel();
}
void GameObject::updateVec(glm::vec3 vec) {
    this->vec = vec;
}
void GameObject::updateAcc(glm::vec3 acc) {
    this->acc = acc;
}

void GameObject::updateRot(glm::mat4 rot) {
    if (rot == this->rot) return;
    this->rot = rot;
    this->meshChanged = true;
    this->e->recreateModel();
}

glm::vec3 GameObject::getPos() {
    return pos;
}
glm::vec3 GameObject::getVec() {
    return vec;
}
glm::vec3 GameObject::getAcc() {
    return acc;
}

Mesh GameObject::getMesh() {
    Mesh m {};
    size_t v = vertecies.size();
    std::vector<Vertex> a(v);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = model * rot;
    for (auto i = 0; i < v; i++) {
        Vertex v = vertecies[i];
        v.pos = glm::vec3(model * glm::vec4(v.pos, 1.0f));
        a[i] = v;
    }
    m.i = indicies;
    m.v = a;
    return m;
}

bool GameObject::hasMeshChanged() {
    return meshChanged;
}

void GameObject::setId(uint32_t id) {
    this->id = id;
    updateVerticeId();
}
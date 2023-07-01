#include "sgl.hpp"
#include "Bezier.h"
#include <algorithm>
#include <random>

using namespace sgl;

// Constants
const float PI = 3.14159265359;

// Forwarding declarations
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode, void* cookie);
auto generateCirclePointsSet() -> std::vector<glm::vec3>;
auto generateUnisinosPointsSet() -> std::vector<glm::vec3>;

// Global variables
glm::vec3 rotate_vector = {0.f, 1.f, 0.f};

// Main function
int main(int argc, char *argv[])
{
    // Context
    Window window = init_window(800, 800, "Visualizador 3D");
    set_key_callback(key_callback, nullptr);
    set_camera_control(true);
    std::vector<Object*> objects;  // list of objects

    // Objects
    ModelRef model = load_model("../../3D_Models/Suzanne/SuzanneTriTextured.obj");
    Object suzanne = create_mesh(model->meshes[0]);
    suzanne.scale(0.5f);
    objects.push_back(&suzanne);

    model = load_model("../../3D_Models/Suzanne/CuboTextured.obj");
    Object bola = create_mesh(model->meshes[0]);
    bola.scale(0.4f);
    bola.position({ -1.4f, 0.f, 0.f });
    objects.push_back(&bola);

    model = load_model("../../3D_Models/Planetas/planeta.obj");
    Object planeta = create_mesh(model->meshes[0]);
    planeta.scale(0.4f);
    glm::vec3 planeta_position = { +1.8f, 0.3f, 1.8f };
    planeta.position(planeta_position);
    objects.push_back(&planeta);

    Object plane = create_quad().color(GRAY);
    plane.scale(3.f);
    plane.rotate({PI/2, 0.f, 0.f});
    plane.position({0.f, -1.0f, 0.f});
    objects.push_back(&plane);

    Object cube = create_cuboid(Size3(1.f)).color(WHITE);
    cube.position({-0.5f, -0.5f, -0.5f});
    objects.push_back(&cube);

    // Curves
    std::vector<glm::vec3> circlePoints = generateCirclePointsSet();
    Bezier circleBezier;
    circleBezier.setControlPoints(circlePoints);
    circleBezier.generateCurve(10);
    int circleNbCurvePoints = circleBezier.getNbCurvePoints();
    int i = 0;

    std::vector<glm::vec3> planetaPoints = generateUnisinosPointsSet();
    Bezier planetaBezier;
    planetaBezier.setControlPoints(planetaPoints);
    planetaBezier.generateCurve(10);
    int planetaNbCurvePoints = planetaBezier.getNbCurvePoints();
    int j = 0;

    // loop
    while (!window_should_close()) {

        // update
        poll_events();
        float angle = get_time();
        suzanne.rotate(rotate_vector * angle);
        bola.rotate(glm::vec3{0.f, angle, 0.f});
        planeta.rotate(glm::vec3(angle));
        bola.position(circleBezier.getPointOnCurve(i));
        planeta.position(planetaBezier.getPointOnCurve(j) + planeta_position);
        i = (i + 1) % circleNbCurvePoints;
        j = (j + 1) % planetaNbCurvePoints;

        // render
        begin_render(DARK_GRAY);
        for (auto obj : objects)
            draw_object(*obj);
        end_render();
    }

    return 0;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode, void* cookie)
{
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        set_camera_control(true);
    }
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        rotate_vector.x = rotate_vector.x ? 0.f : 1.f;
    }
    else if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
        rotate_vector.y = rotate_vector.y ? 0.f : 1.f;
    }
    else if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        rotate_vector.z = rotate_vector.z ? 0.f : 1.f;
    }
}


std::vector<glm::vec3> generateCirclePointsSet()
{
    std::vector<glm::vec3> vertices;
    for (float i = 0.f; i < 2*PI; i += 0.05f) {
        float x = sin(i) * 1.5f;
        float z = cos(i) * 1.5f;
        float y = 0.f;
        vertices.push_back(glm::vec3(x, y, z));
    }
    return vertices;
}


std::vector<glm::vec3> generateUnisinosPointsSet()
{
    float vertices[] = {
        -0.262530, 0.376992, 0.000000,
-0.262530, 0.377406, 0.000000,
-0.262530, 0.334639, 0.000000,
-0.262530, 0.223162, 0.000000,
-0.262530, 0.091495, 0.000000,
-0.262371, -0.006710, 0.000000,
-0.261258, -0.071544, -0.000000,
-0.258238, -0.115777, -0.000000,
-0.252355, -0.149133, -0.000000,
-0.242529, -0.179247, -0.000000,
-0.227170, -0.208406, -0.000000,
-0.205134, -0.237216, -0.000000,
-0.177564, -0.264881, -0.000000,
-0.146433, -0.289891, -0.000000,
-0.114730, -0.309272, -0.000000,
-0.084934, -0.320990, -0.000000,
-0.056475, -0.328224, -0.000000,
-0.028237, -0.334170, -0.000000,
0.000000, -0.336873, -0.000000,
0.028237, -0.334170, -0.000000,
0.056475, -0.328224, -0.000000,
0.084934, -0.320990, -0.000000,
0.114730, -0.309272, -0.000000,
0.146433, -0.289891, -0.000000,
0.177564, -0.264881, -0.000000,
0.205134, -0.237216, -0.000000,
0.227170, -0.208406, -0.000000,
0.242529, -0.179247, -0.000000,
0.252355, -0.149133, -0.000000,
0.258238, -0.115777, -0.000000,
0.261258, -0.071544, -0.000000,
0.262371, -0.009704, 0.000000,
0.262530, 0.067542, 0.000000,
0.262769, 0.153238, 0.000000,
0.264438, 0.230348, 0.000000,
0.268678, 0.284286, 0.000000,
0.275462, 0.320338, 0.000000,
0.284631, 0.347804, 0.000000,
0.296661, 0.372170, 0.000000,
0.311832, 0.396628, 0.000000,
0.328990, 0.419020, 0.000000,
0.347274, 0.436734, 0.000000,
0.368420, 0.450713, 0.000000,
0.393395, 0.462743, 0.000000,
0.417496, 0.474456, 0.000000,
0.436138, 0.487056, 0.000000,
0.450885, 0.500213, 0.000000,
0.464572, 0.513277, 0.000000,
0.478974, 0.525864, 0.000000,
0.494860, 0.538133, 0.000000,
0.510031, 0.552151, 0.000000,
0.522127, 0.570143, 0.000000,
0.531124, 0.593065, 0.000000,
0.537629, 0.620809, 0.000000,
0.542465, 0.650303, 0.000000,
0.546798, 0.678259, 0.000000,
0.552959, 0.703513, 0.000000,
0.563121, 0.725745, 0.000000,
0.577656, 0.745911, 0.000000,
0.596563, 0.764858, 0.000000,
0.620160, 0.781738, 0.000000,
0.648302, 0.795385, 0.000000,
0.678670, 0.805057, 0.000000,
0.710336, 0.810741, 0.000000,
0.750111, 0.814914, 0.000000,
0.802994, 0.819945, 0.000000,
0.860771, 0.825435, 0.000000,
    };

    vector <glm::vec3> uniPoints;

    for (int i = 0; i < 67 * 3; i += 3)
    {
        glm::vec3 point;
        point.x = vertices[i];
        point.y = vertices[i + 1];
        point.z = 0.0;

        uniPoints.push_back(point);
    }

    return uniPoints;
}

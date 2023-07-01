#include "sgl.hpp"

using namespace sgl;

const float PI = 3.14159265359;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode, void* cookie);

glm::vec3 rotate_vector = {0.f, 1.f, 0.f};

int main(int argc, char *argv[])
{
    Window window = init_window(800, 800, "Visualizador 3D");
    ModelRef model = load_model("../../3D_Models/Suzanne/SuzanneTriTextured.obj");
    Object suzanne = create_mesh(model->meshes[0]);
    suzanne.scale(0.5f);

    Object plane = create_quad().color(GRAY);
    plane.scale(2.f);
    plane.rotate({PI/2, 0.f, 0.f});
    plane.position({0.f, -1.0f, 0.f});

    set_camera_control(true);
    set_key_callback(key_callback, nullptr);
    Color colors[] = { WHITE, WHITE, WHITE, WHITE, WHITE, WHITE };
    Object cube = create_color_cuboid(Size3(1.f), colors);

    while (!window_should_close()) {
        poll_events();
        float angle = get_time();
        suzanne.rotate(rotate_vector * angle);
        begin_render(DARK_GRAY);
        draw_object(suzanne);
        //draw_object(plane);
        draw_ambient_light_point();
        draw_object(cube);
        end_render();
    }

    return 0;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode, void* cookie)
{
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

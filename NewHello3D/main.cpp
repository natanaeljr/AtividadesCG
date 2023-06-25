#include "sgl.hpp"

using namespace sgl;

int main(int argc, char *argv[])
{
    Window window = init_window(800, 800, "Visualizador 3D");
    ModelRef model = load_model("../../3D_Models/Suzanne/SuzanneTriTextured.obj");
    Object suzanne = create_mesh(model->meshes[0]);
    suzanne.scale(0.5f);

    Color colors[6] = { BLUE, BLUE, GREEN, GREEN, RED, RED };
    Object cube = create_color_cuboid(1.f, colors).scale(0.15f);

    while (!window_should_close()) {
        poll_events();
        float angle = get_time();
        suzanne.rotate({0.f, angle, 0.f});
        cube.rotate(glm::vec3{angle});
        cube.position({glm::sin(angle)/1.5f, glm::cos(angle)/1.5f, 0.5f});
        begin_render(DARK_GRAY);
        draw_object(suzanne);
        draw_object(cube);
        end_render();
    }

    return 0;
}

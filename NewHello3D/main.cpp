#include "sgl.hpp"

using namespace sgl;

int main(int argc, char *argv[])
{
    Context ctx = init_window(800, 600, "Visualizador 3D");
    ModelRef model = load_model("../../3D_Models/Suzanne/SuzanneTriTextured.obj");
    Object suzanne = create_mesh(model->meshes[0]);
    suzanne.scale(0.5f);

    while (!window_should_close()) {
        poll_events();
        float angle = get_time();
        suzanne.rotate(glm::vec3(0.f, angle, 0.f));
        begin_render(DARK_GRAY);
        draw_object(suzanne);
        end_render();
    }

    return 0;
}

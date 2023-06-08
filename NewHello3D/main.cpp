#include "core.hpp"

int main(int argc, char *argv[])
{
    sgl::init_window(800, 600, "Visualizador 3D");
    {
        std::array cube_colors = { sgl::BLUE, sgl::BLUE, sgl::GREEN, sgl::GREEN, sgl::RED, sgl::RED };
        sgl::Object cube = sgl::create_cuboid(sgl::Size3(1.f, 0.5f, 0.2f)).color(sgl::RED);
        sgl::Object cube2 = sgl::create_color_cuboid(sgl::Size3(1.f), cube_colors.data());

        cube.position({-0.5f, 0.f, 0.f});
        cube.scale(0.3f);

        cube2.position({+0.5f, 0.f, 0.f});
        cube2.scale(0.3f);
        cube2.rotate(glm::vec3(M_PI/3));

        while (!sgl::window_should_close()) {
            sgl::poll_events();

            float angle = sgl::get_time();
            cube.rotate(glm::vec3(angle));
            cube2.rotate(glm::vec3(angle));

            sgl::begin_render(sgl::BLACK);
            sgl::draw_object(cube);
            sgl::draw_object(cube2);
            sgl::end_render();
        }
    }
    sgl::close_window();
    return 0;
}

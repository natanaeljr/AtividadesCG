#include "core.hpp"

int main(int argc, char *argv[])
{
    sgl::Context ctx = sgl::init_window(800, 600, "Visualizador 3D");

    std::array cube_colors = { sgl::BLUE, sgl::BLUE, sgl::GREEN, sgl::GREEN, sgl::RED, sgl::RED };
    sgl::Object cube = sgl::create_cuboid(sgl::Size3(1.f, 0.5f, 0.2f)).color(sgl::RED);
    sgl::Object cube2 = sgl::create_color_cuboid(sgl::Size3(1.f), cube_colors.data());

    sgl::Object rect = sgl::create_rect(sgl::Size2(1.f)).color(sgl::RED);
    sgl::Object rect2 = sgl::create_color_rect(sgl::Size2(1.f), sgl::BLUE);
    sgl::GLTextureRef mario = sgl::load_texture("../../3D_Models/Suzanne/Cube.png", GL_NEAREST);
    sgl::Object rect3 = sgl::create_texture_rect(sgl::Size2(1.f), mario);

    //auto model = sgl::load_model("../../3D_Models/Suzanne/CuboTextured.obj");
    auto model = sgl::load_model("../../3D_Models/Suzanne/SuzanneTriTextured.obj");
    sgl::Object model_cube = sgl::create_mesh(model->meshes[0]);

    cube.position({-0.5f, +0.5f, 0.f});
    cube.scale(0.2f);
    cube2.position({+0.5f, +0.5f, 0.f});
    cube2.scale(0.2f);

    rect.position({-0.5f, -0.5f, 0.f});
    rect.scale(0.2f);
    rect2.position({+0.5f, -0.5f, 0.f});
    rect2.scale(0.2f);

    rect3.position({+0.0f, -0.0f, 0.f});
    rect3.scale(0.2f);

    model_cube.scale(0.5f);

    while (!sgl::window_should_close()) {
        sgl::poll_events();

        float angle = sgl::get_time();
        rect.rotate(glm::vec3(0.f, 0.f, angle));
        rect2.rotate(glm::vec3(0.f, 0.f, angle));
        rect3.rotate(glm::vec3(angle));
        cube.rotate(glm::vec3(angle));
        cube2.rotate(glm::vec3(angle));
        model_cube.rotate(glm::vec3(0.f, angle, 0.f));

        sgl::begin_render(sgl::DARK_GRAY);
        sgl::draw_object(cube);
        sgl::draw_object(cube2);
        sgl::draw_object(rect);
        sgl::draw_object(rect2);
        sgl::draw_object(rect3);
        sgl::draw_object(model_cube);
        sgl::end_render();
    }

    return 0;
}

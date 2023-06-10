#include "sgl.hpp"

#include <sstream>
#include <fstream>
#include <filesystem>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/// Simple Graphics Library
namespace sgl {


///////////////////////////////////////////////////////////////////////////////////////////////////
// UTILS
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Read file contents to a string
auto read_file_to_string(const std::string& filename) -> std::optional<std::string>
{
    std::string string;
    std::fstream fstream(filename, std::ios::in | std::ios::binary);
    if (!fstream) { ERROR("{} ({})", std::strerror(errno), filename); return std::nullopt; }
    fstream.seekg(0, std::ios::end);
    string.reserve(fstream.tellg());
    fstream.seekg(0, std::ios::beg);
    string.assign((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
    return string;
}

/// Convert primitive type to GL constant
template<typename T> 
struct GLType;
template<> 
struct GLType<float> { static constexpr auto value = GL_FLOAT; };
template<> 
struct GLType<const float> { static constexpr auto value = GL_FLOAT; };
template<> 
struct GLType<unsigned int> { static constexpr auto value = GL_UNSIGNED_INT; };
template<> 
struct GLType<const unsigned int> { static constexpr auto value = GL_UNSIGNED_INT; };
template<> 
struct GLType<unsigned char> { static constexpr auto value = GL_UNSIGNED_BYTE; };
template<> 
struct GLType<const unsigned char> { static constexpr auto value = GL_UNSIGNED_BYTE; };
template<> 
struct GLType<unsigned short> { static constexpr auto value = GL_UNSIGNED_SHORT; };
template<> 
struct GLType<const unsigned short> { static constexpr auto value = GL_UNSIGNED_SHORT; };

///////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER
///////////////////////////////////////////////////////////////////////////////////////////////////

GLShader::GLShader(std::string name)
    : name_(std::move(name)), id_(glCreateProgram())
{
    TRACE("New GLShader program '{}'[{}]", name_, id_);
}

GLShader::~GLShader()
{
    if (id_) {
        glDeleteProgram(id_);
        TRACE("Delete GLShader program '{}'[{}]", name_, id_);
    }
}

/// Load attributes' location into local array
void GLShader::load_attr_loc(GLAttr attr, std::string_view attr_name)
{
    const GLint loc = glGetAttribLocation(id_, attr_name.data());
    if (loc == -1)
        ABORT_MSG("Failed to get location for attribute '{}' GLShader '{}'[{}]", attr_name, name_, id_);
    TRACE("Loaded attritube '{}' location {} GLShader '{}'[{}]", attr_name, loc, name_, id_);
    attrs_[static_cast<size_t>(attr)] = loc;
}

/// Load uniforms' location into local array
void GLShader::load_unif_loc(GLUnif unif, std::string_view unif_name)
{
    const GLint loc = glGetUniformLocation(id_, unif_name.data());
    if (loc == -1)
        ABORT_MSG("Failed to get location for uniform '{}' GLShader '{}'[{}]", unif_name, name_, id_);
    TRACE("Loaded uniform '{}' location {} GLShader '{}'[{}]", unif_name, loc, name_, id_);
    unifs_[static_cast<size_t>(unif)] = loc;
}

/// Build a shader program from sources
auto GLShader::build(std::string name, std::string_view vert_src, std::string_view frag_src) -> std::optional<GLShader>
{
    auto shader = GLShader(std::move(name));
    auto vertex = shader.compile(GL_VERTEX_SHADER, vert_src.data());
    auto fragment = shader.compile(GL_FRAGMENT_SHADER, frag_src.data());
    if (!vertex || !fragment) {
        ERROR("Failed to Compile Shaders for program '{}'[{}]", shader.name_, shader.id_);
        if (vertex) glDeleteShader(*vertex);
        if (fragment) glDeleteShader(*fragment);
        return std::nullopt;
    }
    if (!shader.link(*vertex, *fragment)) {
        ERROR("Failed to Link GLShader program '{}'[{}]", shader.name_, shader.id_);
        glDeleteShader(*vertex);
        glDeleteShader(*fragment);
        return std::nullopt;
    }
    glDeleteShader(*vertex);
    glDeleteShader(*fragment);
    TRACE("Compiled&Linked shader program '{}'[{}]", shader.name_, shader.id_);
    return shader;
}

/// Compile a single shader from sources
auto GLShader::compile(GLenum shader_type, const char* shader_src) -> std::optional<GLuint>
{
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &shader_src, nullptr);
    glCompileShader(shader);
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len) {
        auto info = std::make_unique<char[]>(info_len);
        glGetShaderInfoLog(shader, info_len, nullptr, info.get());
        DEBUG("GLShader '{}'[{}] Compilation Output {}:\n{}", name_, id_, shader_type_str(shader_type), info.get());
    }
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        ERROR("Failed to Compile {} for GLShader '{}'[{}]", shader_type_str(shader_type), name_, id_);
        glDeleteShader(shader);
        return std::nullopt;
    }
    return shader;
}

/// Link shaders into program object
bool GLShader::link(GLuint vert, GLuint frag)
{
    glAttachShader(id_, vert);
    glAttachShader(id_, frag);
    glLinkProgram(id_);
    GLint info_len = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len) {
        auto info = std::make_unique<char[]>(info_len);
        glGetProgramInfoLog(id_, info_len, nullptr, info.get());
        DEBUG("GLShader '{}'[{}] Program Link Output:\n{}", name_, id_, info.get());
    }
    GLint link_status = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &link_status);
    if (!link_status)
        ERROR("Failed to Link GLShader Program '{}'[{}]", name_, id_);
    glDetachShader(id_, vert);
    glDetachShader(id_, frag);
    return link_status;
}

/// Stringify opengl shader type.
auto GLShader::shader_type_str(GLenum shader_type) -> std::string_view
{
    switch (shader_type) {
        case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
        case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
        default: ASSERT_MSG(0, "Invalid shader type {}", shader_type); return "<invalid>";
    }
}

/// Core Generic Shader
static Ref<GLShader> generic_shader;

/// Get generic shader loaded by default
const GLShader& default_shader()
{
    return *generic_shader;
}

/// Load Generic Shader
/// (supports rendering: Colored objects, Textured objects and BitmapFont text)
void load_generic_shader()
{
    static constexpr std::string_view kShaderVert = R"(
#version 330 core
in vec3 aPosition;
in vec2 aTexCoord;
in vec4 aColor;
out vec4 fColor;
out vec2 fTexCoord;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
void main()
{
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0f);
    fTexCoord = aTexCoord;
    fColor = aColor;
}
)";

    static constexpr std::string_view kShaderFrag = R"(
#version 330 core
in vec2 fTexCoord;
in vec4 fColor;
out vec4 outColor;
uniform sampler2D uTexture0;
void main()
{
    outColor = texture(uTexture0, fTexCoord) * fColor;
}
)";

    DEBUG("Loading Generic Shader");
    auto shader = GLShader::build("GenericShader", kShaderVert, kShaderFrag);
    ASSERT(shader);
    shader->bind();
    shader->load_attr_loc(GLAttr::POSITION, "aPosition");
    shader->load_attr_loc(GLAttr::TEXCOORD, "aTexCoord");
    shader->load_attr_loc(GLAttr::COLOR, "aColor");
    shader->load_unif_loc(GLUnif::MODEL, "uModel");
    shader->load_unif_loc(GLUnif::VIEW, "uView");
    shader->load_unif_loc(GLUnif::PROJECTION, "uProjection");

    generic_shader = std::make_shared<GLShader>(std::move(*shader));
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// TEXTURE
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Load a texture file from give path into GPU memory
GLTextureRef load_texture(std::string_view inpath, GLenum filter)
{
    //const std::string filepath = SPACESHIP_ASSETS_PATH + "/"s + inpath;
    const std::string filepath = inpath.data();
    auto file = read_file_to_string(filepath);
    if (!file) { ERROR("Failed to read texture path ({})", filepath); return nullptr; }
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load_from_memory((const uint8_t*)file->data(), file->length(), &width, &height, &channels, 0);
    if (!data) { ERROR("Failed to load texture path ({})", filepath); return nullptr; }
    ASSERT_MSG(channels == 4 || channels == 3, "actual channels: {}", channels);
    GLenum type = (channels == 4) ? GL_RGBA : GL_RGB;
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return GLTexture{ texture }.to_ref();
}

/// 1x1 pixel default white texture
static GLTextureRef white_texture = nullptr;

/// Load a default 1x1 white texture for drawing color-filled only objects
void load_white_texture()
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    unsigned char data[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    white_texture = GLTexture{ texture }.to_ref();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// WINDOW
///////////////////////////////////////////////////////////////////////////////////////////////////

static GLFWwindow* window = nullptr;

/// Load OpenGL pointers
static void load_opengl()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        CRITICAL("Failed to load OpenGL");
    }
}

/// Initialize the Window with OpenGL context and core library globals
auto init_window(int width, int height, const char* title) -> Context
{
    /* TODO */
    spdlog::set_default_logger(spdlog::stdout_color_mt("sgl"));
    spdlog::set_pattern("%Y-%m-%d %T.%e <%^%l%$> [%n] %s:%#: %!() -> %v");
    spdlog::set_level(spdlog::level::debug);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        CRITICAL("Failed to create GLFW window");
        glfwTerminate();
        return Context();
    }
    glfwMakeContextCurrent(window);

    // callbacks (TODO)
    //glfwSetKeyCallback(window, key_event_callback);
    //glfwSetWindowFocusCallback(window, window_focus_callback);
    //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetCursorPosCallback(window, cursor_position_callback);

    // settings
    glfwSetWindowAspectRatio(window, width, height);
    glfwSwapInterval(1); // vsync

    // OpenGL
    load_opengl();

    // Default resources
    load_generic_shader();
    load_white_texture();

    return Context{};
}

/// Finalize the core and close the window
void close_window()
{
    generic_shader.reset();
    white_texture.reset();

    glfwTerminate();
    window = nullptr;
}

/// Check if the window should close
bool window_should_close()
{
    return glfwWindowShouldClose(window);
}

/// Poll window events (mouse, keyboard, system)
void poll_events()
{
    glfwPollEvents();
}

/// Get time since window init
double get_time()
{
    return glfwGetTime();
}

Context::~Context()
{
    if (window)
        close_window();
}

bool Context::is_open() const
{
    return window != nullptr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// RENDERING
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Prepare to render
void begin_render(Color color)
{
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glm::vec4 c = color.value();
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 mat(1.f);
    glUniformMatrix4fv(default_shader().unif_loc(GLUnif::VIEW), 1, GL_FALSE, glm::value_ptr(mat));
    glUniformMatrix4fv(default_shader().unif_loc(GLUnif::PROJECTION), 1, GL_FALSE, glm::value_ptr(mat));
}

/// End rendering procedure
void end_render()
{
    glfwSwapBuffers(window);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// DRAWING
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Draw a generic object (textured or colored)
void draw_object(const Object& obj) {
    if (!obj.m_glo)
        return;

    // aliases
    const GLShader& shader = default_shader();
    const GLObject& glo = *obj.m_glo;

    // set uniforms
    const glm::mat4 model = obj.m_transform.matrix();
    glUniformMatrix4fv(shader.unif_loc(GLUnif::MODEL), 1, GL_FALSE, glm::value_ptr(model));

    // set attribute default value
    const Color color = obj.m_color ? *obj.m_color : WHITE;
    glVertexAttrib4fv(shader.attr_loc(GLAttr::COLOR), (float*)&color);

    // bind texture
    const GLuint tex_id = obj.m_texture ? obj.m_texture->id : white_texture->id;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    // bind vao
    glBindVertexArray(obj.m_glo->vao);

    // draw object
    if (obj.m_glo->num_indices)
        glDrawElements(GL_TRIANGLES, obj.m_glo->num_indices, obj.m_glo->index_type, nullptr);
    else
        glDrawArrays(GL_TRIANGLES, 0, obj.m_glo->num_vertices);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// CREATION
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Describes a Vertex Arrray layout and holds the vertex data pointers
class VertexArray final {
  public:
    VertexArray(size_t num_vertices)
        : num_vertices(num_vertices) {
    }

    template<typename T>
    VertexArray& add_buffer(T *data) {
        if ((bindex+1) < (size_t)GLAttr::COUNT) {
            bindex++;
            buffers[bindex].ptr = (void*)data;
        }
        return *this;
    }

    template<typename T>
    VertexArray& add_attr(GLAttr idx, size_t count) {
        return add_attr_args(idx, count, GLType<T>::value, sizeof(T));
    }

    VertexArray& add_attr_args(GLAttr idx, size_t count, GLenum type, size_t size) {
        if (bindex < 0)
            return *this;
        auto& buf = buffers[bindex];
        auto& attr = buf.attrs[(size_t)idx];
        attr.type = type;
        attr.count = count;
        attr.size = size;
        attr.offset = buf.stride;
        buf.stride += (count * size);
        total_stride += (count * size);
        return *this;
    }

    template<typename T>
    VertexArray& add_indices(T *ptr, size_t count) {
        return add_indices_args((void*)ptr, count, GLType<T>::value, sizeof(T));
    }

    VertexArray& add_indices_args(void *ptr, size_t count, GLenum type, size_t size) {
        num_indices = count;
        indices = ptr;
        index_type = type;
        index_size = size;
        return *this;
    }

  public:
    struct Attr {
        GLenum type = GLenum(0);
        size_t count = 0;
        size_t size = 0;
        size_t offset = 0;
    };

    struct Buffer {
        Attr attrs[(size_t)GLAttr::COUNT] = {};
        void *ptr = nullptr;
        size_t stride = 0;
    };

  private:
    Buffer buffers[(size_t)GLAttr::COUNT]; // we can have 1 buffer per attribute
    int bindex = -1;                       // index to buffers
    size_t num_vertices = 0;               // number of vertices
    size_t num_indices = 0;                // number of elements
    void *indices = nullptr;               // elements buffer
    GLenum index_type = GLenum(0);         // index gl type
    size_t index_size = 0;                 // index size in bytes
    size_t total_stride = 0;               // size of one entire vertex

    friend GLObject create_globject(const GLShader& shader, const VertexArray& vertex_array, GLenum usage);
};

GLObject create_globject(const GLShader& shader, const VertexArray& vertex_array, GLenum usage = DEFAULT_GLO_USAGE)
{
    GLuint vbo = 0, ebo = 0, vao = 0;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Keep track of enabled attributes
    bool enabled_attrs[(size_t)GLAttr::COUNT] = { false };

    // Allocate VBO buffer
    size_t total_size = (vertex_array.num_vertices * vertex_array.total_stride);
    glBufferData(GL_ARRAY_BUFFER, total_size, NULL, usage);

    // Submit vertex data and configure attributes
    size_t buf_offset = 0;
    for (auto& buffer : vertex_array.buffers) {
        if (!buffer.ptr || !buffer.stride)
            continue;
        for (size_t attr_idx = 0; attr_idx < (size_t)GLAttr::COUNT; attr_idx++) {
            auto& attr = buffer.attrs[attr_idx];
            if (attr.count && attr.size) {
                auto attr_loc = shader.attr_loc((GLAttr)attr_idx);
                if (attr_loc < 0) {
                    ABORT_MSG("GLAttr %zu unknown to shader '%s'", attr_idx, shader.name()); 
                }
                enabled_attrs[attr_idx] = true;
                glEnableVertexAttribArray(attr_loc);
                glVertexAttribPointer(attr_loc, attr.count, attr.type, GL_FALSE, buffer.stride, (void*)attr.offset);
            }
        }
        size_t buf_size = (buffer.stride * vertex_array.num_vertices);
        glBufferSubData(GL_ARRAY_BUFFER, buf_offset, buf_size, buffer.ptr);
        buf_offset += buf_size;
    }

    // Disable unused attributes
    for (size_t attr_idx = 0; attr_idx < (size_t)GLAttr::COUNT; attr_idx++) {
        auto attr_loc = shader.attr_loc((GLAttr)attr_idx);
        if (attr_loc < 0 && !enabled_attrs[attr_idx])
            glDisableVertexAttribArray(attr_loc);
    }

    // Submit Index buffer
    if (vertex_array.indices && vertex_array.num_indices) {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        size_t size = (vertex_array.index_size * vertex_array.num_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, vertex_array.indices, usage);
    }

    return { vbo, ebo, vao, vertex_array.num_vertices, vertex_array.num_indices, vertex_array.index_type };
}

GLObject create_globject(VertexArray vertex_array, GLenum usage = DEFAULT_GLO_USAGE)
{
    return create_globject(default_shader(), vertex_array, usage);
}

static constexpr auto cuboid_positions(Size3 s)
{
    std::array<glm::vec3, 8> vertices = {glm::vec3
        /* i        X  ,   Y  ,   Z   */
        /*[0]*/ { -s->x, +s->y, +s->z },
        /*[1]*/ { -s->x, -s->y, +s->z },
        /*[2]*/ { +s->x, -s->y, +s->z },
        /*[3]*/ { +s->x, +s->y, +s->z },
        /*[4]*/ { -s->x, +s->y, -s->z },
        /*[5]*/ { -s->x, -s->y, -s->z },
        /*[6]*/ { +s->x, -s->y, -s->z },
        /*[7]*/ { +s->x, +s->y, -s->z },
    };
    return vertices;
}

/// Create a simple colored cuboid and load it into GPU buffers
Object create_cuboid(Size3 s, GLenum usage)
{
    const std::array<glm::vec3, 8> vertices = cuboid_positions(s);
    const std::array<unsigned char, 36> indices = {
        /* front  */ 0, 1, 2, 2, 3, 0,
        /* back   */ 4, 5, 6, 6, 7, 4,
        /* left   */ 4, 5, 1, 1, 0, 4,
        /* right  */ 3, 2, 6, 6, 7, 3,
        /* top    */ 0, 3, 4, 4, 7, 3,
        /* bottom */ 1, 2, 5, 5, 6, 2,
    };

    auto va = VertexArray(vertices.size())
        .add_buffer(vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_indices(indices.data(), indices.size());

    return Object().glo(create_globject(va, usage).to_ref());
}

Object create_color_cuboid(Size3 s, Color c[6], GLenum usage)
{
    const auto p = cuboid_positions(s);
    const std::array<std::pair<glm::vec3, glm::vec4>, 24> vertices = {std::pair<glm::vec3, glm::vec4>
        /* i    X ,   Y  ,   Z  ,  R ,  G ,  B ,  A  */
        /* FRONT */
        /*[ 0]*/ {p[0], c[0]},
        /*[ 1]*/ {p[1], c[0]},
        /*[ 2]*/ {p[2], c[0]},
        /*[ 3]*/ {p[3], c[0]},
        /* BACK */
        /*[ 4]*/ {p[4], c[1]},
        /*[ 5]*/ {p[5], c[1]},
        /*[ 6]*/ {p[6], c[1]},
        /*[ 7]*/ {p[7], c[1]},
        /* LEFT */
        /*[ 8]*/ {p[4], c[2]},
        /*[ 9]*/ {p[5], c[2]},
        /*[10]*/ {p[1], c[2]},
        /*[11]*/ {p[0], c[2]},
        /* RIGHT */
        /*[12]*/ {p[3], c[3]},
        /*[13]*/ {p[2], c[3]},
        /*[14]*/ {p[6], c[3]},
        /*[15]*/ {p[7], c[3]},
        /* TOP */
        /*[16]*/ {p[0], c[4]},
        /*[17]*/ {p[3], c[4]},
        /*[18]*/ {p[7], c[4]},
        /*[19]*/ {p[4], c[4]},
        /* BOTTOM */
        /*[20]*/ {p[1], c[5]},
        /*[21]*/ {p[2], c[5]},
        /*[22]*/ {p[6], c[5]},
        /*[23]*/ {p[5], c[5]},
    };
    const std::array<unsigned char, 36> indices = {
        /* front  */  0,  1,  2,  2,  3,  0,
        /* back   */  4,  5,  6,  6,  7,  4,
        /* left   */  8,  9, 10, 10, 11,  8,
        /* right  */ 12, 13, 14, 14, 15, 12,
        /* top    */ 16, 17, 18, 18, 19, 16,
        /* bottom */ 20, 21, 22, 22, 23, 20,
    };

    auto va = VertexArray(vertices.size())
        .add_buffer(vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_attr<float>(GLAttr::COLOR, 4)
        .add_indices(indices.data(), indices.size());

    return Object().glo(create_globject(va, usage).to_ref());
}

Object create_texture_cuboid(Size3 size, GLTextureRef texture, GLenum usage)
{
    const auto vertices = cuboid_positions(size);

    auto va = VertexArray(vertices.size())
        .add_buffer(vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_attr<float>(GLAttr::TEXCOORD, 2);

    return Object().glo(create_globject(va, usage).to_ref());
}

static constexpr auto rect_positions(Size2 s)
{
    std::array<glm::vec3, 8> vertices = {glm::vec3
        /* i        X  ,   Y  ,   Z   */
        /*[0]*/ { -s->x, +s->y, 0.f },
        /*[1]*/ { -s->x, -s->y, 0.f },
        /*[2]*/ { +s->x, -s->y, 0.f },
        /*[3]*/ { +s->x, +s->y, 0.f },
    };
    return vertices;
}

/// Create a simple color-filled rectangle and load it into GPU buffers
Object create_rect(Size2 size, GLenum usage)
{
    const auto vertices = rect_positions(size);
    const std::array<unsigned char, 6> indices = {
        0, 1, 2, 2, 3, 0,
    };

    auto va = VertexArray(vertices.size())
        .add_buffer(vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_indices(indices.data(), indices.size());

    return Object().glo(create_globject(va, usage).to_ref());
}

Object create_color_rect(Size2 size, Color c, GLenum usage)
{
    const auto p = rect_positions(size);
    const std::array<std::pair<glm::vec3, glm::vec4>, 4> vertices = {std::pair<glm::vec3, glm::vec4>
        /* i    X ,   Y  ,   Z  ,  R ,  G ,  B ,  A  */
        /*[ 0]*/ {p[0], c},
        /*[ 1]*/ {p[1], c},
        /*[ 2]*/ {p[2], c},
        /*[ 3]*/ {p[3], c},
    };
    const std::array<unsigned char, 6> indices = {
        0, 1, 2, 2, 3, 0,
    };

    auto va = VertexArray(vertices.size())
        .add_buffer(vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_attr<float>(GLAttr::COLOR, 4)
        .add_indices(indices.data(), indices.size());

    return Object().glo(create_globject(va, usage).to_ref());
}

Object create_texture_rect(Size2 size, GLenum usage)
{
    return create_texture_rect(size, nullptr, usage);
}

Object create_texture_rect(Size2 size, GLTextureRef texture, GLenum usage)
{
    return create_texture_rect(size, texture, Rect(), usage);
}

Object create_texture_rect(Size2 size, GLTextureRef texture, Rect r, GLenum usage)
{
    const auto p = rect_positions(size);
    const std::array<std::pair<glm::vec3, glm::vec2>, 4> vertices = {std::pair<glm::vec3, glm::vec2>
        /* i    X ,   Y  ,   Z  ,  S  ,  T  */
        /*[ 0]*/ {p[0], {r.x0, r.y0} },
        /*[ 1]*/ {p[1], {r.x0, r.y1} },
        /*[ 2]*/ {p[2], {r.x1, r.y1} },
        /*[ 3]*/ {p[3], {r.x1, r.y0} },
    };
    const std::array<unsigned char, 6> indices = {
        0, 1, 2, 2, 3, 0,
    };

    auto va = VertexArray(vertices.size())
        .add_buffer(vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_attr<float>(GLAttr::TEXCOORD, 2)
        .add_indices(indices.data(), indices.size());

    return Object().glo(create_globject(va, usage).to_ref()).texture(texture);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// MODEL
///////////////////////////////////////////////////////////////////////////////////////////////////

auto load_mtl(const std::string& filename) -> std::optional<Material>
{
    auto file = std::ifstream(filename);
    if (!file.good()) {
        ERROR("Error opening MTL file '{}', error: {}", filename.data(), std::strerror(errno));
        return std::nullopt;
    }

    Material material;
    char buf[BUFSIZ];
    while (file.getline(buf, sizeof(buf))) {
        std::stringstream ss(buf);
        std::string code;
        ss >> code;
        if (code == "map_Kd") {
            std::string texture;
            ss >> texture;
            Material material;
            auto tex_path = std::filesystem::path(filename).remove_filename().append(texture).string();
            material.diffuse_tex = load_texture(tex_path, GL_LINEAR);
            return material;
        }
    }

    return std::nullopt;
}

ModelRef load_model(std::string_view filepath)
{
    auto file = std::ifstream(filepath.data());
    if (!file.good()) {
        ERROR("Failed to open OBJ file {}, error: {}", filepath.data(), std::strerror(errno));
        return nullptr;
    }

    Model model;
    Mesh* curr_mesh = &model.meshes[0];
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    char buf[BUFSIZ];
    while (file.getline(buf, sizeof(buf))) {
        std::stringstream ss(buf);
        std::string code;
        ss >> code;
        if (code == "v") {
            glm::vec3 v;
            ss >> v.x; ss >> v.y; ss >> v.z;
            positions.push_back(v);
        }
        else if (code == "vn") {
            glm::vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (code == "vt") {
            glm::vec2 vt;
            ss >> vt.x; ss >> vt.y;
            texcoords.push_back(vt);
        }
        else if (code == "f") {
            glm::u32vec3 fv; // positions
            glm::u32vec3 fvt; // textcoord
            glm::u32vec3 fvn; // normal
            ss >> fv[0]; ss.get() /*slash*/; ss >> fvt[0]; ss.get() /*slash*/; ss >> fvn[0];
            ss >> fv[1]; ss.get() /*slash*/; ss >> fvt[1]; ss.get() /*slash*/; ss >> fvn[1];
            ss >> fv[2]; ss.get() /*slash*/; ss >> fvt[2]; ss.get() /*slash*/; ss >> fvn[2];
            fv -= glm::u32vec3(1); /* index is offset by 1 */
            fvt -= glm::u32vec3(1); /* index is offset by 1 */
            fvn -= glm::u32vec3(1); /* index is offset by 1 */
            for (int i : {0, 1, 2}) {
                curr_mesh->vertices.push_back(positions[fv[i]].x);
                curr_mesh->vertices.push_back(positions[fv[i]].y);
                curr_mesh->vertices.push_back(positions[fv[i]].z);
                curr_mesh->vertices.push_back(texcoords[fvt[i]].s);
                curr_mesh->vertices.push_back(texcoords[fvt[i]].t);
            }
        }
        else if (code == "mtllib") {
            std::string mtllib_str;
            ss >> mtllib_str;
            auto mtlpath = std::filesystem::path(filepath).remove_filename().append(mtllib_str).string();
            auto mtl = load_mtl(mtlpath);
            if (!mtl) {
                ERROR("Failed to read MTL file: {}", mtlpath);
                return nullptr;
            }
            curr_mesh->material = mtl->to_ref();
        }
    }

    return std::make_shared<Model>(std::move(model));
}

Object create_mesh(const Mesh& mesh, GLenum usage)
{
    constexpr auto kFloatsPerVertex = 5;
    auto va = VertexArray(mesh.vertices.size() / kFloatsPerVertex)
        .add_buffer(mesh.vertices.data())
        .add_attr<float>(GLAttr::POSITION, 3)
        .add_attr<float>(GLAttr::TEXCOORD, 2);

    return Object().glo(create_globject(va, usage).to_ref()).texture(mesh.material->diffuse_tex);
}

} // namespace sgl


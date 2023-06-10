#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

/// Simple Graphics Library
namespace sgl {


///////////////////////////////////////////////////////////////////////////////////////////////////
// LOG
///////////////////////////////////////////////////////////////////////////////////////////////////

#define TRACE SPDLOG_TRACE
#define DEBUG SPDLOG_DEBUG
#define INFO SPDLOG_INFO
#define WARN SPDLOG_WARN
#define ERROR SPDLOG_ERROR
#define CRITICAL SPDLOG_CRITICAL
#define ABORT_MSG(...) do { CRITICAL(__VA_ARGS__); std::abort(); } while (0)
#define ASSERT_MSG(expr, ...) do { if (expr) { } else { ABORT_MSG(__VA_ARGS__); } } while (0)
#define ASSERT(expr) ASSERT_MSG(expr, "Assertion failed: ({})", #expr)
#define ASSERT_RET(expr) [&]{ auto ret = (expr); ASSERT_MSG(ret, "Assertion failed: ({})", #expr); return ret; }()
#define DBG(expr) [&]{ auto ret = (expr); DEBUG("({}) = {{{}}}", #expr, ret); return ret; }()


///////////////////////////////////////////////////////////////////////////////////////////////////
// UTILS
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Default core reference type
template<typename ...T>
using Ref = std::shared_ptr<T...>;

/// Utility type to make unique numbers (IDs) movable, when moved the value should be zero
template<typename T>
struct UniqueNum final {
  T inner;
  UniqueNum() : inner(0) {}
  UniqueNum(T n) : inner(n) {}
  UniqueNum(UniqueNum&& o) : inner(o.inner) { o.inner = 0; }
  UniqueNum& operator=(UniqueNum&& o) { inner = o.inner; o.inner = 0; return *this; }
  UniqueNum(const UniqueNum&) = delete;
  UniqueNum& operator=(const UniqueNum&) = delete;
  operator T() const { return inner; }
};

/// Read file contents to a string
auto read_file_to_string(const std::string& filename) -> std::optional<std::string>;


///////////////////////////////////////////////////////////////////////////////////////////////////
// COLORS
///////////////////////////////////////////////////////////////////////////////////////////////////
class Color final {
  public:
    constexpr Color() : inner() {}
    constexpr Color(float r, float g, float b, float a = 1.f) : inner{r, g, b, a} {}
    constexpr Color(glm::vec3 v) : inner{v, 1.f} {}
    constexpr Color(glm::vec4 v) : inner{v} {}
    constexpr operator glm::vec3() const { return inner; }
    constexpr operator glm::vec4() const { return inner; }
    constexpr auto operator->() { return &inner; }
    constexpr const auto operator->() const { return &inner; }
    constexpr glm::vec4& value() { return inner; }
    constexpr const glm::vec4& value() const { return inner; }
  private:
    glm::vec4 inner;
};

[[maybe_unused]] inline constexpr Color BLACK      = {0.0f, 0.0f, 0.0f};
[[maybe_unused]] inline constexpr Color WHITE      = {1.0f, 1.0f, 1.0f};
[[maybe_unused]] inline constexpr Color GRAY       = {0.5f, 0.5f, 0.5f};
[[maybe_unused]] inline constexpr Color DARK_GRAY  = {0.1f, 0.1f, 0.1f};
[[maybe_unused]] inline constexpr Color LIGHT_GRAY = {0.9f, 0.9f, 0.9f};
[[maybe_unused]] inline constexpr Color RED        = {1.0f, 0.0f, 0.0f};
[[maybe_unused]] inline constexpr Color GREEN      = {0.0f, 1.0f, 0.0f};
[[maybe_unused]] inline constexpr Color BLUE       = {0.0f, 0.0f, 1.0f};


///////////////////////////////////////////////////////////////////////////////////////////////////
// SHADER
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Enumeration of supported Shader Attributes
enum class GLAttr {
    POSITION,
    COLOR,
    NORMAL,
    TEXCOORD,
    COUNT, // must be last
};

/// Enumeration of supported Shader Uniforms
enum class GLUnif {
    MODEL,
    VIEW,
    PROJECTION,
    COUNT, // must be last
};

/// GLShader represents an OpenGL shader program
class GLShader final {
    /// Program name
    std::string name_;
    /// Program ID
    UniqueNum<unsigned int> id_;
    /// Attributes' location
    std::array<GLint, static_cast<size_t>(GLAttr::COUNT)> attrs_ = { -1 };
    /// Uniforms' location
    std::array<GLint, static_cast<size_t>(GLUnif::COUNT)> unifs_ = { -1 };

    public:
    explicit GLShader(std::string name);
    ~GLShader();

    // Movable but not Copyable
    GLShader(GLShader&& o) = default;
    GLShader(const GLShader&) = delete;
    GLShader& operator=(GLShader&&) = default;
    GLShader& operator=(const GLShader&) = delete;

    public:
    /// Get shader program name
    [[nodiscard]] std::string_view name() const { return name_; }

    /// Bind shader program
    void bind() { glUseProgram(id_); }
    /// Unbind shader program
    void unbind() { glUseProgram(0); }

    /// Get attribute location
    [[nodiscard]] GLint attr_loc(GLAttr attr) const { return attrs_[static_cast<size_t>(attr)]; }
    /// Get uniform location
    [[nodiscard]] GLint unif_loc(GLUnif unif) const { return unifs_[static_cast<size_t>(unif)]; }

    /// Load attributes' location into local array
    void load_attr_loc(GLAttr attr, std::string_view attr_name);

    /// Load uniforms' location into local array
    void load_unif_loc(GLUnif unif, std::string_view unif_name);

    public:
    /// Build a shader program from sources
    static auto build(std::string name, std::string_view vert_src, std::string_view frag_src) -> std::optional<GLShader>;

    private:
    /// Compile a single shader from sources
    auto compile(GLenum shader_type, const char* shader_src) -> std::optional<GLuint>;

    /// Link shaders into program object
    bool link(GLuint vert, GLuint frag);

    /// Stringify opengl shader type.
    static auto shader_type_str(GLenum shader_type) -> std::string_view;
};


/// Get generic shader loaded by default
const GLShader& default_shader();


///////////////////////////////////////////////////////////////////////////////////////////////////
// TEXTURE
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Represents a texture loaded to GPU memory
struct GLTexture final {
    UniqueNum<GLuint> id;

    ~GLTexture() {
        if (id) glDeleteTextures(1, &id.inner);
    }

    // Movable but not Copyable
    GLTexture(GLTexture&&) = default;
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(GLTexture&&) = default;
    GLTexture& operator=(const GLTexture&) = delete;

    Ref<GLTexture> to_ref() { return std::make_shared<GLTexture>(std::move(*this)); }
};

using GLTextureRef = Ref<GLTexture>;

/// Load a texture file from give path into GPU memory
GLTextureRef load_texture(std::string_view path, GLenum filter);


///////////////////////////////////////////////////////////////////////////////////////////////////
// SPACE
///////////////////////////////////////////////////////////////////////////////////////////////////

struct Pos2 {
    glm::vec2 inner;
    constexpr Pos2(float v) : inner(v) {}
    constexpr Pos2(float x, float y) : inner(x, y) {}
};

struct Pos3 {
    glm::vec3 inner;
    constexpr Pos3() : inner() {}
    constexpr Pos3(Pos2 p2) : inner(p2.inner, 1.f) {};
    constexpr Pos3(float x, float y, float z) : inner(x, y, z) {}
    constexpr Pos3(float v) : inner(v) {}
};

struct Size2 {
    glm::vec2 inner;
    constexpr Size2() : inner() {}
    constexpr Size2(float v) : inner(v) {}
    constexpr Size2(float x, float y) : inner(x, y) {}
    constexpr auto operator->() { return &inner; }
    constexpr const auto operator->() const { return &inner; }
    constexpr operator glm::vec2() const { return inner; }
};

struct Size3 {
    glm::vec3 inner;
    constexpr Size3() : inner() {}
    constexpr Size3(Size2 s) : inner(s.inner, 0.f) {}
    constexpr Size3(float v) : inner(v) {}
    constexpr Size3(float x, float y, float z) : inner(x, y, z) {}
    constexpr auto operator->() { return &inner; }
    constexpr const auto operator->() const { return &inner; }
    constexpr operator glm::vec3() const { return inner; }
};

struct Rect {
    union {
        Pos2 top_left;
        Pos2 tl;
        struct {
            float x0;
            float y0;
        };
    };
    union {
        Pos2 bottom_right;
        Pos2 br;
        struct {
            float x1;
            float y1;
        };
    };
    constexpr Rect() : top_left(0.f), bottom_right(1.f) {}
};

struct Transform {
    Pos3 position;
    Size3 scale;
    glm::vec3 rotation;

    constexpr operator glm::mat4() const { return glm::mat4(1.f); }

    glm::mat4 matrix() const {
        glm::mat4 matrix(1.0f);
        matrix = glm::translate(matrix, position.inner);
        matrix = glm::rotate(matrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        matrix = glm::rotate(matrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        matrix = glm::rotate(matrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        matrix = glm::scale(matrix, scale.inner);
        return matrix;
    }
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// OBJECTS
///////////////////////////////////////////////////////////////////////////////////////////////////

struct GLObject final {
    UniqueNum<GLuint> vbo;
    UniqueNum<GLuint> ebo;
    UniqueNum<GLuint> vao;
    size_t num_vertices;
    size_t num_indices;
    GLenum index_type;

    ~GLObject() {
        if (vbo) glDeleteBuffers(1, &vbo.inner);
        if (ebo) glDeleteBuffers(1, &ebo.inner);
        if (vao) glDeleteVertexArrays(1, &vao.inner);
    }

    // Movable but not Copyable
    GLObject(GLObject&&) = default;
    GLObject(const GLObject&) = delete;
    GLObject& operator=(GLObject&&) = default;
    GLObject& operator=(const GLObject&) = delete;

    Ref<GLObject> to_ref() { return std::make_shared<GLObject>(std::move(*this)); }
};

using GLObjectRef = Ref<GLObject>;


struct Object {
    GLObjectRef m_glo;
    Object& glo(GLObjectRef g) { m_glo = std::move(g); return *this; }

    std::optional<Color> m_color;
    Object& color(std::optional<Color> c) { m_color = std::move(c); return *this; }

    GLTextureRef m_texture;
    Object& texture(GLTextureRef t) { m_texture = std::move(t); return *this; }

    Transform m_transform;
    Object& scale(Size3 s) { m_transform.scale = s; return *this; }
    Object& rotate(glm::vec3 r) { m_transform.rotation = r; return *this; }
    Object& position(Pos3 p) { m_transform.position = p; return *this; }
    Object& transform(Transform t) { m_transform = t; return *this; }
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// WINDOW
///////////////////////////////////////////////////////////////////////////////////////////////////

struct Context final {
    ~Context();
    bool is_open() const;
};

/// Initialize the Window with OpenGL context and core library globals
[[nodiscard]]
auto init_window(int width, int height, const char* title) -> Context;

/// Check if the window should close
[[nodiscard]]
bool window_should_close();

/// Poll window events (mouse, keyboard, system)
void poll_events();

/// Get time since window init
[[nodiscard]]
double get_time();


///////////////////////////////////////////////////////////////////////////////////////////////////
// RENDERING
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Prepare to render and clear background
void begin_render(Color color);

/// End rendering procedure
void end_render();


///////////////////////////////////////////////////////////////////////////////////////////////////
// DRAWING
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Draw any object using default settings
void draw_object(const Object& obj);


///////////////////////////////////////////////////////////////////////////////////////////////////
// CREATION
///////////////////////////////////////////////////////////////////////////////////////////////////

/// Default usage of GL buffers for creating GLObjects
constexpr const GLenum DEFAULT_GLO_USAGE = GL_STATIC_DRAW;

/// Create a cuboid and load it into GPU buffers
Object create_cuboid(Size3 size, GLenum usage = DEFAULT_GLO_USAGE);
Object create_color_cuboid(Size3 size, Color color[6], GLenum usage = DEFAULT_GLO_USAGE);
Object create_texture_cuboid(Size3 size, GLenum usage = DEFAULT_GLO_USAGE);

/// Create a rectangle and load it into GPU buffers
Object create_rect(Size2 size, GLenum usage = DEFAULT_GLO_USAGE);
Object create_color_rect(Size2 size, Color color, GLenum usage = DEFAULT_GLO_USAGE);
Object create_texture_rect(Size2 size, GLenum usage = DEFAULT_GLO_USAGE);
Object create_texture_rect(Size2 size, GLTextureRef texture, GLenum usage = DEFAULT_GLO_USAGE);
Object create_texture_rect(Size2 size, GLTextureRef texture, Rect texcoord, GLenum usage = DEFAULT_GLO_USAGE);

/// Create a simple textured cuboid and load it into GPU buffers
//Object create_texture_cuboid(Size3 size, GLTextureRef texture);

//GLObject create_color_cube_glo(Size3 size, Color color);
//GLObject create_color_cube_glo(Size3 size, Color color, GLenum usage);
//GLObject create_color_cube_glo(const GLShader& shader, Size3 size, Color color, GLenum usage);
//GLObject create_color_sphere_glo(const GLShader& shader, float radius, Color color, GLenum usage);
//GLObject create_color_mesh_glo(const GLShader& shader, Size3 size, Color color, GLenum usage);


} // namespace sgl


/* Hello Triangle - c�digo adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gr�fico - Jogos Digitais - Unisinos
 * Vers�o inicial: 7/4/2017
 * �ltima atualiza��o em 12/05/2023
 *
 */

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <optional>
#include <vector>
#include <filesystem>
#include <map>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// Prot�tipo da fun��o de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Prot�tipos das fun��es
int setupShader();
int setupGeometry(const struct Obj& obj);
int setupTexture(const std::string& filepath, GLenum min_filter, GLenum mag_filter = 0);

// Dimens�es da janela (pode ser alterado em tempo de execu��o)
const GLuint WIDTH = 720, HEIGHT = 720;

// C�digo fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 texcoord_in;\n"
"uniform mat4 model;\n"
"out vec2 texcoord;\n"
"void main()\n"
"{\n"
//...pode ter mais linhas de c�digo aqui!
"gl_Position = model * vec4(position, 1.0);\n"
"texcoord = texcoord_in;\n"
"}\0";

//C�difo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 330\n"
"in vec2 texcoord;\n"
"out vec4 color;\n"
"uniform sampler2D texbuffer;\n"
"void main()\n"
"{\n"
"color = texture(texbuffer, texcoord);\n"
"}\n\0";

bool rotateX=false, rotateY=false, rotateZ=false;

struct Material {
    std::string texture_path;
};

auto parse_mtl(const std::string& filename) -> std::optional<Material>
{
    auto file = std::ifstream(filename);
    if (!file.good()) {
        cerr << "Error opening MTL file '" << filename
             << "', error: " << std::strerror(errno) << endl;
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
            material.texture_path = std::filesystem::path(filename).remove_filename().append(texture).string();
            return material;
        }
    }

    return std::nullopt;
}

struct Vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
};

struct Obj {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::u32vec3> triangle_indices;
    std::vector<Vertex> vertices;
    Material material;
};

auto parse_obj(const std::string& filename) -> std::optional<Obj>
{
    auto file = std::ifstream(filename);
    if (!file.good()) {
        cerr << "Error opening OBJ file '" << filename
             << "', error: " << std::strerror(errno) << endl;
        return std::nullopt;
    }

    Obj obj;
    char buf[BUFSIZ];
    while (file.getline(buf, sizeof(buf))) {
        std::stringstream ss(buf);
        std::string code;
        ss >> code;
        if (code == "v") {
            glm::vec3 v;
            ss >> v.x; ss >> v.y; ss >> v.z;
            obj.positions.push_back(v);
        }
        else if (code == "vn") {
            glm::vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            obj.normals.push_back(vn);
        }
        else if (code == "vt") {
            glm::vec2 vt;
            ss >> vt.x; ss >> vt.y;
            obj.texcoords.push_back(vt);
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
            obj.triangle_indices.push_back(fv);
            Vertex v0;
            v0.position = obj.positions[fv[0]];
            v0.texcoord = obj.texcoords[fvt[0]];
            obj.vertices.push_back(v0);
            Vertex v1;
            v1.position = obj.positions[fv[1]];
            v1.texcoord = obj.texcoords[fvt[1]];
            obj.vertices.push_back(v1);
            Vertex v2;
            v2.position = obj.positions[fv[2]];
            v2.texcoord = obj.texcoords[fvt[2]];
            obj.vertices.push_back(v2);
        }
        else if (code == "mtllib") {
            std::string mtllib_str;
            ss >> mtllib_str;
            auto mtlpath = std::filesystem::path(filename).remove_filename().append(mtllib_str).string();
            auto mtl = parse_mtl(mtlpath);
            if (!mtl) {
                cerr << "Failed to read MTL file: " << mtlpath << endl;
                return std::nullopt;
            }
            obj.material = *mtl;
        }
    }

    printf("Parsed OBJ file '%s' with %zu positions, %zu normals, %zu texcoords, %zu triangles and %zu vertices\n",
           filename.c_str(), obj.positions.size(), obj.normals.size(), obj.texcoords.size(), obj.triangle_indices.size(), obj.vertices.size());

    return obj;
}

// Fun��o MAIN
int main()
{
    // Inicializa��o da GLFW
    glfwInit();

    //Muita aten��o aqui: alguns ambientes n�o aceitam essas configura��es
    //Voc� deve adaptar para a vers�o do OpenGL suportada por sua placa
    //Sugest�o: comente essas linhas de c�digo para desobrir a vers�o e
    //depois atualize (por exemplo: 4.5 com 4 e 5)
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Essencial para computadores da Apple
//#ifdef __APPLE__
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif

    // Cria��o da janela GLFW
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Natanael!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da fun��o de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);

    // GLAD: carrega todos os ponteiros d fun��es da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;

    }

    // Obtendo as informa��es de vers�o
    const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
    const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Carrega o OBJ
    auto obj = parse_obj("../../3D_Models/Suzanne/SuzanneTriTextured.obj");
    //auto obj = parse_obj("../../3D_Models/Suzanne/CuboTextured.obj");
    //auto obj = parse_obj("../../3D_Models/Suzanne/suzanneTriLowPoly.obj");
    //auto obj = parse_obj("../../3D_Models/Cube/cube.obj");
    if (!obj) {
        cerr << "Failed to load 3D Model" << endl;
        return 1;
    }

    // Definindo as dimens�es da viewport com as mesmas dimens�es da janela da aplica��o
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);


    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Gerando um buffer simples, com a geometria de um tri�ngulo
    GLuint VAO = setupGeometry(*obj);

    GLuint TexID = setupTexture(obj->material.texture_path, GL_LINEAR);

    glUseProgram(shaderID);

    glm::mat4 model = glm::mat4(1); //matriz identidade;
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    //
    model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, false, glm::value_ptr(model));

    glEnable(GL_DEPTH_TEST);


    // Loop da aplica��o - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as fun��es de callback correspondentes
        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //cor de fundo
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

         glLineWidth(2);
        // glPointSize(20);

        float angle = (GLfloat)glfwGetTime();

        model = glm::mat4(1);
        if (rotateX)
        {
            model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));

        }
        if (rotateY)
        {
            model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

        }
        if (rotateZ)
        {
            model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

        }
        model = glm::scale(model, glm::vec3(0.6f));

        glUniformMatrix4fv(modelLoc, 1, false, glm::value_ptr(model));
        // Chamada de desenho - drawcall
        // Poligono Preenchido - GL_TRIANGLES

        // Draw the faces
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        //glBindVertexArray(VAO);
        //glVertexAttrib3f(1, 1.f, 0.f, 0.f); // default color for attribute color
        //glDrawElements(GL_TRIANGLES, obj->triangle_indices.size() * 3, GL_UNSIGNED_INT, 0);

        // Draw the LINES
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //glBindVertexArray(VAO);
        //glVertexAttrib3f(1, 0.f, 0.f, 0.f); // default color for attribute color
        //glDrawElements(GL_TRIANGLES, obj->triangle_indices.size() * 3, GL_UNSIGNED_INT, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TexID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, obj->vertices.size());

        // Chamada de desenho - drawcall
        // CONTORNO - GL_LINE_LOOP

        // glDrawArrays(GL_POINTS, 0, 18);
        // glBindVertexArray(0);

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }
    // Pede pra OpenGL desalocar os buffers
    glDeleteVertexArrays(1, &VAO);
    // Finaliza a execu��o da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}

// Fun��o de callback de teclado - s� pode ter uma inst�ncia (deve ser est�tica se
// estiver dentro de uma classe) - � chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        rotateX = !rotateX;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        rotateY = !rotateY;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        rotateZ = !rotateZ;
    }
}

//Esta fun��o est� basntante hardcoded - objetivo � compilar e "buildar" um programa de
// shader simples e �nico neste exemplo de c�digo
// O c�digo fonte do vertex e fragment shader est� nos arrays vertexShaderSource e
// fragmentShader source no ini�io deste arquivo
// A fun��o retorna o identificador do programa de shader
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compila��o (exibi��o via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Checando erros de compila��o (exibi��o via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int setupGeometry(const struct Obj& obj)
{
    GLuint VBO, EBO, VAO;

    //Gera��o do identificador do VBO
    glGenBuffers(1, &VBO);

    //Gera��o do identificador do EBO
    //glGenBuffers(1, &EBO);

    //Faz a conex�o (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //Envia os dados do array de floats para o buffer da OpenGl
    size_t vertex_stride_size = sizeof(Vertex);
    size_t vertex_total_size = obj.vertices.size() * vertex_stride_size;
    glBufferData(GL_ARRAY_BUFFER, vertex_total_size, obj.vertices.data(), GL_STATIC_DRAW);

    //Gera��o do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);

    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de v�rtices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);

    //Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
    // Localiza��o no shader * (a localiza��o dos atributos devem ser correspondentes no layout especificado no vertex shader)
    // Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
    // Tipo do dado
    // Se est� normalizado (entre zero e um)
    // Tamanho em bytes
    // Deslocamento a partir do byte zero

    //Atributo posi��o (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride_size, (GLvoid*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    //Atributo cor (r, g, b)
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    //glEnableVertexAttribArray(1);

    //Atributo texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertex_stride_size, (GLvoid*)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(1);

    // Default value for color attribute
    //glVertexAttrib3f(1, 1.f, 0.f, 0.f);
    //glDisableVertexAttribArray(1);

    // Carrega os indices
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //size_t index_stride_size = sizeof(decltype(obj.triangle_indices)::value_type);
    //size_t index_total_size = obj.triangle_indices.size() * index_stride_size;
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_total_size, obj.triangle_indices.data(), GL_STATIC_DRAW);


    // Observe que isso � permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de v�rtice
    // atualmente vinculado - para que depois possamos desvincular com seguran�a
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Desvincula o VAO (� uma boa pr�tica desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}

int setupTexture(const std::string& filepath, GLenum min_filter, GLenum mag_filter)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (!data) { cerr << "Failed to load texture path " << filepath; abort(); }
    if (!(channels == 4 || channels == 3)) { cerr << "num channels not supported, texture: " << filepath; abort(); }
    GLenum type = (channels == 4) ? GL_RGBA : GL_RGB;
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter != GLenum(0) ? mag_filter : min_filter);
    glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return texture;
}

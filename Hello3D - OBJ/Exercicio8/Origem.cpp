/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Jogos Digitais - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 12/05/2023
 *
 */

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <optional>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry(const struct Obj& obj);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 720, HEIGHT = 720;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
//...pode ter mais linhas de código aqui!
"gl_Position = model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = "#version 330\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

bool rotateX=false, rotateY=false, rotateZ=false;

struct Obj {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::u32vec3> triangle_indices;
};

auto parse_obj(const std::string& filename) -> std::optional<Obj>
{
    auto file = std::ifstream(filename);
    if (!file.good()) {
        cerr << "Error opening OBJ file '" << filename
             << "', error: " << std::strerror(errno) << endl;
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
            obj.vertices.push_back(v);
        }
        else if (code == "vn") {
            glm::vec3 vn;
            ss >> vn.x; ss >> vn.y; ss >> vn.z;
            obj.normals.push_back(vn);
        }
        else if (code == "vt") {
            glm::vec2 vt;
            ss >> vt.x; ss >> vt.y;
            obj.texcoords.push_back(vt);
        }
        else if (code == "f") {
            uint32_t _; // dummy var
            glm::u32vec3 fv; // get only vertex indices for now
            ss >> fv[0]; ss.get() /*slash*/; ss >> _; ss.get() /*slash*/; ss >> _;
            ss >> fv[1]; ss.get() /*slash*/; ss >> _; ss.get() /*slash*/; ss >> _;
            ss >> fv[2]; ss.get() /*slash*/; ss >> _; ss.get() /*slash*/; ss >> _;
            fv -= glm::u32vec3(1); /* index is offset by 1 */
            obj.triangle_indices.push_back(fv);
        }
    }

    printf("Parsed OBJ file '%s' with %zu vertices, %zu normals, %zu texcoords, %zu triangles\n",
           filename.c_str(), obj.vertices.size(), obj.normals.size(), obj.texcoords.size(), obj.triangle_indices.size());

    return obj;
}

// Função MAIN
int main()
{
    // Inicialização da GLFW
    glfwInit();

    //Muita atenção aqui: alguns ambientes não aceitam essas configurações
    //Você deve adaptar para a versão do OpenGL suportada por sua placa
    //Sugestão: comente essas linhas de código para desobrir a versão e
    //depois atualize (por exemplo: 4.5 com 4 e 5)
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Essencial para computadores da Apple
//#ifdef __APPLE__
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif

    // Criação da janela GLFW
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Natanael!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);

    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;

    }

    // Obtendo as informações de versão
    const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
    const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Carrega o OBJ
    auto obj = parse_obj("../../3D_Models/Suzanne/suzanneTriLowPoly.obj");
    //auto obj = parse_obj("../../3D_Models/Cube/cube.obj");
    if (!obj) {
        cerr << "Failed to load 3D Model" << endl;
        return 1;
    }

    // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);


    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Gerando um buffer simples, com a geometria de um triângulo
    GLuint VAO = setupGeometry(*obj);


    glUseProgram(shaderID);

    glm::mat4 model = glm::mat4(1); //matriz identidade;
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    //
    model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, false, glm::value_ptr(model));

    glEnable(GL_DEPTH_TEST);


    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
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
        glBindVertexArray(VAO);
        glVertexAttrib3f(1, 1.f, 0.f, 0.f); // default color for attribute color
        glDrawElements(GL_TRIANGLES, obj->triangle_indices.size() * 3, GL_UNSIGNED_INT, 0);

        // Draw the LINES
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBindVertexArray(VAO);
        glVertexAttrib3f(1, 0.f, 0.f, 0.f); // default color for attribute color
        glDrawElements(GL_TRIANGLES, obj->triangle_indices.size() * 3, GL_UNSIGNED_INT, 0);

        //glDrawArrays(GL_TRIANGLES, 0, obj->vertices.size());

        // Chamada de desenho - drawcall
        // CONTORNO - GL_LINE_LOOP

        // glDrawArrays(GL_POINTS, 0, 18);
        // glBindVertexArray(0);

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }
    // Pede pra OpenGL desalocar os buffers
    glDeleteVertexArrays(1, &VAO);
    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
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

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compilação (exibição via log no terminal)
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
    // Checando erros de compilação (exibição via log no terminal)
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

    //Geração do identificador do VBO
    glGenBuffers(1, &VBO);

    //Geração do identificador do EBO
    glGenBuffers(1, &EBO);

    //Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //Envia os dados do array de floats para o buffer da OpenGl
    size_t vertex_stride_size = sizeof(decltype(obj.vertices)::value_type);
    size_t vertex_total_size = obj.vertices.size() * vertex_stride_size;
    glBufferData(GL_ARRAY_BUFFER, vertex_total_size, obj.vertices.data(), GL_STATIC_DRAW);

    //Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);

    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);

    //Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
    // Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
    // Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
    // Tipo do dado
    // Se está normalizado (entre zero e um)
    // Tamanho em bytes
    // Deslocamento a partir do byte zero

    //Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride_size, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    //Atributo cor (r, g, b)
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    //glEnableVertexAttribArray(1);

    // Default value for color attribute
    glVertexAttrib3f(1, 1.f, 0.f, 0.f);
    glDisableVertexAttribArray(1);

    // Carrega os indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    size_t index_stride_size = sizeof(decltype(obj.triangle_indices)::value_type);
    size_t index_total_size = obj.triangle_indices.size() * index_stride_size;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_total_size, obj.triangle_indices.data(), GL_STATIC_DRAW);


    // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
    // atualmente vinculado - para que depois possamos desvincular com segurança
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}


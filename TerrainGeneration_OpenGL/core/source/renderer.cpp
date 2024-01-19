#include <iostream>

#include "renderer.h"
#include "app.h"
#include "terrain.h"
#include "shader.h"
#include "camera.h"
#include "texture.h"
#include "model.h"
#include "stb_image.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


float lastX = (float)1920 / 2.0;
float lastY = (float)1080 / 2.0;

std::vector<std::string> faces
{
   "C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/skybox/right.jpg",
   "C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/skybox/left.jpg",
   "C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/skybox/top.jpg",
   "C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/skybox/bottom.jpg",
   "C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/skybox/front.jpg",
   "C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/skybox/back.jpg"
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
unsigned int loadTexture(char const* path);

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


Renderer::Renderer()
    : deltaTime(0.0f),lastFrame(0.0f),
    app(App::GetInstance()), camera(Camera::GetInstance())
{
    skybox = new Skybox();
}

Renderer::~Renderer()
{
}


Renderer* Renderer::GetInstance()
{
    static Renderer* instance = new Renderer();
    return instance;
}

void Renderer::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(app->GetWindowWidth(), app->GetWindowHeight(), "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
}

void Renderer::RenderMap()
{
    map->GenerateVertexData(0.4f);
    map->GenerateIndexData();

    //=====================================Load map VAO VBO EBO===================================
    glGenVertexArrays(1, &mapVAO);
    glGenBuffers(1, &mapVBO);
    glGenBuffers(1, &mapEBO);

    glBindVertexArray(mapVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, map->vertices.size() * sizeof(t_Vertex), map->vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mapEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, map->indices.size() * sizeof(int), map->indices.data(), GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(t_Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::RenderWindow()
{
    //=====================================Load Skybox VAO VBO EBO================================
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox->skyboxVertices), &skybox->skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    unsigned int dep = 0, texture1 = 0, texture2 = 0, texture3 = 0;

    Texture* mapTex = new Texture(dep);
    Texture* T1 = new Texture(texture1);
    Texture* T2 = new Texture(texture2);

    mapTex->LoadTexture("assets/heightmap.png");
    T1->LoadTexture("assets/green.jpg");
    T2->LoadTexture("assets/brown.jpg");

    Shader shader("assets/shaders/v_shader.vs", "assets/shaders/f_shader.fs");
    Shader skyboxShader("assets/shaders/skybox.vs", "assets/shaders/skybox.fs");
    unsigned int cubemapTexture = skybox->LoadSkybox(faces);

    shader.Use();
    shader.SetInt("dep", 0);
    shader.SetInt("tex1", 1);
    shader.SetInt("tex2", 2);
    shader.SetInt("blendMode", 1);
    shader.SetFloat("textureSeperationHeight", 2.0f);
    shader.SetFloat("heightMult", 4.0f);

    skyboxShader.Use();
    skyboxShader.SetInt("skybox", 0);

    stbi_set_flip_vertically_on_load(true);
    Shader bagShader("assets/shaders/v_loadModel.vs", "assets/shaders/f_loadModel.fs");
    Model bagModel("C:/Users/m.leguevacques/Documents/Projects/TerrainGeneration_OpenGL/TerrainGeneration_OpenGL/assets/backpack/backpack.obj");

    InitImGui(window);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // mouse callback
        if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            app->fpsView = !app->fpsView;
            double lastMouseX, lastMouseY;

            if (app->fpsView) {
                glfwSetCursorPosCallback(window, mouse_callback);
                glfwSetScrollCallback(window, scroll_callback);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetCursorPosCallback(window, ImGui_ImplGlfw_CursorPosCallback);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapTex->GetTextureName());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, T1->GetTextureName());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, T2->GetTextureName());

        // Set matrices and pass them to the shader
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera->GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)1920 / (float)1080, 0.1f, 100.0f);
        shader.SetMat4("model", model);
        shader.SetMat4("view", view);
        shader.SetMat4("projection", projection);

        // Render map
        glBindVertexArray(mapVAO);
        glDrawElements(GL_TRIANGLES, (map->rows - 1) * (map->cols - 1) * 6, GL_UNSIGNED_INT, 0);

        // Draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.Use();
        view = glm::mat4(glm::mat3(camera->GetViewMatrix()));

        skyboxShader.SetMat4("view", view);
        skyboxShader.SetMat4("projection", projection);

        // Skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        bagShader.Use();

        // view/projection transformations
        projection = glm::perspective(glm::radians(camera->Zoom), (float)1920 / (float)1080, 0.1f, 100.0f);
        view = camera->GetViewMatrix();
        bagShader.SetMat4("projection", projection);
        bagShader.SetMat4("view", view);

        // render the loaded model
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));	// it's a bit too big for our scene, so scale it down
        bagShader.SetMat4("model", model);
        bagModel.Draw(bagShader);

        shader.Use();
        RenderImGui(&shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &mapVAO);
    glDeleteBuffers(1, &mapVBO);
    glDeleteBuffers(1, &mapEBO);
}

void Renderer::CloseWindow()
{
    CleanImGui();
    glfwTerminate();
}

#pragma region Renderer ImGui

void Renderer::InitImGui(GLFWwindow* _window)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void Renderer::RenderImGui(Shader* _shader)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Hello, world!");

    ImGui::SliderFloat("Depth", &depth, -100.0f, 100.0f, "%.f", 0);
    ImGui::SliderFloat("Vertical", &upPos, -100.0f, 100.0f, "%.f", 0);
    ImGui::SliderFloat("Horizontal", &horizontalPos, -100.0f, 100.0f, "%.f", 0);
    if (ImGui::Button("Load map")) {
        glDeleteVertexArrays(1, &mapVAO);
        glDeleteBuffers(1, &mapVBO);
        glDeleteBuffers(1, &mapEBO);
        RenderMap();
    }
    ImGui::Text("Press 'c' to switch camera/cursor mode");
    if (ImGui::Checkbox("blend mode", &app->blendMode)) {
        if (app->blendMode) {
            _shader->SetInt("blendMode", 0);
        }
        else {
            _shader->SetInt("blendMode", 1);
        }
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::CleanImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

#pragma endregion Renderer ImGui

void Renderer::processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera->ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera->ProcessKeyboard(UP, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }

    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    Renderer* renderer = Renderer::GetInstance();
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    renderer->camera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Renderer* renderer = Renderer::GetInstance();
    renderer->camera->ProcessMouseScroll(static_cast<float>(yoffset));
}
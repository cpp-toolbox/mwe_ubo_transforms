#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "graphics/batcher/generated/batcher.hpp"
#include "graphics/shader_cache/shader_cache.hpp"
#include "graphics/vertex_geometry/vertex_geometry.hpp"
#include "graphics/window/window.hpp"

GLuint uboModelMatrices;

void update_matrices(double time_since_start_of_program_sec, glm::mat4 *model_matrices) {
    int numTriangles = 100;
    float scale_x = 3 * 0.1f;
    float scale_y = 3 * 0.1f;

    for (int i = 0; i < numTriangles; ++i) {
        float angle = 2.0f * M_PI * i / numTriangles;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.75f * cos(angle + time_since_start_of_program_sec * scale_x),
                                                0.75f * sin(angle + time_since_start_of_program_sec * scale_y), 0.0f));
        model = glm::scale(model, glm::vec3(0.3f));

        float rotation_angle = time_since_start_of_program_sec; // Rotation speed proportional to time
        glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0f), rotation_angle, glm::vec3(0.0f, 1.0f, 1.0f));

        model = rotation_matrix * model;

        model_matrices[i] = model;
    }
}

int main() {

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    std::vector<spdlog::sink_ptr> sinks = {console_sink};

    unsigned int screen_width = 800;
    unsigned int screen_height = 800;

    GLFWwindow *window =
        initialize_glfw_glad_and_return_window(screen_width, screen_height, "ubo transforms", false, false, false);

    std::vector<ShaderType> requested_shaders = {ShaderType::CWL_V_TRANSFORMATION_USING_UBOS_WITH_SOLID_COLOR};
    ShaderCache shader_cache(requested_shaders, sinks);
    Batcher batcher(shader_cache);

    std::vector<glm::vec3> base_triangle = {{0.0f, 0.1f, 0.0f}, {-0.1f, -0.1f, 0.0f}, {0.1f, -0.1f, 0.0f}};

    std::vector<glm::vec3> triangle_vertices;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> ltw_matrix_indices;

    const int num_triangles = 100;

    for (int i = 0; i < num_triangles; ++i) {
        // Add vertices for this triangle (no transformation)
        triangle_vertices.insert(triangle_vertices.end(), base_triangle.begin(), base_triangle.end());

        // Add indices for this triangle
        unsigned int base_index = i * 3; // Each triangle has 3 vertices
        indices.push_back(base_index);
        indices.push_back(base_index + 1);
        indices.push_back(base_index + 2);

        // Add ltw_matrix_indices for this triangle
        ltw_matrix_indices.push_back(i);
        ltw_matrix_indices.push_back(i);
        ltw_matrix_indices.push_back(i);
    }

    glm::mat4 modelMatrices[100];
    update_matrices(0.0, modelMatrices);

    glGenBuffers(1, &uboModelMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboModelMatrices);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(modelMatrices), modelMatrices, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboModelMatrices);

    shader_cache.use_shader_program(ShaderType::CWL_V_TRANSFORMATION_USING_UBOS_WITH_SOLID_COLOR);

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_USING_UBOS_WITH_SOLID_COLOR,
                             ShaderUniformVariable::CAMERA_TO_CLIP, projection);
    shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_USING_UBOS_WITH_SOLID_COLOR,
                             ShaderUniformVariable::WORLD_TO_CAMERA, view);
    shader_cache.set_uniform(ShaderType::CWL_V_TRANSFORMATION_USING_UBOS_WITH_SOLID_COLOR,
                             ShaderUniformVariable::RGBA_COLOR, glm::vec4(0, 0, 1, 1));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double currentTime = glfwGetTime();
        update_matrices(currentTime, modelMatrices);

        glBindBuffer(GL_UNIFORM_BUFFER, uboModelMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(modelMatrices), modelMatrices);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        batcher.cwl_v_transformation_using_ubos_with_solid_color_shader_batcher.queue_draw(indices, triangle_vertices,
                                                                                           ltw_matrix_indices);
        batcher.cwl_v_transformation_using_ubos_with_solid_color_shader_batcher.draw_everything();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

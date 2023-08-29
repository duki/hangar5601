#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action,
		  int mods);

// settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

struct ProgramState {
	glm::vec3 clearColor = glm::vec3(0);
	bool ImGuiEnabled = false;
	Camera camera;
	bool CameraMouseMovementUpdateEnabled = true;
	glm::vec3 backpackPosition = glm::vec3(0.0f);
	float backpackScale = 1000.0f;
	PointLight pointLight;
	ProgramState() : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

	void SaveToFile(std::string filename);

	void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename)
{
	std::ofstream out(filename);
	out << clearColor.r << '\n'
	    << clearColor.g << '\n'
	    << clearColor.b << '\n'
	    << ImGuiEnabled << '\n'
	    << camera.Position.x << '\n'
	    << camera.Position.y << '\n'
	    << camera.Position.z << '\n'
	    << camera.Front.x << '\n'
	    << camera.Front.y << '\n'
	    << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename)
{
	std::ifstream in(filename);
	if (in) {
		in >> clearColor.r >> clearColor.g >> clearColor.b >>
		    ImGuiEnabled >> camera.Position.x >> camera.Position.y >>
		    camera.Position.z >> camera.Front.x >> camera.Front.y >>
		    camera.Front.z;
	}
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

GLfloat planeVertices[] = {
    -1000.0f, 0, -1000.0f, 0.0f, 0.0f, -1000.0f, 0, 1000.0f,  0.0f, 1.0f,
    1000.0f,  0, 1000.0f,  1.0f, 1.0f, 1000.0f,	 0, -1000.0f, 1.0f, 0.0f};

float skyboxVertices[] = {
    //   Coordinates
    -1.0f, -1.0f, 1.0f,	  // 1
    1.0f,  -1.0f, 1.0f,	  // 2
    1.0f,  -1.0f, -1.0f,  // 3
    -1.0f, -1.0f, -1.0f,  // 4
    -1.0f, 1.0f,  1.0f,	  // 5
    1.0f,  1.0f,  1.0f,	  // 6
    1.0f,  1.0f,  -1.0f,  // 7
    -1.0f, 1.0f,  -1.0f	  // 8
};

unsigned int skyboxIndices[] = {
    // R
    1, 2, 5, 5, 2, 6,
    // L
    0, 4, 7, 7, 3, 0,
    // T
    4, 5, 6, 6, 7, 4,
    // B
    0, 3, 2, 2, 1, 0,
    // BA
    0, 1, 5, 5, 4, 0,
    // F
    3, 7, 6, 6, 2, 3};

auto main() -> int
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
					      "LearnOpenGL", nullptr, nullptr);
	if (window == nullptr) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader(
		reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	stbi_set_flip_vertically_on_load(true);

	programState = new ProgramState;
	programState->LoadFromFile("resources/program_state.txt");
	if (programState->ImGuiEnabled) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	// Init Imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	// configure global opengl state

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// build and compile shaders
	// -------------------------
	Shader planeShader("resources/shaders/grass.vs",
			   "resources/shaders/grass.fs");
	Shader stationShader("resources/shaders/station.vs",
			     "resources/shaders/station.fs");
	Shader outlineShader("resources/shaders/outlining.vs",
			     "resources/shaders/outlining.fs");
	Shader skyboxShader("resources/shaders/skybox.vs",
			    "resources/shaders/skybox.fs");
	Shader treeShader("resources/shaders/trees.vs",
			  "resources/shaders/trees.fs");
	// load models
	// -----------
	// Model
	// ourModel("resources/objects/space_station/Space\ Station\ Scene.obj");
	Model ourModel("resources/objects/grass/grass.obj");
	Model stationModel(
	    "resources/objects/space_station/Space\ Station\ Scene.obj");
	Model freighterModel("resources/objects/freighter/freighter.obj");
	Model treeModel("resources/objects/trees/trees9.obj");

	freighterModel.SetShaderTextureNamePrefix("material.");
	ourModel.SetShaderTextureNamePrefix("material.");
	stationModel.SetShaderTextureNamePrefix("material.");
	treeModel.SetShaderTextureNamePrefix("material.");

	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);

	PointLight &pointLight = programState->pointLight;
	pointLight.position = glm::vec3(0.0f, 0.0, 0.0);
	pointLight.ambient = glm::vec3(0.4, 0.4, 0.4);
	pointLight.diffuse = glm::vec3(2000.0, 2000.0, 2000.0);
	pointLight.specular = glm::vec3(50.0, 50.0, 50.0);

	// pointLight.constant = 0.03f;
	// pointLight.linear = 0.07f;
	// pointLight.quadratic = 0.00003f;

	pointLight.constant = 0.505f;
	pointLight.linear = 0.575f;
	pointLight.quadratic = 0.0000003f;

	// draw in wireframe
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glfwSwapInterval(0);
	unsigned int fpsCounter = 0;
	// render loop
	// -----------

	unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glGenBuffers(1, &skyboxEBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
		     GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices),
		     &skyboxIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
			      (void *)nullptr);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	std::string facesCubemap[6] = {
	    "resources/textures/right.png", "resources/textures/left.png",
	    "resources/textures/top.png",   "resources/textures/bottom.png",
	    "resources/textures/front.png", "resources/textures/back.png",
	};

	unsigned int cubemapTexture;
	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
			GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
			GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
			GL_CLAMP_TO_EDGE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	for (unsigned int i = 0; i < 6; i++) {
		int width, height, nrChannels;
		unsigned char *data = stbi_load(facesCubemap[i].c_str(), &width,
						&height, &nrChannels, 0);
		if (data) {
			stbi_set_flip_vertically_on_load(false);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
				     GL_RGBA, width, height, 0, GL_RGBA,
				     GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		} else {
			std::cout
			    << "Failed to load texture: " << facesCubemap[i]
			    << std::endl;
			stbi_image_free(data);
		}
	}

	// texture loading
	int widthImg, heightImg, numColCh;
	unsigned char *bytes = stbi_load("resources/textures/grass.jpg",
					 &widthImg, &heightImg, &numColCh, 0);

	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthImg, heightImg, 0, GL_RGB,
		     GL_UNSIGNED_BYTE, bytes);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(bytes);
	glBindTexture(GL_TEXTURE_2D, 0);

	while (!glfwWindowShouldClose(window)) {
		// per-frame time logic
		// --------------------
		float progTime = glfwGetTime();
		deltaTime = progTime - lastFrame;
		// lastFrame = progTime;
		fpsCounter++;

		if (deltaTime >= 1.0 / 30.0) {
			std::string fpsString = std::to_string(
			    round((1.0 / deltaTime) * fpsCounter));
			std::string milString = std::to_string(
			    round((deltaTime / fpsCounter) * 1000));
			std::string newTitle = "hangar5601 [" + fpsString +
					       "]fps / [" + milString + "]ms";
			glfwSetWindowTitle(window, newTitle.c_str());
			lastFrame = progTime;
			fpsCounter = 0;
		}
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(programState->clearColor.r,
			     programState->clearColor.g,
			     programState->clearColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);

		// grassDrawing
		planeShader.use();
		pointLight.position = glm::vec3(300.0 * cos(progTime),
						-abs(cos(progTime)) * 200.0f,
						500.0 * sin(progTime));
		planeShader.setVec3("pointLight.position", pointLight.position);
		planeShader.setVec3("pointLight.ambient", pointLight.ambient);
		planeShader.setVec3("pointLight.diffuse", pointLight.diffuse);
		planeShader.setVec3("pointLight.specular", pointLight.specular);
		planeShader.setFloat("pointLight.constant",
				     pointLight.constant);
		planeShader.setFloat("pointLight.linear", pointLight.linear);
		planeShader.setFloat("pointLight.quadratic",
				     pointLight.quadratic);
		planeShader.setVec3("viewPosition",
				    programState->camera.Position);
		planeShader.setFloat("material.shininess", 32.0f);

		// stationDrawing
		stationShader.use();
		stationShader.setVec3("pointLight.position",
				      pointLight.position);
		stationShader.setVec3("pointLight.ambient", pointLight.ambient);
		stationShader.setVec3("pointLight.diffuse", pointLight.diffuse);
		stationShader.setVec3("pointLight.specular",
				      pointLight.specular);
		stationShader.setFloat("pointLight.constant",
				       pointLight.constant);
		stationShader.setFloat("pointLight.linear", pointLight.linear);
		stationShader.setFloat("pointLight.quadratic",
				       pointLight.quadratic);
		stationShader.setVec3("viewPosition",
				      programState->camera.Position);
		stationShader.setFloat("material.shininess", 12.0f);
		// view/projection transformations
		glm::mat4 projection =
		    glm::perspective(glm::radians(programState->camera.Zoom),
				     static_cast<float>(SCR_WIDTH) /
					 static_cast<float>(SCR_HEIGHT),
				     0.1f, 100000.0f);
		glm::mat4 view = programState->camera.GetViewMatrix();

		planeShader.use();
		planeShader.setMat4("projection", projection);
		planeShader.setMat4("view", view);
		stationShader.use();
		stationShader.setMat4("projection", projection);
		stationShader.setMat4("view", view);
		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(
		    model,
		    programState
			->backpackPosition);  // translate it down so it's at
					      // the center of the scene
		model = glm::scale(
		    model,
		    glm::vec3(
			programState
			    ->backpackScale));	// it's a bit too big for our
						// scene, so scale it down

		planeShader.use();
		planeShader.setMat4("model", model);

		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);

		float rotationAngle =
		    glm::radians(sin(progTime) * (12) * cos(progTime));
		glm::vec3 rotationAxis = glm::vec3(0, 1, 0);
		glm::mat4 freighterRot = glm::mat4(1.0);

		freighterRot = glm::translate(
		    freighterRot,
		    programState
			->backpackPosition);  // translate it down so it's at
					      // the center of the scene
		freighterRot = glm::scale(
		    freighterRot, glm::vec3(programState->backpackScale));
		freighterRot =
		    glm::scale(freighterRot,
			       glm::vec3(programState->backpackScale / 100000));
		freighterRot =
		    glm::rotate(freighterRot, rotationAngle, rotationAxis);
		freighterRot = glm::translate(
		    freighterRot, glm::vec3(cos(progTime / 4) * 50.0, 10.0f,
					    sin(progTime / 4) * 50.0));
		// starting to use the outline shader
		outlineShader.use();
		outlineShader.setMat4("projection", projection);
		outlineShader.setMat4("view", view);
		outlineShader.setMat4("model", freighterRot);
		outlineShader.setFloat("outlining", 1.0);
		freighterModel.Draw(outlineShader);

		planeShader.use();
		ourModel.Draw(planeShader);

		planeShader.setMat4("model", freighterRot);
		freighterModel.Draw(planeShader);

		// enabling blending
		glEnable(GL_BLEND);
		treeShader.use();
		treeShader.setVec3("pointLight.position", pointLight.position);
		treeShader.setVec3("pointLight.ambient", pointLight.ambient);
		treeShader.setVec3("pointLight.diffuse", pointLight.diffuse);
		treeShader.setVec3("pointLight.specular", pointLight.specular);
		treeShader.setFloat("pointLight.constant", pointLight.constant);
		treeShader.setFloat("pointLight.linear", pointLight.linear);
		treeShader.setFloat("pointLight.quadratic",
				    pointLight.quadratic);
		treeShader.setVec3("viewPosition",
				   programState->camera.Position);
		treeShader.setFloat("material.shininess", 32.0f);

		treeShader.setMat4("projection", projection);
		treeShader.setMat4("view", view);
		glm::mat4 treeRot = glm::mat4(1.0f);
		treeRot = glm::scale(
		    treeRot, glm::vec3(programState->backpackScale / 100));
		// treeRot = glm::rotate(treeRot, rotationAngle, rotationAxis);
		treeRot =
		    glm::translate(treeRot, glm::vec3(20.0f, 0.0f, 80.2f));
		treeShader.setMat4("model", treeRot);

		treeModel.Draw(treeShader);
		glDisable(GL_BLEND);
		// disabling blending

		// rotation and translation for freigther
		// START OF STENCIL SHADER
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glDisable(GL_DEPTH_TEST);

		glStencilMask(0xff);
		glStencilFunc(GL_ALWAYS, 0, 0xff);
		glEnable(GL_DEPTH_TEST);
		// END OF STENCIL SHADER

		stationShader.use();
		stationShader.setMat4("projection", projection);
		stationShader.setMat4("view", view);
		stationShader.setMat4(
		    "model",
		    glm::scale(model, glm::vec3(programState->backpackScale /
						1000000)));
		stationModel.Draw(stationShader);

		// SKYBOX
		glDepthFunc(GL_LEQUAL);
		skyboxShader.use();
		glm::mat4 skyboxView = glm::mat4(1.0f);
		glm::mat4 skyProjection = glm::mat4(1.0f);

		skyboxView = glm::mat4(glm::mat3(glm::lookAt(
		    programState->camera.Position,
		    programState->camera.Position + programState->camera.Front,
		    programState->camera.Up)));
		skyProjection = glm::perspective(
		    glm::radians(45.0f),
		    static_cast<float>(SCR_WIDTH) / SCR_HEIGHT, 0.1f, 1000.0f);
		glUniformMatrix4fv(
		    glGetUniformLocation(skyboxShader.ID, "skyView"), 1,
		    GL_FALSE, glm::value_ptr(skyboxView));
		glUniformMatrix4fv(
		    glGetUniformLocation(skyboxShader.ID, "skyProjection"), 1,
		    GL_FALSE, glm::value_ptr(skyProjection));

		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		// END OF SKYBOX

		glDepthFunc(GL_LESS);

		if (programState->ImGuiEnabled) {
			DrawImGui(programState);
		}

		// glfw: swap buffers and poll IO events (keys pressed/released,
		// mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glDeleteTextures(1, &texture);

	programState->SaveToFile("resources/program_state.txt");
	delete programState;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this
// frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		programState->camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		programState->camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		programState->camera.ProcessKeyboard(RIGHT, deltaTime);
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that
	// width and height will be significantly larger than specified on
	// retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset =
	    lastY - ypos;  // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	if (programState->CameraMouseMovementUpdateEnabled) {
		programState->camera.ProcessMouseMovement(xoffset, yoffset);
	}
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	{
		static float f = 0.0f;
		ImGui::Begin("Hello window");
		ImGui::Text("Hello text");
		ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
		ImGui::ColorEdit3(
		    "Background color",
		    reinterpret_cast<float *>(&programState->clearColor));
		ImGui::DragFloat3(
		    "Backpack position",
		    reinterpret_cast<float *>(&programState->backpackPosition));
		ImGui::DragFloat("Backpack scale", &programState->backpackScale,
				 0.55, 0.1, 1000.0);

		ImGui::DragFloat("pointLight.constant",
				 &programState->pointLight.constant, 0.05, 0.0,
				 1.0);
		ImGui::DragFloat("pointLight.linear",
				 &programState->pointLight.linear, 0.05, 0.0,
				 1.0);
		ImGui::DragFloat("pointLight.quadratic",
				 &programState->pointLight.quadratic, 0.05, 0.0,
				 1.0);
		ImGui::End();
	}

	{
		ImGui::Begin("Camera info");
		const Camera &c = programState->camera;
		ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x,
			    c.Position.y, c.Position.z);
		ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
		ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y,
			    c.Front.z);
		ImGui::Checkbox(
		    "Camera mouse update",
		    &programState->CameraMouseMovementUpdateEnabled);
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action,
		  int mods)
{
	if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
		programState->ImGuiEnabled = !programState->ImGuiEnabled;
		if (programState->ImGuiEnabled) {
			programState->CameraMouseMovementUpdateEnabled = false;
			glfwSetInputMode(window, GLFW_CURSOR,
					 GLFW_CURSOR_NORMAL);
		} else {
			glfwSetInputMode(window, GLFW_CURSOR,
					 GLFW_CURSOR_DISABLED);
		}
	}
}

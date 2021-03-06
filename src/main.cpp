// Core libraries
#include <iostream>
#include <cmath>

// Third party libraries
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Local headers
#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"
#include "Shape.h"
#include "Camera.h"
#include "GrlModel.hpp"

using namespace std;
using namespace glm;

double get_last_elapsed_time() {
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
}



class Application : public EventCallbacks {
public:
	struct PlaneControlAngles {
		float stabilator_L = 0;
		float stabilator_R = 0;
		float rudder_L = 0;
		float rudder_R = 0;
		float aileron_L = 0;
		float aileron_R = 0;
	};
	WindowManager *windowManager = nullptr;
    Camera *camera = nullptr;
	PlaneControlAngles f18Controls;
	std::shared_ptr<GrlModel> f18Model;
    std::shared_ptr<Shape> shape;
	std::shared_ptr<Program> phongShader;
    
    double gametime = 0;
    bool wireframeEnabled = false;
    bool mousePressed = false;
    bool mouseCaptured = false;
    glm::vec2 mouseMoveOrigin = glm::vec2(0);
    glm::vec3 mouseMoveInitialCameraRot;

    Application() {
        camera = new Camera();
    }
    
    ~Application() {
        delete camera;
    }

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		// Movement
        if (key == GLFW_KEY_W && action != GLFW_REPEAT) camera->vel.z = (action == GLFW_PRESS) * -0.2f;
        if (key == GLFW_KEY_S && action != GLFW_REPEAT) camera->vel.z = (action == GLFW_PRESS) * 0.2f;
        if (key == GLFW_KEY_A && action != GLFW_REPEAT) camera->vel.x = (action == GLFW_PRESS) * -0.2f;
        if (key == GLFW_KEY_D && action != GLFW_REPEAT) camera->vel.x = (action == GLFW_PRESS) * 0.2f;
        // Rotation
        if (key == GLFW_KEY_I && action != GLFW_REPEAT) camera->rotVel.x = (action == GLFW_PRESS) * 0.02f;
        if (key == GLFW_KEY_K && action != GLFW_REPEAT) camera->rotVel.x = (action == GLFW_PRESS) * -0.02f;
        if (key == GLFW_KEY_J && action != GLFW_REPEAT) camera->rotVel.y = (action == GLFW_PRESS) * 0.02f;
        if (key == GLFW_KEY_L && action != GLFW_REPEAT) camera->rotVel.y = (action == GLFW_PRESS) * -0.02f;
        if (key == GLFW_KEY_U && action != GLFW_REPEAT) camera->rotVel.z = (action == GLFW_PRESS) * 0.02f;
        if (key == GLFW_KEY_O && action != GLFW_REPEAT) camera->rotVel.z = (action == GLFW_PRESS) * -0.02f;
        // Polygon mode (wireframe vs solid)
        if (key == GLFW_KEY_P && action == GLFW_PRESS) {
            wireframeEnabled = !wireframeEnabled;
            glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);
        }
        // Hide cursor (allows unlimited scrolling)
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            mouseCaptured = !mouseCaptured;
            glfwSetInputMode(window, GLFW_CURSOR, mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            resetMouseMoveInitialValues(window);
        }

		//raise/lower stabilators
		if (key == GLFW_KEY_Z && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
			f18Controls.stabilator_L += 0.02f;
			f18Controls.stabilator_R += 0.02f;
		}
		if (key == GLFW_KEY_X && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
			f18Controls.stabilator_L -= 0.02f;
			f18Controls.stabilator_R -= 0.02f;
		}

		//raise/lower ailerons
		if (key == GLFW_KEY_C && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
			f18Controls.aileron_L += 0.02f;
			f18Controls.aileron_R += 0.02f;
		}
		if (key == GLFW_KEY_V && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
			f18Controls.aileron_L -= 0.02f;
			f18Controls.aileron_R -= 0.02f;
		}

		//raise/lower rudders
		if (key == GLFW_KEY_B && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
			f18Controls.rudder_L += 0.02f;
			f18Controls.rudder_R += 0.02f;
		}
		if (key == GLFW_KEY_N && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
			f18Controls.rudder_L -= 0.02f;
			f18Controls.rudder_R -= 0.02f;
		}

	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods) {
        mousePressed = (action != GLFW_RELEASE);
        if (action == GLFW_PRESS) {
            resetMouseMoveInitialValues(window);
        }
    }
    
    void mouseMoveCallback(GLFWwindow *window, double xpos, double ypos) {
        if (mousePressed || mouseCaptured) {
            float yAngle = (xpos - mouseMoveOrigin.x) / windowManager->getWidth() * 3.14159f;
            float xAngle = (ypos - mouseMoveOrigin.y) / windowManager->getHeight() * 3.14159f;
            camera->setRotation(mouseMoveInitialCameraRot + glm::vec3(-xAngle, -yAngle, 0));
        }
    }

	void resizeCallback(GLFWwindow *window, int in_width, int in_height) { }
    
    // Reset mouse move initial position and rotation
    void resetMouseMoveInitialValues(GLFWwindow *window) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        mouseMoveOrigin = glm::vec2(mouseX, mouseY);
        mouseMoveInitialCameraRot = camera->rot;
    }

	void initGeom(const std::string& resourceDirectory) {
		f18Model = make_shared<GrlModel>();
		f18Model->loadSubModels(resourceDirectory + "/f18.grl");
		f18Model->init();
	}
	
	void init(const std::string& resourceDirectory) {
		GLSL::checkVersion();

		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
        
		// Initialize the GLSL programs
        phongShader = std::make_shared<Program>();
        phongShader->setShaderNames(resourceDirectory + "/normal.vert", resourceDirectory + "/normal.frag");
        phongShader->init();
	}
    
    glm::mat4 getPerspectiveMatrix() {
        float fov = 3.14159f / 4.0f;
        float aspect = windowManager->getAspect();
        return glm::perspective(fov, aspect, 0.01f, 10000.0f);
    }

	void drawF18Model(mat4 M = mat4(1)) {
		f18Model->drawSubModel(phongShader, "ElevatorL01", M,
			glm::rotate(mat4(1), f18Controls.stabilator_L, vec3(1, 0, 0))
		);
		f18Model->drawSubModel(phongShader, "ElevatorR01", M,
			glm::rotate(mat4(1), f18Controls.stabilator_R, vec3(1, 0, 0))
		);
		f18Model->drawSubModel(phongShader, "RudderL01", M,
			glm::rotate(mat4(1), f18Controls.rudder_L, vec3(0, 1, 0))
		);
		f18Model->drawSubModel(phongShader, "RudderR01", M,
			glm::rotate(mat4(1), f18Controls.rudder_R, vec3(0, 1, 0))
		);
		f18Model->drawSubModel(phongShader, "AileronL01", M,
			glm::rotate(mat4(1), f18Controls.aileron_L, vec3(1, 0, 0))
		);
		f18Model->drawSubModel(phongShader, "AileronR01", M,
			glm::rotate(mat4(1), f18Controls.aileron_R, vec3(1, 0, 0))
		);
		f18Model->drawSubModel(phongShader, "VoletR01", M);
		f18Model->drawSubModel(phongShader, "Glass_Canopy", M);
		f18Model->drawSubModel(phongShader, "Pilot", M);
		f18Model->drawSubModel(phongShader, "Glass", M);
		f18Model->drawSubModel(phongShader, "VoletL01", M);
		f18Model->drawSubModel(phongShader, "LOD0", M);
		f18Model->drawSubModel(phongShader, "Hook", M);
		f18Model->drawSubModel(phongShader, "EngineR01", M);
		f18Model->drawSubModel(phongShader, "FlapL01", M);
		f18Model->drawSubModel(phongShader, "Eject_Seat", M);
		f18Model->drawSubModel(phongShader, "FlapR01", M);
		f18Model->drawSubModel(phongShader, "Glass_HUD", M);
		f18Model->drawSubModel(phongShader, "EngineL01", M);
		f18Model->drawSubModel(phongShader, "Canopy01", M);
	}

	void render() {
		double frametime = get_last_elapsed_time();
		gametime += frametime;

		// Clear framebuffer.
		glClearColor(0.3f, 0.7f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks.
		glm::mat4 V, M, P;
        P = getPerspectiveMatrix();
        V = camera->getViewMatrix();
        M = glm::mat4(1);
        
        /**************/
        /* DRAW SHAPE */
        /**************/
        M = glm::translate(M, glm::vec3(0, 0, -20));
		M = glm::rotate(M, 3.14f, vec3(1, 0, 0));
		M = glm::rotate(M, 3.14f, vec3(0, 1, 0));
		mat4 N = glm::inverse(glm::transpose(M));

		mat4 localR;

        phongShader->bind();
        phongShader->setMVP(&M[0][0], &V[0][0], &P[0][0]);
		phongShader->setMatrix("N", &N[0][0]);
		drawF18Model(M);
        phongShader->unbind();
	}
};

int main(int argc, char **argv) {
	std::string resourceDir = "../resources";
	if (argc >= 2) {
		resourceDir = argv[1];
	}

	Application *application = new Application();

    // Initialize window.
	WindowManager * windowManager = new WindowManager();
	windowManager->init(800, 600);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// Initialize scene.
	application->init(resourceDir);
	application->initGeom(resourceDir);
    
	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle())) {
        // Update camera position.
        application->camera->update();
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}

#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"
#include "Physics.h"

using namespace std;
using namespace glm;

// scene properties
namespace Objects {
    struct Properties {
        glm::vec3 size, pos;
    };

    const int numSpheres = 10;

    Properties
        spheres[numSpheres] = {
            { { 0.4, 0.4, 0.4 },{ -3, 8.25, 0 } },
        { { 0.4, 0.4, 0.4 },{ -2.6, 8.85, 0 } },
        { { 0.4, 0.4, 0.4 },{ -2.2, 8.55, 0 } },
        { { 0.4, 0.4, 0.4 },{ -1.8, 8.25, 0 } },
        { { 0.4, 0.4, 0.4 },{ -1.4, 8.85, 0 } },
        { { 0.4, 0.4, 0.4 },{ -1.0, 8.55, 0 } },
        { { 0.4, 0.4, 0.4 },{ -0.6, 8.85, 0 } },
        { { 0.4, 0.4, 0.4 },{ -0.2, 8.55, 0 } },
        { { 0.4, 0.4, 0.4 },{ 0.2, 8.25, 0 } },
        { { 0.4, 0.4, 0.4 },{ 3, 4.75, 0 } } };
}
                


namespace Spaceship {
    struct Properties {
        glm::vec3 size, pos;
    };

    Properties spaceship{ { 0.8, 0.5, 1.0  },{ 8, 0, 0 } };
}

Core::Shader_Loader shaderLoader;
GLuint programColor;
GLuint programTexture;
GLuint program_Sun;
GLuint programSkybox;

obj::Model planeModel, sphereModel, spaceshipModel,skyboxModel;
Core::RenderContext planeContext, sphereContext, spaceshipContext,skyboxContext;
GLuint objectTexture, groundTexture,spaceshipTexture,sunTexture, jowiszTexture,skyboxTexture;


glm::vec3 cameraPos = glm::vec3(0, 2, 10);
glm::vec3 cameraDir = vec3(0,0,-1);
glm::vec3 cameraSide;
float cameraAngle = 0;
glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 lightDir = glm::normalize(glm::vec3(0, -0.3, -0.5));
float lastX = 0, lastY = 0;
float Pitch = 0, Yaw = -90.0f;
float sensitive = 0.05;
bool startMouse = 1;
float distanceX = 0, distanceY = 0;


// Initalization of physical scene (PhysX)
Physics pxScene(0 /* gravity (m/s^2) */);

// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

// physical objects
PxMaterial      *material = nullptr;
PxRigidStatic   *bodyGround = nullptr;
PxRigidDynamic  *bodySpheres[Objects::numSpheres] = { nullptr, nullptr, nullptr },
                *bodySpaceship = nullptr;

// renderable objects (description of a single renderable instance)
struct Renderable {
    Core::RenderContext *context;
    glm::mat4 localTransform, physicsTransform;
    GLuint textureId;
};
Renderable rendGround, rendSpheres[10],rendSkybox;
vector<Renderable*> renderables, noshadow;

struct RenderableSpaceship {
    Core::RenderContext* context2;
    glm::mat4 localTransform2, physicsTransform2;
    GLuint textureId2;
};
RenderableSpaceship rendSpaceship;
vector<RenderableSpaceship*> renderablesSpaceship;

//SKYBOX

unsigned int cubemapTexture;

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};


void initRenderables()
{
    // load models
    planeModel = obj::loadModelFromFile("models/plane.obj");
   
    sphereModel = obj::loadModelFromFile("models/sphere.obj");
    spaceshipModel = obj::loadModelFromFile("models/starship.obj");
	skyboxModel = obj::loadModelFromFile("models/skybox.obj");


    planeContext.initFromOBJ(planeModel);
    
    sphereContext.initFromOBJ(sphereModel);
    spaceshipContext.initFromOBJ(spaceshipModel);
	skyboxContext.initFromOBJ(skyboxModel);


    // load textures
    groundTexture = Core::LoadTexture("textures/neptune.jpg");
    objectTexture = Core::LoadTexture("textures/spacetex.jpg");
	spaceshipTexture = Core::LoadTexture("textures/spaceship.jpg");
    sunTexture = Core::LoadTexture("textures/sun.jpg");
    jowiszTexture = Core::LoadTexture("textures/jowisz.jpg");
	skyboxTexture= Core::LoadTexture("textures/cubemap.jpg");
    //spaceshipTexture = Core::LoadTexture("textrues/sand.jpg");

    // This time we organize all the renderables in a list
    // of basic properties (model, transform, texture),
    // to unify their rendering and simplify their managament
    // in connection to the physics simulation

    // create ground
    //rendGround.context = &planeContext;
    //rendGround.textureId = groundTexture;
    //renderables.emplace_back(&rendGround);

	rendSkybox.context = &skyboxContext;
	rendSkybox.textureId = skyboxTexture;
	//rendSkybox.localTransform = scale(vec3(0.1f));
	noshadow.emplace_back(&rendSkybox);


    // create spheres
    for (int i = 0; i < Objects::numSpheres; i++) {
        rendSpheres[i].context = &sphereContext;
        rendSpheres[i].textureId = objectTexture;
        rendSpheres[i].localTransform = glm::scale(Objects::spheres[i].size * 0.5f);
        renderables.emplace_back(&rendSpheres[i]);
    }

    //create spaceship
    rendSpaceship.context2 = &spaceshipContext;
    rendSpaceship.textureId2 = spaceshipTexture;
    rendSpaceship.localTransform2 = glm::scale(Spaceship::spaceship.size * 0.05f);
    renderablesSpaceship.emplace_back(&rendSpaceship);
}

// helper function: glm::vec3 position -> PxTransform
PxTransform posToPxTransform(glm::vec3 const& pos) {
    return PxTransform(pos.x, pos.y, pos.z);
}
// helper function: glm::vec3 size -> PxBoxGeometry
PxBoxGeometry sizeToPxBoxGeometry(glm::vec3 const& size) {
    auto h = size * 0.005f;
    return PxBoxGeometry(h.x, h.y, h.z);
}

void createDynamicBox(PxRigidDynamic* &body, Renderable *rend, glm::vec3 const& pos, glm::vec3 const& size)
{
    body = pxScene.physics->createRigidDynamic(posToPxTransform(pos));
    PxShape* boxShape = pxScene.physics->createShape(sizeToPxBoxGeometry(size), *material);
    body->attachShape(*boxShape);
    boxShape->release();
    body->userData = rend;
    pxScene.scene->addActor(*body);
}

void createDynamicSphere(PxRigidDynamic* &body, Renderable *rend, glm::vec3 const& pos, float radius)
{
    body = pxScene.physics->createRigidDynamic(posToPxTransform(pos));
    PxShape* sphereShape = pxScene.physics->createShape(PxSphereGeometry(radius), *material);
    body->attachShape(*sphereShape);
    sphereShape->release();
    body->userData = rend;
    pxScene.scene->addActor(*body);
}

void createDynamicSpaceship(PxRigidDynamic*& body, RenderableSpaceship* rend, glm::vec3 const& pos, glm::vec3 const& size)
{
    body = pxScene.physics->createRigidDynamic(posToPxTransform(pos));
    PxShape* boxShape = pxScene.physics->createShape(sizeToPxBoxGeometry(size), *material);
    body->attachShape(*boxShape);
    boxShape->release();
    body->userData = rend;
    pxScene.scene->addActor(*body);
}

void initPhysicsScene()
{
    // single common material
    material = pxScene.physics->createMaterial(0.5f, 0.5f, 0.6f);

    // create ground
    bodyGround = pxScene.physics->createRigidStatic(PxTransformFromPlaneEquation(PxPlane(0, 1, 0, 0)));
    PxShape* planeShape = pxScene.physics->createShape(PxPlaneGeometry(), *material);
    bodyGround->attachShape(*planeShape);
    planeShape->release();
    bodyGround->userData = &rendGround;
    pxScene.scene->addActor(*bodyGround);


    // create spheres
    for (int i = 0; i < Objects::numSpheres; i++) {
        createDynamicSphere(bodySpheres[i], &rendSpheres[i], Objects::spheres[i].pos, Objects::spheres[i].size.x * 0.5f);
        PxRigidBodyExt::setMassAndUpdateInertia(*bodySpheres[i], 0.1f);
    }

    // create spaceship
    createDynamicSpaceship(bodySpaceship, &rendSpaceship, Spaceship::spaceship.pos, Spaceship::spaceship.size);
    PxRigidBodyExt::setMassAndUpdateInertia(*bodySpaceship, 1.f);

    //-----------------------------------------------------------
    // TASKS
    //-----------------------------------------------------------
    // Your task is to joint bodies together. See zadanie.png for details.
    // For help, read https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Joints.html
    // Use keys J and L to move the handle left and right

    // 1. Create spherical joint between the handle and the first sphere
    // Use method: PxSphericalJointCreate
    // The localFrame of the handle should be (0, -0.5 * handle.size.y, 0)
    // The localFrame of the sphere should be (0, 0.5 * sphere.size.y, 0)
    // ...
/*
    PxSphericalJointCreate(*pxScene.physics, bodyHandle , PxTransform(0,-0.5 * Objects::handle.size.y,0), bodySpheres[0], PxTransform(0, 0.5 * Objects::spheres[0].size.y, 0, PxQuat(PxPi / 2, PxVec3(0, 0, 1))));
    
    // 2. Create spherical joint between consecutive pairs of spheres
    // Set localFrames properly (to the point between the spheres)
    // ...
    PxSphericalJointCreate(*pxScene.physics, bodySpheres[0], PxTransform(0, 0.5 * Objects::spheres[0].size.y, 0, PxQuat(PxPi / 2, PxVec3(0, 0, 1))), bodySpheres[1], PxTransform(0.5, 0.5 * Objects::spheres[1].size.y, 0, PxQuat(PxPi / 2, PxVec3(0, 0, 1))));
    PxSphericalJointCreate(*pxScene.physics, bodySpheres[1], PxTransform(0, 0.5 * Objects::spheres[1].size.y, 0, PxQuat(PxPi / 2, PxVec3(0, 0, 1))), bodySpheres[2], PxTransform(0.5, 0.5 * Objects::spheres[2].size.y, 0, PxQuat(PxPi / 2, PxVec3(0, 0, 1))));

    
    // Here, prismatic joint is created for the box to slide on the ground
    PxPrismaticJointCreate(*pxScene.physics,
        bodyGround, PxTransform(0.5f * Objects::box.size.y, 0, 0, PxQuat(PxPi/2, PxVec3(0,0,1))),
        bodyBox, PxTransform(0, 0, 0));
    
    // Here, revolute joint is created for the gate to open
    auto gateJoint = PxRevoluteJointCreate(*pxScene.physics,
        bodyGround, PxTransform(0, -Objects::gate.pos.x, Objects::gate.pos.z + 0.5f * Objects::gate.size.z),
        bodyGate, PxTransform(0, -0.5f * Objects::gate.size.y, 0.5f * Objects::gate.size.z, PxQuat(PxPi / 2, PxVec3(0, 0, 1))));
    // 3. Fix the limit of the joint to enable opening of the gate
    gateJoint->setLimit(PxJointAngularLimitPair(-PxPi / 2, PxPi/2, 0.01f));
    gateJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
    */
}

void updateTransforms()
{
    // Here we retrieve the current transforms of the objects from the physical simulation.
    auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC;
    PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
    if (nbActors)
    {
        vector<PxRigidActor*> actors(nbActors);
        pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);
        for (auto actor : actors)
        {
            // We use the userData of the objects to set up the model matrices
            // of proper renderables.
            if (!actor->userData) continue;
            Renderable *renderable = (Renderable*)actor->userData;

            // get world matrix of the object (actor)
            PxMat44 transform = actor->getGlobalPose();
            auto &c0 = transform.column0;
            auto &c1 = transform.column1;
            auto &c2 = transform.column2;
            auto &c3 = transform.column3;

            // set up the model matrix used for the rendering
            renderable->physicsTransform = glm::mat4(
                c0.x, c0.y, c0.z, c0.w,
                c1.x, c1.y, c1.z, c1.w,
                c2.x, c2.y, c2.z, c2.w,
                c3.x, c3.y, c3.z, c3.w);
        }
    }
}

/*
void moveHandle(float offset) {
    if (!bodyHandle) return;
    Objects::handle.pos.x += offset;
    bodyHandle->setGlobalPose(posToPxTransform(Objects::handle.pos));
    for (int i = 0; i < Objects::numSpheres; i++)
        bodySpheres[i]->wakeUp();
}
*/

void keyboard(unsigned char key, int x, int y)
{
    float angleSpeed = 0.1f;
    float moveSpeed = 0.1f;
    float handleSpeed = 0.05f;
    switch (key)
    {
    /*case 'z': cameraAngle -= angleSpeed; break;
    case 'x': cameraAngle += angleSpeed; break;
    case 'w': cameraPos += cameraDir * moveSpeed; break;
    case 's': cameraPos -= cameraDir * moveSpeed; break;
    case 'd': cameraPos += cameraSide * moveSpeed; break;
    case 'a': cameraPos -= cameraSide * moveSpeed; break;*/;
	case 'w': distanceX  = 10; break;
	case 's': distanceX = -10; break;
	case 'd': distanceY = 10; break;
	case 'a': distanceY = -10; break;

   // case 'j': moveHandle(-handleSpeed);  break;
   // case 'l': moveHandle(handleSpeed);  break;
    }
}

void keyboardUp(unsigned char key, int x, int y)
{

	switch (key)
	{
	case 'w': distanceX = 0; break;
	case 's': distanceX = 0; break;
	case 'd': distanceY = 0; break;
	case 'a': distanceY = 0; break;
	}
}
void resetPointer() {
	glutWarpPointer(400, 400);
	lastX = 400;
	lastY = 400;
}
void mouse( int x, int y)
{
	if (startMouse)
	{
		lastX = x;
		lastY = y;
		startMouse = 0;
		return;
	}

	float xoffset = x - lastX;
	float yoffset = lastY - y;
	lastX = x;
	lastY = y;

	xoffset *= sensitive;
	yoffset *= sensitive;

	Yaw += xoffset;
	Pitch += yoffset;

	if (Pitch > 89.0f)
		Pitch = 89.0f;
	if (Pitch < -89.0f)
		Pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	direction.y = sin(glm::radians(Pitch));
	direction.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	cameraDir = glm::normalize(direction);
	cout << x << " " << y << endl;
	resetPointer();

}

glm::mat4 createCameraMatrix()
{
   // cameraDir = glm::normalize(glm::vec3(cosf(cameraAngle - glm::radians(90.0f)), 0, sinf(cameraAngle - glm::radians(90.0f))));
    glm::vec3 up = glm::vec3(0, 1, 0);
    cameraSide = glm::cross(cameraDir, up);

    return Core::createViewMatrix(cameraPos, cameraDir, up);
	return Core::createViewMatrix(cameraPos - cameraDir, cameraPos + vec3(0, 1, 0), up);
}

void drawObject(Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 color, GLuint program)
{
    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);


    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(context);
    glUseProgram(0);
}

void drawObjectColor(Core::RenderContext context, glm::mat4 modelMatrix, glm::vec3 color)
{
    GLuint program = programColor;

    glUseProgram(program);

    glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(context);

    glUseProgram(0);
}

void drawObjectTexture(Core::RenderContext context, glm::mat4 modelMatrix, GLuint textureId, int shaderId)
{
	vector< GLuint> shaders;

    shaders.push_back(programTexture);
	shaders.push_back(programSkybox);


    glUseProgram(shaders[shaderId]);

    glUniform3f(glGetUniformLocation(shaders[shaderId], "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    Core::SetActiveTexture(textureId, "textureSampler", shaders[shaderId], 0);

    glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(shaders[shaderId], "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(shaders[shaderId], "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

    Core::DrawContext(context);

    glUseProgram(0);
}

void renderScene()
{
    // Update of camera and perspective matrices
    cameraMatrix = createCameraMatrix();
    perspectiveMatrix = Core::createPerspectiveMatrix();

    double time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    static double prevTime = time;
    double dtime = time - prevTime;
    prevTime = time;

	//obliczanie przesuniêcia ruchu 
	float distancex = distanceX * dtime;
	float distancey = distanceY * dtime;
	cameraPos += distancex * cameraDir;
	cameraPos += distancey * normalize(cross(cameraDir, vec3(0, 1, 0)));

    // Update physics
    if (dtime < 1.f) {
        physicsTimeToProcess += dtime;
        while (physicsTimeToProcess > 0) {
            // here we perform the physics simulation step
            pxScene.step(physicsStepTime);
            physicsTimeToProcess -= physicsStepTime;
        }
    }


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // update transforms from physics simulation
    updateTransforms();

    // render models
    for (Renderable* renderable : renderables) {
        glm::mat4 transform = renderable->physicsTransform * renderable->localTransform;
        drawObjectTexture(*renderable->context, transform, renderable->textureId,0);
    }

    //glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f + glm::vec3(0, -0.25f, 0)) * glm::rotate(-cameraAngle + glm::radians(90.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
    
    for (RenderableSpaceship* renderable : renderablesSpaceship) {
        glm::mat4 transform = renderable->physicsTransform2 * renderable->localTransform2;
       // glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.75f + glm::vec3(0, -0.25f, 0)) * rotate(radians(Pitch), glm::vec3(1, 0, 0)) * rotate(radians(-1*Yaw+90), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.2f));
		glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.75f + glm::vec3(0, -0.25f, 0));
		shipModelMatrix[0][0] = cameraMatrix[0][0];
		shipModelMatrix[0][1] = cameraMatrix[1][0];
		shipModelMatrix[0][2] = cameraMatrix[2][0];
		shipModelMatrix[1][0] = cameraMatrix[0][1];
		shipModelMatrix[1][1] = cameraMatrix[1][1];
		shipModelMatrix[1][2] = cameraMatrix[2][1];
		shipModelMatrix[2][0] = cameraMatrix[0][2];
		shipModelMatrix[2][1] = cameraMatrix[1][2];
		shipModelMatrix[2][2] = cameraMatrix[2][2];

		
		shipModelMatrix *= rotate(radians(180.0f), glm::vec3(0, 1, 0))*glm::scale(glm::vec3(0.2f));
        drawObjectTexture(*renderable->context2, shipModelMatrix, renderable->textureId2,0);
    }

	for (Renderable* renderable : noshadow) {
		glm::mat4 transform = renderable->physicsTransform * renderable->localTransform;
		drawObjectTexture(*renderable->context, transform, renderable->textureId,1);
	}


    glm::mat4 jowModelMatrix = glm::translate(glm::vec3(3, 2.75f, 0)) * glm::scale(glm::vec3(0.5f));
    drawObjectTexture(sphereContext, glm::translate(glm::vec3(0, 2, 2)),sunTexture,0);
    drawObjectTexture(sphereContext, jowModelMatrix, jowiszTexture,0);

    /*//SKYBOX

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glUseProgram(programSkybox);

    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    glUseProgram(programSkybox);
    //cameraMatrix = glm::mat4(glm::mat3(cameraMatrix)); // remove translation from the view matrix
    cameraMatrix = glm::mat4(glm::mat3(createCameraMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(programSkybox, "perspectiveMatrix"), 1, GL_FALSE, (float*)&perspectiveMatrix);
    glUniformMatrix4fv(glGetUniformLocation(programSkybox, "cameraMatrix"), 1, GL_FALSE, (float*)&cameraMatrix);

    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back t*/

    glutSwapBuffers();
}

void init()
{
    srand(time(0));
    glEnable(GL_DEPTH_TEST);
    programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
    programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
    programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");

    program_Sun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");
    sphereModel = obj::loadModelFromFile("models/sphere.obj");
    sphereContext.initFromOBJ(sphereModel);

    initRenderables();
    initPhysicsScene();

    vector<std::string> faces
    {
    "skybox/right.png",
    "skybox/left.png",
    "skybox/top.png",
    "skybox/bottom.png",
    "skybox/front.png",
    "skybox/back.png"
        /*"resources/textures/skybox/right.png",
        "resources/textures/skybox/left.png",
        "resources/textures/skybox/top.png",
        "resources/textures/skybox/bottom.png",
        "resources/textures/skybox/front.png",
        "resources/textures/skybox/back.png"*/
    };


    cubemapTexture = Core::loadCubemap(faces);
}

void shutdown()
{
    shaderLoader.DeleteProgram(programColor);
    shaderLoader.DeleteProgram(programTexture);
}

void idle()
{
    glutPostRedisplay();
}

int main(int argc, char ** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(900, 900);
    glutCreateWindow("OpenGL + PhysX");
    glewInit();

    init();
	glutSetCursor(GLUT_CURSOR_NONE);
    glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutPassiveMotionFunc(mouse);

    glutDisplayFunc(renderScene);
    glutIdleFunc(idle);

    glutMainLoop();

    shutdown();

    return 0;
}

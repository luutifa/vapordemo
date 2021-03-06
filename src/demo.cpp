#include "demo.hpp"
#include "3dapp_graphics.hpp"
#include <iostream>
#include "3dapp_paths.hpp"
#include "3dapp_shader.hpp"
#include "3dapp_geo_primitives.hpp"
#include "3dapp_vectors.hpp"
#include "3dapp_uniforms.hpp"
#include <string>
#include <cstdio>

#include "part_logo.hpp"
#include "part_vapor1.hpp"
#include "part_trap1.hpp"

#include "draw_line.hpp"

Demo* Demo::instance;

Demo::Demo(Window& _window):
Application(_window),
demoStart(0.0f),
rect(vec2(DEMO_W, DEMO_H), vec2(_window.getWidth(), _window.getHeight())),
internalRes(DEMO_W, DEMO_H),
internalAspectRatio(DEMO_W/DEMO_H),
sync("sync.txt"),
shaderPostAnalog(Shader::loadFromFile(shaderPath("simple.vert")), Shader::loadFromFile(shaderPath("analog.frag"))),
shaderPostBlur(Shader::loadFromFile(shaderPath("simple.vert")), Shader::loadFromFile(shaderPath("fastblur.frag"))),
shaderSimple(Shader::loadFromFile(shaderPath("simple.vert")), Shader::loadFromFile(shaderPath("generic.frag"))),
fboPostAnalog(DEMO_W, DEMO_H),
fboPostBlur(DEMO_W/2, DEMO_H/2),
fboMain(DEMO_W*DEMO_POST_SIZE_MULT, DEMO_H*DEMO_POST_SIZE_MULT) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_CULL_FACE);
    
    fboPostBlur.getTexture().setFilter(GL_LINEAR);
    //fboPostAnalog.getTexture().setFilter(GL_LINEAR);
    
    setBaseUniforms(shaderPostAnalog, 2);
    setBaseUniforms(shaderSimple, 1);
    setBaseUniforms(shaderPostBlur, 1);
    shaderPostBlur.use();
    setResolutionUniform(shaderPostBlur, vec2(DEMO_W/2, DEMO_H/2));

    parts.push_back(new PartLogo(sync.getTime(SYNC_PART, 0)));
    parts.push_back(new PartVapor1(sync.getTime(SYNC_PART, 1)));
    parts.push_back(new PartTrap1(sync.getTime(SYNC_PART, 2), (130.55f/60.0f)));
    glClearColor(0,0,0,0);
}

Demo::~Demo() {
    for (unsigned int i=0; i<parts.size(); i++) {
        delete parts[i];
    }
}

void Demo::draw() {
    fboPostAnalog.bind();
    //fboPostBlur.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    float part = sync.getValue(SYNC_PART, getTime());
    if (part<-0.1f)
		exit(ERR_SUCCESS);
    parts[(unsigned int)(part+0.5f)]->draw();
    
    //Blur to Analog
    fboPostBlur.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderPostBlur.use();
    fboPostAnalog.getTexture().bindToUnit(0);
    GeoPrimitives::singleton().quad.draw(shaderPostBlur);
    
    //Analog postproc to scaling buffer*/
    fboMain.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderPostAnalog.use();
    //shaderPostBlur.use();
    setTimeUniform(shaderPostAnalog, getTime());
    glUniform1f(shaderPostAnalog.getUfmHandle("brightness"), sync.getValue(SYNC_BRIGHTNESS, getTime()));
    glUniform1f(shaderPostAnalog.getUfmHandle("contrast"), sync.getValue(SYNC_CONTRAST, getTime()));
    glUniform1f(shaderPostAnalog.getUfmHandle("blur"), sync.getValue(SYNC_BLUR, getTime()));
    glUniform1f(shaderPostAnalog.getUfmHandle("glitchiness"), sync.getValue(SYNC_GLITCHINESS, getTime()));
    fboPostAnalog.getTexture().bindToUnit(0);
    fboPostBlur.getTexture().bindToUnit(1);
    //fboPostBlur.getTexture().bindToUnit(0);
    GeoPrimitives::singleton().quad.draw(shaderPostAnalog);
    //GeoPrimitives::singleton().quad.draw(shaderPostBlur);
    
    //Scaling buffer to window
    window.bindFramebuffer();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderSimple.use();
    fboMain.getTexture().bindToUnit(0);
    rect.draw(shaderSimple);
    
    window.swapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    check();
}

Window& Demo::getWindow() {
    return window;
}

void Demo::createSingleton(Window& _window) {
    if (!instance)
        instance = new Demo(_window);
    else
        std::cout << "Cannot abandon initialized Demo instance.\n";
}

void Demo::deleteSingleton() {
    if (instance)
        delete instance;
    else
        std::cout << "Cannot delete uninitialized Demo instance.\n";
}

Demo& Demo::singleton() {
    if (!instance) {
        std::cout << "Cannot get uninitialized Demo instance.\n";
        exit(ERR_WTF);
    }
    return *instance;
}

float Demo::getInternalAspectRatio() {
    return internalAspectRatio;
}

vec2 Demo::getInternalResolution() {
    return internalRes;
}

Sync& Demo::getSync() {
    return sync;
}

float Demo::getTime(float scale, float offset) {
	return ((window.getTime()-demoStart)+offset)*scale;
}

void Demo::startTimer(float t) {
	demoStart = window.getTime()-t;
}

Framebuffer& Demo::getLastFBO() {
    return fboPostAnalog;
}

#include "scene.h"

#include "platform.h"
#include "util/stringsOp.h"

#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

Scene::Scene() {

}

void Scene::addStyle(std::unique_ptr<Style> _style) {
    m_styles.push_back(std::move(_style));
}

void Scene::addLight(std::shared_ptr<Light> _light, InjectionType _type) {
    
    // For the first light added, add the main lighting function
    if (m_lights.size() == 0) {
        std::string vertexLightBlock = stringFromResource("lights_vert.glsl");
        std::string fragmentLightBlock = stringFromResource("lights_frag.glsl");
        
        //  TODO:
        //          - BM - remove the _ on the injection points
        //
        for (auto& style : m_styles) {
            style->getShaderProgram()->addSourceBlock("_vertex_lighting", vertexLightBlock+"\n");
            style->getShaderProgram()->addSourceBlock("_fragment_lighting", fragmentLightBlock+"\n");
        }
    }

    //  Avoid duplications
    if (m_lights.find(_light->getName()) == m_lights.end()) {
        for (auto& style : m_styles) {
            _light->injectOnProgram(style->getShaderProgram(), _type);

            //  TODO:
            //          - define injection should make this simpler checking duplications

            std::string define = "";
            if( _light->getInjectionType() == FRAGMENT || _light->getInjectionType() == BOTH){
                define += "#ifndef TANGRAM_FRAGMENT_LIGHTS\n";
                define += "#define TANGRAM_FRAGMENT_LIGHTS\n";
                define += "#endif\n";
            } 

            if( _light->getInjectionType() == VERTEX || _light->getInjectionType() == BOTH){
                define += "#ifndef TANGRAM_VERTEX_LIGHTS\n";
                define += "#define TANGRAM_VERTEX_LIGHTS\n";
                define += "#endif\n";
            }
            style->getShaderProgram()->addSourceBlock("defines",define);
        }
        m_lights[_light->getName()] = _light;
    } else {
        logMsg("ERROR, Can't add the same light twice. Try using another the name instead.\n");
    }
}

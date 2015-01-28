#pragma once

#include <vector>
#include "glm/geometric.hpp"

class Rectangle {
public:
    
    Rectangle();
    
    /* The vec4 sets x,y and width (vec4.z) and height (vec4.w) */
    Rectangle(const glm::vec4 &_vec4);
    
    /* The ivec4 sets x,y and width (ivec4.z) and height (ivec4.w)
     * Ex:
     *  glm::ivec4 viewport;
     *  glGetIntegerv(GL_VIEWPORT, &viewport[0]);
     *  Rectangle vp = Rectangle(viewport);
     */
    Rectangle(const glm::ivec4 &_viewPort);
    
    /* Create a rectangle from other one, with the option to add some margins on the sides */
    Rectangle(const Rectangle &_rectangel, const float &_margin = 0.0);
    
    /* Element by element constructor */
    Rectangle(const float &_x, const float &_y, const float &_width, const float &_height);
    virtual ~Rectangle();
    
    /* The vec4 sets x,y and width (vec4.z) and height (vec4.w) */
    void    set(const glm::vec4 &_vec4);
    
    /* The vec4 sets x,y and width (vec4.z) and height (vec4.w)
     * Ex:
     *  glm::ivec4 viewport;
     *  glGetIntegerv(GL_VIEWPORT, &viewport[0]);
     *  Rectangle vp;
     *  vp.set(viewport);
     */
    void    set(const glm::ivec4 &_viewPort);
    
    /* Sets values element by element */
    void    set(const float &_x, const float &_y, const float &_width, const float &_height);
    
    /* Translate the position of the rectangle */
    void    translate(const glm::vec3 &_pos);
    
    /*  Grow the area of the rectangle to include one or several points*/
    void    growToInclude(const glm::vec3& _point);
    void    growToInclude(const std::vector<glm::vec3> &_points);
    
    float   getMinX() const;
    float   getMaxX() const;
    
    float   getMinY() const;
    float   getMaxY() const;
    
    float   getLeft()   const;
    float   getRight()  const;
    
    /* Return top value of the rectangle assuming y-down coordinate system */
    float   getTop()    const;
    
    /* Return bottom value of the rectangle assuming y-down coordinate system */
    float   getBottom() const;
    
    glm::vec3   getMin() const;
    glm::vec3   getMax() const;
    
    glm::vec3   getCenter() const;
    
    /* Return top-left coorner of the rectangle assuming y-down coordinate system */
    glm::vec3   getTopLeft() const;
    
    /* Return top-right coorner of the rectangle assuming y-down coordinate system */
    glm::vec3   getTopRight() const;
    
    /* Return bottom-left coorner of the rectangle assuming y-down coordinate system */
    glm::vec3   getBottomLeft() const;
    
    /* Return bottom-right coorner of the rectangle assuming y-down coordinate system */
    glm::vec3   getBottomRight() const;
    
    float   x,y,width,height;
};

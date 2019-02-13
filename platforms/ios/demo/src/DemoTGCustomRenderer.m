//
//  DemoTGCustomRenderer.m
//  TangramDemo
//
//  Created by Varun Talwar on 7/18/18.
//

#import "DemoTGCustomRenderer.h"
#import <GLKit/GLKit.h>

typedef struct Vertex {
    float x, y;
} Vertex;

Vertex vertices[] = {
    { -0.5, -0.5 },
    {  0.5, -0.5 },
    { -0.5,  0.5 },
    {  0.5, -0.5 },
    {  0.5,  0.5 },
    { -0.5,  0.5 },
};

const GLchar *vertShaderSrc =
"precision highp float;\n"
"attribute vec2 a_position;\n"
"varying vec4 v_color;\n"
"void main() {\n"
"    v_color = vec4(0., 1., 0., 1.);\n"
"    gl_Position = vec4(a_position.x, a_position.y, 0., 1.);\n"
"}\n";

const GLchar *fragShaderSrc =
"precision highp float;\n"
"varying vec4 v_color;\n"
"void main() {\n"
"   gl_FragColor = v_color;\n"
"}\n";

@interface DemoTGCustomRenderer () <TGCustomRenderer> {
    GLuint _shaderProg;
    GLuint _fragShader;
    GLuint _vertShader;
    GLuint _posAttrLoc;
    GLuint _vertexBuffer;
}
@end

@implementation DemoTGCustomRenderer

- (void)logGLError:(NSString *)location
{
    int e = glGetError();
    NSString* message;
    while (e != GL_NO_ERROR) {
        switch (e) {
            case GL_INVALID_ENUM:
                message = @"GL_INVALID_ENUM\n";
                break;
            case GL_INVALID_VALUE:
                message = @"GL_INVALID_VALUE\n";
                break;
            case GL_INVALID_OPERATION:
                message = @"GL_INVALID_OPERATION\n";
                break;
            case GL_OUT_OF_MEMORY:
                message = @"GL_OUT_OF_MEMORY\n";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                message = @"GL_INVALID_FRAMEBUFFER_OPERATION\n";
                break;
            default:
                message = @"???\n";
                break;
        }
        NSLog(@"%@: %@", location, message);
        e = glGetError();
    }
}

- (void)prepare
{
    GLint status = -1;
    GLchar shaderLog[1024];

    _vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(_vertShader, 1, &vertShaderSrc, NULL);
    glCompileShader(_vertShader);
    glGetShaderInfoLog(_vertShader, 1024, &status, shaderLog);
    if (status > 0) {
        NSLog(@"vert shader error: %s", shaderLog);
    }

    _fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(_fragShader, 1, &fragShaderSrc, NULL);
    glCompileShader(_fragShader);
    glGetShaderInfoLog(_fragShader, 1024, &status, shaderLog);
    if (status > 0) {
        NSLog(@"frag shader error: %s", shaderLog);
    }

    _shaderProg = glCreateProgram();
    glAttachShader(_shaderProg, _vertShader);
    glAttachShader(_shaderProg, _fragShader);
    glLinkProgram(_shaderProg);
    glGetProgramiv(_shaderProg, GL_LINK_STATUS, &status);
    NSLog(@"linked: %d", status);
    [self logGLError:@"0"];

    _posAttrLoc = glGetAttribLocation(_shaderProg, "a_position");
    [self logGLError:@"1"];

    glGenBuffers(1, &_vertexBuffer);
    [self logGLError:@"2"];
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    [self logGLError:@"3"];
    glVertexAttribPointer(_posAttrLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
    [self logGLError:@"4"];
    glEnableVertexAttribArray(_posAttrLoc);
    [self logGLError:@"5"];

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    [self logGLError:@"6"];
}

- (void)drawWithContext:(TGCustomRendererContext)context
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(_shaderProg);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glVertexAttribPointer(_posAttrLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, x));
    glEnableVertexAttribArray(_posAttrLoc);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

- (void)complete
{
    glDeleteProgram(_shaderProg);
    _shaderProg = 0;
    glDeleteShader(_vertShader);
    _vertShader = 0;
    glDeleteShader(_fragShader);
    _fragShader = 0;

    glDeleteBuffers(1, &_vertexBuffer);
    _vertexBuffer = 0;
}

@end

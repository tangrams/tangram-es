#include "tangram.h"

const GLfloat vertices[] = {
	0.0f, 1.0f,
	-1.0f, 0.0f,
	1.0f, 0.0f
};

GLint shaderProgram;
GLuint vbo;
GLint posAttrib;
float t;

const GLchar* vertShaderSrc =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "attribute vec4 a_position;\n"
	"void main() {\n"
	"  gl_Position = a_position;\n"
	"}\n";

const GLchar* fragShaderSrc =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"#endif\n"
	"void main(void) {\n"
	"  gl_FragColor = vec4(0.0/255.0, 141.0/255.0, 148.0/255.0, 1.0);\n"
	"}\n";

void initializeOpenGL()
{

	// Make a VBO
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Make a shader program
	shaderProgram = makeLinkedShaderProgram(vertShaderSrc, fragShaderSrc);

	// Link vertex attribute data to shader program
	posAttrib = glGetAttribLocation(shaderProgram, "a_position");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	t = 0;

	logMsg("%s\n", "initializeOpenGL");
}

void resizeViewport(int newWidth, int newHeight)
{
	glViewport(0, 0, newWidth, newHeight);

	logMsg("%s\n", "resizeViewport");
}

void renderFrame()
{

	// Draw a triangle!
	t += 0.016;
	float sintsqr = pow(sin(t), 2);
	glClearColor(0.8f * sintsqr, 0.32f * sintsqr, 0.3f * sintsqr, 1.0f);
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgram);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	GLenum glError = glGetError();
	if (glError) {
		logMsg("%s\n", "glError!!");
	}

}

GLuint makeCompiledShader(const GLchar* src, GLenum type) {

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

	if (!compileStatus) {
		GLint infoLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength > 1) {
			char* infoLog = (char*)malloc(sizeof(char) * infoLength);
			//	char* infoLog = new char[infoLength];
			glGetShaderInfoLog(shader, infoLength, NULL, infoLog);
			logMsg("Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}

		glDeleteShader(shader);
		assert(0);
		return 0;
	}

	return shader;
}

GLuint makeLinkedShaderProgram(const GLchar* vertexSrc, const GLchar* fragmentSrc) {

	GLuint program = glCreateProgram();

	GLuint vertexShader = makeCompiledShader(vertexSrc, GL_VERTEX_SHADER);

	if (vertexShader == 0) {
		assert(0);
		return 0;
	}

	GLuint fragmentShader = makeCompiledShader(fragmentSrc, GL_FRAGMENT_SHADER);

	if (fragmentShader == 0) {
		glDeleteShader(vertexShader);
		assert(0);
		return 0;
	}

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint linkerStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkerStatus);

	if (!linkerStatus) {
		GLint infoLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
		if (infoLength > 1) {
			char* infoLog = (char*)malloc(sizeof(char)*infoLength);
			//char* infoLog = new char[infoLength];
			glGetProgramInfoLog(program, infoLength, NULL, infoLog);
			logMsg("Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(program);
	}

	return program;
}

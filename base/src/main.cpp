#include "main.h"

#include "Utility.h"

#include <GL/glut.h>
#include "SOIL.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_projection.hpp>
#include <glm/gtc/matrix_operation.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;
using namespace glm;

const float PI = 3.14159f;

int width, height;

/***** Uniform variables that are interaction controllable *****/
int kernelWidthX = 3;
int kernelWidthY = 3;
int postProcessMode = 0;
int numLights = 12345;
vector<vec3>lightPos;
vector<vec3>lightCol;
vector<float> lightInt;
void setupLights()
{
	lightPos.resize(0);
	lightInt.resize(0);
	for(int iter=0; iter<numLights;iter++)
	{
		float x = 30*((double) rand() / (RAND_MAX)) - 15;
		float y = 20*((double) rand() / (RAND_MAX)) - 10;
		float z = 15*((double) rand() / (RAND_MAX)) ;

		float r = ((double) rand() / (RAND_MAX)) ;
		float g = ((double) rand() / (RAND_MAX)) ;
		float b = ((double) rand() / (RAND_MAX)) ;

		vec3 col = vec3(r,g,b);
		col = normalize(col);
		col = abs(col);

		float l = 1.0*((double) rand() / (RAND_MAX));
		lightPos.push_back(vec3(x,y,z));
		lightCol.push_back(col);
		lightInt.push_back(l);
	}
}
/***************************************************************/

device_mesh_t uploadMesh(const mesh_t & mesh) {
    device_mesh_t out;
    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex 
    //attributes and the buffers they are bound to
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(out.vertex_array));
    glBindVertexArray(out.vertex_array);

    //Allocate vbos for data
    glGenBuffers(1, &(out.vbo_vertices));
    glGenBuffers(1, &(out.vbo_normals));
    glGenBuffers(1, &(out.vbo_indices));
    glGenBuffers(1, &(out.vbo_texcoords));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size()*sizeof(vec3), 
            &mesh.vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::POSITION);
    //cout << mesh.vertices.size() << " verts:" << endl;
    //for(int i = 0; i < mesh.vertices.size(); ++i)
    //    cout << "    " << mesh.vertices[i][0] << ", " << mesh.vertices[i][1] << ", " << mesh.vertices[i][2] << endl;

    //Upload normal data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_normals);
    glBufferData(GL_ARRAY_BUFFER, mesh.normals.size()*sizeof(vec3), 
            &mesh.normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::NORMAL, 3, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::NORMAL);
    //cout << mesh.normals.size() << " norms:" << endl;
    //for(int i = 0; i < mesh.normals.size(); ++i)
    //    cout << "    " << mesh.normals[i][0] << ", " << mesh.normals[i][1] << ", " << mesh.normals[i][2] << endl;

    //Upload texture coord data
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo_texcoords);
    glBufferData(GL_ARRAY_BUFFER, mesh.texcoords.size()*sizeof(vec2), 
            &mesh.texcoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(mesh_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,0,0);
    glEnableVertexAttribArray(mesh_attributes::TEXCOORD);
    //cout << mesh.texcoords.size() << " texcos:" << endl;
    //for(int i = 0; i < mesh.texcoords.size(); ++i)
    //    cout << "    " << mesh.texcoords[i][0] << ", " << mesh.texcoords[i][1] << endl;

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(GLushort), 
            &mesh.indices[0], GL_STATIC_DRAW);
    out.num_indices = mesh.indices.size();
    //Unplug Vertex Array
    glBindVertexArray(0);

	out.shininess = mesh.shininess;
    out.texname = mesh.texname;
	out.meshName = mesh.meshName;
    out.color = mesh.color;
    return out;
}


int num_boxes = 3;
const int DUMP_SIZE = 1024;

vector<device_mesh_t> draw_meshes;
void initMesh() {
    for(vector<tinyobj::shape_t>::iterator it = shapes.begin();
            it != shapes.end(); ++it)
    {
        tinyobj::shape_t shape = *it;
        int totalsize = shape.mesh.indices.size() / 3;
        int f = 0;
        while(f<totalsize){
            mesh_t mesh;
            int process = std::min(10000, totalsize-f);
            int point = 0;
            for(int i=f; i<process+f; i++){
                int idx0 = shape.mesh.indices[3*i];
                int idx1 = shape.mesh.indices[3*i+1];
                int idx2 = shape.mesh.indices[3*i+2];
                vec3 p0 = vec3(shape.mesh.positions[3*idx0],
                               shape.mesh.positions[3*idx0+1],
                               shape.mesh.positions[3*idx0+2]);
                vec3 p1 = vec3(shape.mesh.positions[3*idx1],
                               shape.mesh.positions[3*idx1+1],
                               shape.mesh.positions[3*idx1+2]);
                vec3 p2 = vec3(shape.mesh.positions[3*idx2],
                               shape.mesh.positions[3*idx2+1],
                               shape.mesh.positions[3*idx2+2]);

                mesh.vertices.push_back(p0);
                mesh.vertices.push_back(p1);
                mesh.vertices.push_back(p2);

                if(shape.mesh.normals.size() > 0)
                {
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx0],
                                                shape.mesh.normals[3*idx0+1],
                                                shape.mesh.normals[3*idx0+2]));
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx1],
                                                shape.mesh.normals[3*idx1+1],
                                                shape.mesh.normals[3*idx1+2]));
                    mesh.normals.push_back(vec3(shape.mesh.normals[3*idx2],
                                                shape.mesh.normals[3*idx2+1],
                                                shape.mesh.normals[3*idx2+2]));
                }
                else
                {
                    vec3 norm = normalize(glm::cross(normalize(p1-p0), normalize(p2-p0)));
                    mesh.normals.push_back(norm);
                    mesh.normals.push_back(norm);
                    mesh.normals.push_back(norm);
                }

                if(shape.mesh.texcoords.size() > 0)
                {
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx0],
                                                  shape.mesh.positions[2*idx0+1]));
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx1],
                                                  shape.mesh.positions[2*idx1+1]));
                    mesh.texcoords.push_back(vec2(shape.mesh.positions[2*idx2],
                                                  shape.mesh.positions[2*idx2+1]));
                }
                else
                {
                    vec2 tex(0.0);
                    mesh.texcoords.push_back(tex);
                    mesh.texcoords.push_back(tex);
                    mesh.texcoords.push_back(tex);
                }
                mesh.indices.push_back(point++);
                mesh.indices.push_back(point++);
                mesh.indices.push_back(point++);
            }

            mesh.color = vec3(shape.material.diffuse[0],
                              shape.material.diffuse[1],
                              shape.material.diffuse[2]);
			
			mesh.shininess = shape.material.shininess;
            mesh.texname = shape.material.diffuse_texname;
			mesh.meshName = shape.name;
            draw_meshes.push_back(uploadMesh(mesh));
            f=f+process;
        }
    }
}


device_mesh2_t device_quad;
void initQuad() {
    vertex2_t verts [] = { {vec3(-1,1,0),vec2(0,1)},
        {vec3(-1,-1,0),vec2(0,0)},
        {vec3(1,-1,0),vec2(1,0)},
        {vec3(1,1,0),vec2(1,1)}};

    unsigned short indices[] = { 0,1,2,0,2,3};

    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
    //Different vertex array per mesh.
    glGenVertexArrays(1, &(device_quad.vertex_array));
    glBindVertexArray(device_quad.vertex_array);


    //Allocate vbos for data
    glGenBuffers(1,&(device_quad.vbo_data));
    glGenBuffers(1,&(device_quad.vbo_indices));

    //Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, device_quad.vbo_data);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    //Use of strided data, Array of Structures instead of Structures of Arrays
    glVertexAttribPointer(quad_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),0);
    glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(vec3));
    glEnableVertexAttribArray(quad_attributes::POSITION);
    glEnableVertexAttribArray(quad_attributes::TEXCOORD);

    //indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);
    device_quad.num_indices = 6;
    //Unplug Vertex Array
    glBindVertexArray(0);
}


GLuint depthTexture = 0;
GLuint normalTexture = 0;
GLuint positionTexture = 0;
GLuint colorTexture = 0;
GLuint postTexture = 0;
GLuint postTexture2 = 0;
GLuint postTexture3 = 0;
GLuint FBO[4] = {0, 0, 0, 0};


GLuint pass_prog;
GLuint point_prog;
GLuint ambient_prog;
GLuint diagnostic_prog;
GLuint post_prog;
GLuint post_prog2;
void initShader() {
    Utility::shaders_t shaders = Utility::loadShaders("../../../res/shaders/pass.vert", "../../../res/shaders/pass.frag");

    pass_prog = glCreateProgram();

    glBindAttribLocation(pass_prog, mesh_attributes::POSITION, "Position");
    glBindAttribLocation(pass_prog, mesh_attributes::NORMAL, "Normal");
    glBindAttribLocation(pass_prog, mesh_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(pass_prog,shaders);

    shaders = Utility::loadShaders("../../../res/shaders/shade.vert", "../../../res/shaders/diagnostic.frag");

    diagnostic_prog = glCreateProgram();

    glBindAttribLocation(diagnostic_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(diagnostic_prog, quad_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(diagnostic_prog, shaders);

    shaders = Utility::loadShaders("../../../res/shaders/shade.vert", "../../../res/shaders/ambient.frag");

    ambient_prog = glCreateProgram();

    glBindAttribLocation(ambient_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(ambient_prog, quad_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(ambient_prog, shaders);

    shaders = Utility::loadShaders("../../../res/shaders/shade.vert", "../../../res/shaders/point.frag");

    point_prog = glCreateProgram();

    glBindAttribLocation(point_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(point_prog, quad_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(point_prog, shaders);

    shaders = Utility::loadShaders("../../../res/shaders/post.vert", "../../../res/shaders/post.frag");

    post_prog = glCreateProgram();

    glBindAttribLocation(post_prog, quad_attributes::POSITION, "Position");
    glBindAttribLocation(post_prog, quad_attributes::TEXCOORD, "Texcoord");

    Utility::attachAndLinkProgram(post_prog, shaders);
}

void freeFBO() {
    glDeleteTextures(1,&depthTexture);
    glDeleteTextures(1,&normalTexture);
    glDeleteTextures(1,&positionTexture);
    glDeleteTextures(1,&colorTexture);
    glDeleteTextures(1,&postTexture);
	glDeleteTextures(1,&postTexture2);
    glDeleteFramebuffers(1,&FBO[0]);
    glDeleteFramebuffers(1,&FBO[1]);
	glDeleteFramebuffers(1,&FBO[2]);
	glDeleteFramebuffers(1,&FBO[3]);
}

void checkFramebufferStatus(GLenum framebufferStatus) {
    switch (framebufferStatus) {
        case GL_FRAMEBUFFER_COMPLETE_EXT: break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
                                          printf("Attachment Point Unconnected\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                                          printf("Missing Attachment\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                                          printf("Dimensions do not match\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
                                          printf("Formats\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
                                          printf("Draw Buffer\n");
                                          break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
                                          printf("Read Buffer\n");
                                          break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                                          printf("Unsupported Framebuffer Configuration\n");
                                          break;
        default:
                                          printf("Unkown Framebuffer Object Failure\n");
                                          break;
    }
}


GLuint random_normal_tex;
GLuint random_scalar_tex;
void initNoise() {  
    random_normal_tex = (unsigned int)SOIL_load_OGL_texture("../../../res/random_normal.png",0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    random_scalar_tex = (unsigned int)SOIL_load_OGL_texture("../../../res/random.png",0,0,0);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void initFBO(int w, int h) {
    GLenum FBOstatus;

    glActiveTexture(GL_TEXTURE9);

    glGenTextures(1, &depthTexture);
    glGenTextures(1, &normalTexture);
    glGenTextures(1, &positionTexture);
    glGenTextures(1, &colorTexture);

    //Set up depth FBO
    glBindTexture(GL_TEXTURE_2D, depthTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    //Set up normal FBO
    glBindTexture(GL_TEXTURE_2D, normalTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    //Set up position FBO
    glBindTexture(GL_TEXTURE_2D, positionTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    //Set up color FBO
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    // creatwwe a framebuffer object
    glGenFramebuffers(1, &FBO[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[0]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_NONE);
    GLint normal_loc = glGetFragDataLocation(pass_prog,"out_Normal");
    GLint position_loc = glGetFragDataLocation(pass_prog,"out_Position");
    GLint color_loc = glGetFragDataLocation(pass_prog,"out_Color");
    GLenum draws [3];
    draws[normal_loc] = GL_COLOR_ATTACHMENT0;
    draws[position_loc] = GL_COLOR_ATTACHMENT1;
    draws[color_loc] = GL_COLOR_ATTACHMENT2;
    glDrawBuffers(3, draws);

    // attach the texture to FBO depth attachment point
    int test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[normal_loc], normalTexture, 0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[position_loc], positionTexture, 0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);    
    glFramebufferTexture(GL_FRAMEBUFFER, draws[color_loc], colorTexture, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[0]\n");
        checkFramebufferStatus(FBOstatus);
    }

    //Post Processing buffer!
    glActiveTexture(GL_TEXTURE9);

    glGenTextures(1, &postTexture);

    //Set up post FBO
    glBindTexture(GL_TEXTURE_2D, postTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

    // creatwwe a framebuffer object
    glGenFramebuffers(1, &FBO[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[1]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_BACK);
    color_loc = glGetFragDataLocation(ambient_prog,"out_Color");
    GLenum draw[1];
    draw[color_loc] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, draw);

    // attach the texture to FBO depth attachment point
    test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, postTexture);
    glFramebufferTexture(GL_FRAMEBUFFER, draw[color_loc], postTexture, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[1]\n");
        checkFramebufferStatus(FBOstatus);
    }

    glGenTextures(1, &postTexture2);

    //Set up post FBO
    glBindTexture(GL_TEXTURE_2D, postTexture2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	// creatwwe a framebuffer object
    glGenFramebuffers(1, &FBO[2]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[2]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_BACK);
    color_loc = glGetFragDataLocation(ambient_prog,"out_Color");
    GLenum drawMore[1];
    drawMore[color_loc] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, drawMore);

    // attach the texture to FBO depth attachment point
    test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, postTexture2);
    glFramebufferTexture(GL_FRAMEBUFFER, drawMore[color_loc], postTexture2, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[2]\n");
        checkFramebufferStatus(FBOstatus);
    }

	glGenTextures(1, &postTexture3);

    //Set up post FBO
    glBindTexture(GL_TEXTURE_2D, postTexture3);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	// creatwwe a framebuffer object
    glGenFramebuffers(1, &FBO[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[3]);

    // Instruct openGL that we won't bind a color texture with the currently bound FBO
    glReadBuffer(GL_BACK);
    color_loc = glGetFragDataLocation(ambient_prog,"out_Color");
    GLenum drawMoreMore[1];
    drawMoreMore[color_loc] = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, drawMoreMore);

    // attach the texture to FBO depth attachment point
    test = GL_COLOR_ATTACHMENT0;
    glBindTexture(GL_TEXTURE_2D, postTexture3);
    glFramebufferTexture(GL_FRAMEBUFFER, drawMoreMore[color_loc], postTexture3, 0);

    // check FBO status
    FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
        printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO[3]\n");
        checkFramebufferStatus(FBOstatus);
    }


    // switch back to window-system-provided framebuffer
    glClear(GL_DEPTH_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void bindFBO(int buf) {
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); //Bad mojo to unbind the framebuffer using the texture
    glBindFramebuffer(GL_FRAMEBUFFER, FBO[buf]);
    glClear(GL_DEPTH_BUFFER_BIT);
    //glColorMask(false,false,false,false);
    glEnable(GL_DEPTH_TEST);
}

void setTextures() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glColorMask(true,true,true,true);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
}



Camera cam(vec3(2.5, 5, 2),
        normalize(vec3(0,-1,0)),
        normalize(vec3(0,0,1)));

    void
Camera::adjust(float dx, // look left right
        float dy, //look up down
        float dz,
        float tx, //strafe left right
        float ty,
        float tz)//go forward) //strafe up down
{

    if (abs(dx) > 0) {
        rx += dx;
        rx = fmod(rx,360.0f);
    }

    if (abs(dy) > 0) {
        ry += dy;
        ry = clamp(ry,-70.0f, 70.0f);
    }

    if (abs(tx) > 0) {
        vec3 dir = glm::gtx::rotate_vector::rotate(start_dir,rx + 90,up);
        vec2 dir2(dir.x,dir.y);
        vec2 mag = dir2 * tx;
        pos += mag;	
    }

    if (abs(ty) > 0) {
        z += ty;
    }

    if (abs(tz) > 0) {
        vec3 dir = glm::gtx::rotate_vector::rotate(start_dir,rx,up);
        vec2 dir2(dir.x,dir.y);
        vec2 mag = dir2 * tz;
        pos += mag;
    }
}

mat4x4 Camera::get_view() {
    vec3 inclin = glm::gtx::rotate_vector::rotate(start_dir,ry,start_left);
    vec3 spun = glm::gtx::rotate_vector::rotate(inclin,rx,up);
    vec3 cent(pos, z);
    return lookAt(cent, cent + spun, up);
}

mat4x4 get_mesh_world() {
    vec3 tilt(1.0f,0.0f,0.0f);
    //mat4 translate_mat = glm::translate(glm::vec3(0.0f,.5f,0.0f));
    mat4 tilt_mat = glm::rotate(mat4(), 90.0f, tilt);
    mat4 scale_mat = glm::scale(mat4(), vec3(0.01));
    return tilt_mat * scale_mat; //translate_mat;
}


float FARP;
float NEARP;
void draw_mesh() {
    FARP = 100.0f;
    NEARP = 0.1f;

    glUseProgram(pass_prog);


    mat4 model = get_mesh_world();
    mat4 view = cam.get_view();
    mat4 persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    mat4 inverse_transposed = transpose(inverse(view*model));

    glUniform1f(glGetUniformLocation(pass_prog, "u_Far"), FARP);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_Model"),1,GL_FALSE,&model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_View"),1,GL_FALSE,&view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_Persp"),1,GL_FALSE,&persp[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pass_prog,"u_InvTrans") ,1,GL_FALSE,&inverse_transposed[0][0]);

    for(int i=0; i<draw_meshes.size(); i++){
		float glow = (draw_meshes[i].meshName.compare("short_block")==0) ? 1.0 : 0.0;
		glUniform1f(glGetUniformLocation(pass_prog, "u_Glow"), glow);
		glUniform1f(glGetUniformLocation(pass_prog, "u_Shininess"), draw_meshes[i].shininess);
        glUniform3fv(glGetUniformLocation(pass_prog, "u_Color"), 1, &(draw_meshes[i].color[0]));
        glBindVertexArray(draw_meshes[i].vertex_array);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_meshes[i].vbo_indices);
        glDrawElements(GL_TRIANGLES, draw_meshes[i].num_indices, GL_UNSIGNED_SHORT,0);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
}


enum Display display_type = DISPLAY_TOTAL;

void setup_quad(GLuint prog)
{
    glUseProgram(prog);
    glEnable(GL_TEXTURE_2D);

    mat4 persp = perspective(45.0f,(float)width/(float)height,NEARP,FARP);
    vec4 test(-2,0,10,1);
    vec4 testp = persp * test;
    vec4 testh = testp / testp.w;
    vec2 coords = vec2(testh.x, testh.y) / 2.0f + 0.5f;
    glUniform1i(glGetUniformLocation(prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(prog, "u_ScreenWidth"), width);
    glUniform1f(glGetUniformLocation(prog, "u_Far"), FARP);
    glUniform1f(glGetUniformLocation(prog, "u_Near"), NEARP);
    glUniform1i(glGetUniformLocation(prog, "u_DisplayType"), display_type);
    glUniformMatrix4fv(glGetUniformLocation(prog, "u_Persp"),1, GL_FALSE, &persp[0][0] );

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Depthtex"),0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Normaltex"),1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Positiontex"),2);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(prog, "u_Colortex"),3);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(prog, "u_RandomScalartex"),5);
}

void draw_quad() {

    glBindVertexArray(device_quad.vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, device_quad.vbo_indices);

    glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT,0);

    glBindVertexArray(0);
}

void draw_light(vec3 pos, vec3 col, float strength, mat4 sc, mat4 vp, float NEARP) {
    float radius = strength;
    vec4 light = cam.get_view() * vec4(pos, 1.0); 
    if( light.z > NEARP)
    {
        return;
    }
    light.w = radius;
	glUniform3fv(glGetUniformLocation(point_prog, "u_LightCol"), 1, &(col[0]));
    glUniform4fv(glGetUniformLocation(point_prog, "u_Light"), 1, &(light[0]));
    glUniform1f(glGetUniformLocation(point_prog, "u_LightIl"), strength);

    vec4 left = vp * vec4(pos + radius*cam.start_left, 1.0);
    vec4 up = vp * vec4(pos + radius*cam.up, 1.0);
    vec4 center = vp * vec4(pos, 1.0);

    left = sc * left;
    up = sc * up;
    center = sc * center;

    left /= left.w;
    up /= up.w;
    center /= center.w;

    float hw = glm::distance(left, center);
    float hh = glm::distance(up, center);

    float r = (hh > hw) ? hh : hw;

    float x = center.x-r;
    float y = center.y-r;

    glScissor(x, y, 2*r, 2*r);
    draw_quad();
}

void updateDisplayText(char * disp) {
    switch(display_type) {
        case(DISPLAY_DEPTH):
            sprintf(disp, "Displaying Depth");
            break; 
        case(DISPLAY_NORMAL):
            sprintf(disp, "Displaying Normal");
            break; 
        case(DISPLAY_COLOR):
            sprintf(disp, "Displaying Color");
            break;
        case(DISPLAY_POSITION):
            sprintf(disp, "Displaying Position");
            break;
        case(DISPLAY_TOTAL):
            sprintf(disp, "Displaying Diffuse");
            break;
        case(DISPLAY_LIGHTS):
            sprintf(disp, "Displaying Lights");
            break;
    }
}

void updateModeText(char * mode) {
	switch(postProcessMode){
		case(POST_PASS):
			sprintf(mode, "Pass Through");
			break;
		case(POST_BLOOM_2D):
			sprintf(mode, "Bloom 2D Kernel");
			break;
		case(POST_BLOOM_SEPARATED):
			sprintf(mode, "Bloom Separated Kernel");
			break;
		case(POST_GLOW_2D):
			sprintf(mode, "Glow 2D Kernel");
			break;
		case(POST_GLOW_SEPARATED):
			sprintf(mode, "Glow Separated Kernel");
			break;
		case(POST_VIGNETTE):
			sprintf(mode, "Vignette Kernel");
			break;
		case(POST_SSAO):
			sprintf(mode, "SSAO: Scalable");
			break;
		case(POST_SSAO_BLEND):
			sprintf(mode, "SSAO: Scalable Blended");
			break;
	}
}

int frame = 0;
int currenttime = 0;
int timebase = 0;
char title[1024];
char disp[1024];
char mode[1024];
char occl[1024];

void updateTitle() {
    updateDisplayText(disp);

	updateModeText(mode);
    //calculate the frames per second
    frame++;

    //get the current time
    currenttime = glutGet(GLUT_ELAPSED_TIME);

    //check if a second has passed
    if (currenttime - timebase > 1000) 
    {
        sprintf(title, "CIS565 OpenGL Frame | %s FPS: %4.2f |%d %s | Blur:%dx%d", disp, frame*1000.0/(currenttime-timebase),postProcessMode, mode, 2*kernelWidthX+1, 2*kernelWidthY+1);
        //sprintf(title, "CIS565 OpenGL Frame | %4.2f FPS", frame*1000.0/(currenttime-timebase));
        glutSetWindowTitle(title);
        timebase = currenttime;		
        frame = 0;
    }
}

void setupPostFrameBuffer(int fboID, GLuint inputTexture, int postProcessEffect)
{
    bindFBO(fboID);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glUseProgram(post_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex"),0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Normaltex"),2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D,positionTexture);
	glUniform1i(glGetUniformLocation(post_prog, "u_Positiontex"),3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomScalartex"),5);

    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenWidth"), width);

	glUniform1i(glGetUniformLocation(post_prog, "u_kernelWidthX"),kernelWidthX);
	glUniform1i(glGetUniformLocation(post_prog, "u_kernelWidthY"),kernelWidthY);
	glUniform1i(glGetUniformLocation(post_prog, "u_Mode"),postProcessEffect);

}

void setupBlendFrameBuffer(int outputFBO, GLuint inputTex1, GLuint inputTex2)
{
    bindFBO(outputFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glUseProgram(post_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex1);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex"),0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, inputTex2);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex2"),1);

	glUniform1i(glGetUniformLocation(post_prog, "u_Mode"),ARG_ADD_BLEND);

}

bool doIScissor = true;
void display(void)
{
    // clear the screen
    bindFBO(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_mesh();

    setTextures();
    bindFBO(1);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glClear(GL_COLOR_BUFFER_BIT);
    if(display_type == DISPLAY_LIGHTS || display_type == DISPLAY_TOTAL)
    {
        setup_quad(point_prog);
        if(doIScissor) glEnable(GL_SCISSOR_TEST);
        mat4 vp = perspective(45.0f,(float)width/(float)height,NEARP,FARP) * 
                  cam.get_view();
        mat4 sc = mat4(width, 0.0,    0.0, 0.0,
                       0.0,   height, 0.0, 0.0,
                       0.0,   0.0,    1.0, 0.0,
                       0.0,   0.0,    0.0, 1.0) *
                  mat4(0.5, 0.0, 0.0, 0.0,
                       0.0, 0.5, 0.0, 0.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.5, 0.5, 0.0, 1.0);
		
		for( float i = 0.1 ; i <= 6.0 ; i+=0.85)
			for(float j = 0.1 ; j <= 6.0 ; j+=0.85)
				for(float k = 0.1 ; k <= 6.0 ; k+=0.85)
					draw_light(vec3(i, -j, k), vec3(1.0,1.0,1.0), 0.75, sc, vp, NEARP);
		/*
		//setupLights();
		for(int l = 0; l < numLights; l++)
		{
			draw_light(lightPos[l],lightCol[l],lightInt[l],sc,vp,NEARP);
		}
		*/
        //draw_light(vec3(2.5, -2.5, 4.0), 3.00, sc, vp, NEARP);

        glDisable(GL_SCISSOR_TEST);
        vec4 dir_light(0.1, 1.0, 1.0, 0.0);
        dir_light = cam.get_view() * dir_light; 
        dir_light = normalize(dir_light);
        dir_light.w = 0.3;
        float strength = 0.09;
        setup_quad(ambient_prog);
        glUniform4fv(glGetUniformLocation(ambient_prog, "u_Light"), 1, &(dir_light[0]));
        glUniform1f(glGetUniformLocation(ambient_prog, "u_LightIl"), strength);
        draw_quad();
    }
    else
    {
        setup_quad(diagnostic_prog);
        draw_quad();
    }
    glDisable(GL_BLEND);

    setTextures();

	GLuint finalTexture = postTexture;
	GLuint finalBlendTexture = postTexture3;
	int finalPostProcess = ARG_VIGNETTE;

	switch(postProcessMode)
	{
		case POST_PASS:
			finalPostProcess = ARG_PASS;
			break;
		case POST_BLOOM_2D:
			finalPostProcess = ARG_BLOOM_2D;
			break;
		case POST_GLOW_2D:
			finalPostProcess = ARG_GLOW_2D;
			break;
		case POST_VIGNETTE:
			finalPostProcess = ARG_VIGNETTE;
			break;
		case POST_BLOOM_SEPARATED:
			setupPostFrameBuffer(2,postTexture,ARG_BLOOM_SELECT);
			draw_quad();
			setupPostFrameBuffer(3,postTexture2,ARG_BLUR_X);
			draw_quad();
			setupPostFrameBuffer(2,postTexture3,ARG_BLUR_Y);
			draw_quad();
			finalPostProcess = ARG_ADD_BLEND;
			finalTexture = postTexture;
			finalBlendTexture = postTexture2;
			break;
		case POST_GLOW_SEPARATED:
			setupPostFrameBuffer(2,postTexture,ARG_GLOW_SELECT);
			draw_quad();
			setupPostFrameBuffer(3,postTexture2,ARG_BLUR_X);
			draw_quad();
			setupPostFrameBuffer(2,postTexture3,ARG_BLUR_Y);
			draw_quad();
			finalPostProcess = ARG_ADD_BLEND;
			finalTexture = postTexture;
			finalBlendTexture = postTexture2;
			break;
		case POST_SSAO:
			finalPostProcess = ARG_SSAO_SCALE;
			break;
		case POST_SSAO_BLEND:
			setupPostFrameBuffer(2,postTexture,ARG_SSAO_SCALE);
			draw_quad();
			finalPostProcess = ARG_MULT_BLEND;
			finalBlendTexture = postTexture2;
			break;
	}	
	
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glUseProgram(post_prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex"),0);
    
	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, finalBlendTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Posttex2"),1);

	glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(post_prog, "u_Normaltex"),2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D,positionTexture);
	glUniform1i(glGetUniformLocation(post_prog, "u_Positiontex"),3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, random_normal_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomNormaltex"),4);
    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, random_scalar_tex);
    glUniform1i(glGetUniformLocation(post_prog, "u_RandomScalartex"),5);

    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenHeight"), height);
    glUniform1i(glGetUniformLocation(post_prog, "u_ScreenWidth"), width);

	glUniform1i(glGetUniformLocation(post_prog, "u_kernelWidthX"),kernelWidthX);
	glUniform1i(glGetUniformLocation(post_prog, "u_kernelWidthY"),kernelWidthY);
	glUniform1i(glGetUniformLocation(post_prog, "u_Mode"),finalPostProcess);

    draw_quad();
	

    glEnable(GL_DEPTH_TEST);
    updateTitle();

    glutPostRedisplay();
    glutSwapBuffers();
}



void reshape(int w, int h)
{
    width = w;
    height = h;
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,(GLsizei)w,(GLsizei)h);
    if (FBO[0] != 0 || depthTexture != 0 || normalTexture != 0 ) {
        freeFBO();
    }
    initFBO(w,h);
}


int mouse_buttons = 0;
int mouse_old_x = 0;
int mouse_old_y = 0;
void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        mouse_buttons |= 1<<button;
    } else if (state == GLUT_UP) {
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void motion(int x, int y)
{
    float dx, dy;
    dx = (float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);

    if (mouse_buttons & 1<<GLUT_RIGHT_BUTTON) {
        cam.adjust(0,0,dx,0,0,0);;
    }
    else {
        cam.adjust(-dx*0.2f,-dy*0.2f,0,0,0,0);
    }

    mouse_old_x = x;
    mouse_old_y = y;
}

void specialKeyboard(int key, int x, int y){
	
	switch(key)
	{
		case GLUT_KEY_UP:	kernelWidthY++;
							break;
		case GLUT_KEY_DOWN:	kernelWidthY += kernelWidthY > 0? -1 : 0;
							break;
		case GLUT_KEY_RIGHT:kernelWidthX++;
							break;
		case GLUT_KEY_LEFT:	kernelWidthX += kernelWidthX > 0? -1 : 0;
							break;
	}

}

void keyboard(unsigned char key, int x, int y) {
    float tx = 0;
    float ty = 0;
    float tz = 0;
    switch(key) {
        case(27):
            exit(0.0);
            break;
        case('w'):
            tz = 0.3;
            break;
        case('s'):
            tz = -0.3;
            break;
        case('d'):
            tx = -0.3;
            break;
        case('a'):
            tx = 0.3;
            break;
        case('q'):
            ty = 0.3;
            break;
        case('z'):
            ty = -0.3;
            break;
        case('1'):
            display_type = DISPLAY_DEPTH;
            break;
        case('2'):
            display_type = DISPLAY_NORMAL;
            break;
        case('3'):
            display_type = DISPLAY_COLOR;
            break;
        case('4'):
            display_type = DISPLAY_POSITION;
            break;
        case('5'):
            display_type = DISPLAY_LIGHTS;
            break;
        case('0'):
            display_type = DISPLAY_TOTAL;
            break;
		case(']'):
			postProcessMode = (postProcessMode + 1) % POST_COUNT;
			break;
		case('['):
			postProcessMode = postProcessMode > 1 ? (postProcessMode - 1) : 0;
			break;
        case('x'):
            doIScissor ^= true;
            break;
        case('r'):
            initShader();
            break;
    }

    if (abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0) {
        cam.adjust(0,0,0,tx,ty,tz);
    }
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f,1.0f);
}

int main (int argc, char* argv[])
{

	srand (time(NULL));
	setupLights();

    bool loadedScene = false;
    for(int i=1; i<argc; i++){
        string header; string data;
        istringstream liness(argv[i]);
        getline(liness, header, '='); getline(liness, data, '=');
        if(strcmp(header.c_str(), "mesh")==0){
            int found = data.find_last_of("/\\");
            string path = data.substr(0,found+1);
            cout << "Loading: " << data << endl;
            string err = tinyobj::LoadObj(shapes, data.c_str(), path.c_str());
            if(!err.empty())
            {
                cerr << err << endl;
                return -1;
            }
            loadedScene = true;
        }
    }

    if(!loadedScene){
        cout << "Usage: mesh=[obj file]" << endl; 
        std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
        return 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    width = 1280;
    height = 720;
    glutInitWindowSize(width,height);
    glutCreateWindow("CIS565 OpenGL Frame");
    glewInit();
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        cout << "glewInit failed, aborting." << endl;
        exit (1);
    }
    cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;
    cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

    initNoise();
    initShader();
    initFBO(width,height);
    init();
    initMesh();
    initQuad();


    glutDisplayFunc(display);
    glutReshapeFunc(reshape);	
    glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}

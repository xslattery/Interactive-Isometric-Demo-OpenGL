//
//  shader.cpp
//  RTSProject
//
//  Created by Xavier Slattery.
//  Copyright © 2015 Xavier Slattery. All rights reserved.
//

#include <OpenGL/gl3.h>
#include <glm/glm.hpp>

#include <fstream>
#include <string>
#include <vector>

#include "debug.hpp"
#include "shader.hpp"

unsigned int LoadShaders( const char * vertex_file_path, const char * fragment_file_path ) {
    
    int Result = 0;
    int InfoLogLength;

    // Create the shaders:
    unsigned int VertexShaderID = glCreateShader( GL_VERTEX_SHADER );
    // Read the Vertex Shader code from the file:
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream( vertex_file_path, std::ios::in );
    if( VertexShaderStream.is_open() ) {
        std::string Line = "";
        while( getline( VertexShaderStream, Line ) )
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    } else {
        ERROR( "Impossible to open. Are you in the right directory ? : " << vertex_file_path << "\n" );
        return 0;
    }
    // Compile Vertex Shader
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource( VertexShaderID, 1, &VertexSourcePointer , NULL );
    glCompileShader( VertexShaderID );
    // Check Vertex Shader
    glGetShaderiv( VertexShaderID, GL_COMPILE_STATUS, &Result );
    glGetShaderiv( VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength );
    if ( InfoLogLength > 0 ) {
        std::vector<char> VertexShaderErrorMessage( InfoLogLength+1 );
        glGetShaderInfoLog( VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0] );
        ERROR( &VertexShaderErrorMessage[0] << "\n" );
    }

    unsigned int FragmentShaderID = glCreateShader( GL_FRAGMENT_SHADER );
    // Read the Fragment Shader code from the file:
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream( fragment_file_path, std::ios::in );
    if( FragmentShaderStream.is_open() ){
        std::string Line = "";
        while( getline(FragmentShaderStream, Line) )
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    } else {
        ERROR( "Impossible to open. Are you in the right directory ? : " << fragment_file_path << "\n" );
        return 0;
    }
    // Compile Fragment Shader:
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource( FragmentShaderID, 1, &FragmentSourcePointer , NULL );
    glCompileShader( FragmentShaderID );
    // Check Fragment Shader:
    glGetShaderiv( FragmentShaderID, GL_COMPILE_STATUS, &Result );
    glGetShaderiv( FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength );
    if ( InfoLogLength > 0 ){
        std::vector<char> FragmentShaderErrorMessage( InfoLogLength+1 );
        glGetShaderInfoLog( FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0] );
        ERROR( &FragmentShaderErrorMessage[0] << "\n" );
    }
    
    // Link the program:
    unsigned int ProgramID = glCreateProgram();    
    glAttachShader( ProgramID, VertexShaderID );
    glAttachShader( ProgramID, FragmentShaderID );
    glLinkProgram( ProgramID );
    
    // Check the program:
    glGetProgramiv( ProgramID, GL_LINK_STATUS, &Result );
    glGetProgramiv( ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength );
    if ( InfoLogLength > 0 ) {
        std::vector<char> ProgramErrorMessage( InfoLogLength+1 );
        glGetProgramInfoLog( ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0] );
        std::cout << &ProgramErrorMessage[0] << "\n";
        std::cout << "Shader failed to be created.\n";
    }
    
    glDetachShader( ProgramID, VertexShaderID );
    glDetachShader( ProgramID, FragmentShaderID );
    
    glDeleteShader( VertexShaderID );
    glDeleteShader( FragmentShaderID );
    
    return ProgramID;
}

// Getting Uniforms:
GLint getUniformLocation(const GLint programID, const GLchar* name) {
    return glGetUniformLocation(programID, name);
}

// Setting Uniforms:
void setUniform1f(const GLint programID, const GLchar* name, float value) {
    glUniform1f(getUniformLocation(programID, name), value);
}
void setUniform1fv(const GLint programID, const GLchar* name, float* value, int count) {
    glUniform1fv(getUniformLocation(programID, name), count, value);
}
void setUniform1i(const GLint programID, const GLchar* name, int value) {
    glUniform1i(getUniformLocation(programID, name), value);
}
void setUniform1iv(const GLint programID, const GLchar* name, int* value, int count) {
    glUniform1iv(getUniformLocation(programID, name), count, value);
}
void setUniform2f(const GLint programID, const GLchar* name, const glm::vec2& vector) {
    glUniform2f(getUniformLocation(programID, name), vector.x, vector.y);
}
void setUniform3f(const GLint programID, const GLchar* name, const glm::vec3& vector) {
    glUniform3f(getUniformLocation(programID, name), vector.x, vector.y, vector.z);
}
void setUniform4f(const GLint programID, const GLchar* name, const glm::vec4& vector) {
    glUniform4f(getUniformLocation(programID, name), vector.x, vector.y, vector.z, vector.w);
}
void setUniformMat4(const GLint programID, const GLchar* name, const glm::mat4& matrix) {
    glUniformMatrix4fv(getUniformLocation(programID, name), 1, GL_FALSE, &(matrix[0][0]) );
}





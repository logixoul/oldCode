#if 0
// from https://github.com/dli/filtering , ported to c++

#include "precompiled.h"
#include <string>
using namespace std;
GLuint PING_TEXTURE_UNIT = 0,
    PONG_TEXTURE_UNIT = 1,
    FILTER_TEXTURE_UNIT = 2,
    ORIGINAL_SPECTRUM_TEXTURE_UNIT = 3,
    FILTERED_SPECTRUM_TEXTURE_UNIT = 4,
    IMAGE_TEXTURE_UNIT = 5,
    FILTERED_IMAGE_TEXTURE_UNIT = 6,
    READOUT_TEXTURE_UNIT = 7;

int RESOLUTION = 512;

int FORWARD = 0,
    INVERSE = 1;

int log2(float x) {
    return log(x) / log(2.0f);
};

GLuint buildShader(GLuint type, string source) {
    auto shader = glCreateShader(type);
	const char* sources[1] = { source.c_str() };
	int lens[1] = { source.size() };
    glShaderSource(shader, 1, sources, lens);
    glCompileShader(shader);
    //std::cout << (glGetShaderInfoLog(shader)) << std::endl;
    return shader;
};

struct ProgramWrapper {
	GLuint program;

};

void buildProgramWrapper(GLuint vertexShader, GLuint fragmentShader) {
    ProgramWrapper programWrapper;

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glBindAttribLocation(program, 0, "a_position");
    glLinkProgram(program);
    var uniformLocations = {};
	auto numberOfUniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, );
    for (var i = 0; i < numberOfUniforms; i += 1) {
        GLuint activeUniform = glGetActiveUniform(program, i);
        GLuint uniformLocation = glgetUniformLocation(program, activeUniform.name);
        uniformLocations[activeUniform.name] = uniformLocation;
    }

    programWrapper.program = program;
    programWrapper.uniformLocations = uniformLocations;

    return programWrapper;
};

var buildTexture = function (gl, unit, format, type, width, height, data, wrapS, wrapT, minFilter, magFilter) {
    var texture = glcreateTexture();
    glactiveTexture(glTEXTURE0 + unit);
    glbindTexture(glTEXTURE_2D, texture);
    gltexImage2D(glTEXTURE_2D, 0, format, width, height, 0, format, type, data);
    gltexParameteri(glTEXTURE_2D, glTEXTURE_WRAP_S, wrapS);
    gltexParameteri(glTEXTURE_2D, glTEXTURE_WRAP_T, wrapT);
    gltexParameteri(glTEXTURE_2D, glTEXTURE_MIN_FILTER, minFilter);
    gltexParameteri(glTEXTURE_2D, glTEXTURE_MAG_FILTER, magFilter);
    return texture;
};

var buildFramebuffer = function (gl, attachment) {
    var framebuffer = glcreateFramebuffer();
    glbindFramebuffer(glFRAMEBUFFER, framebuffer);
    glframebufferTexture2D(glFRAMEBUFFER, glCOLOR_ATTACHMENT0, glTEXTURE_2D, attachment, 0);
    return framebuffer;
};

var getMousePosition = function (event, element) {
    var boundingRect = element.getBoundingClientRect();
    return {
        x: event.clientX - boundingRect.left,
        y: event.clientY - boundingRect.top
    };
};

var clamp = function (x, min, max) {
    return Math.min(Math.max(x, min), max);
};

var averageArray = function (array) {
    var sum = 0;
    for (var i = 0; i < array.length; i += 1) {
        sum += array[i];
    }
    return sum / array.length;
};

var hasWebGLSupportWithExtensions = function (extensions) {
    var canvas = document.createElement('canvas');
    var gl = null;
    try {
        gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
    } catch (e) {
        return false;
    }
    if (gl === null) {
        return false;
    }

    for (var i = 0; i < extensions.length; ++i) {
        if (glgetExtension(extensions[i]) === null) {
            return false
        }
    }

    return true;
};

var mod = function (x, n) { //positive modulo
    var m = x % n;
    return m < 0 ? m + n : m;
};
string FULLSCREEN_VERTEX_SOURCE =
    "attribute vec2 a_position;"
    "varying vec2 v_coordinates;" //this might be phased out soon (no pun intended)
    "void main (void) {"
        "v_coordinates = a_position * 0.5 + 0.5;"
        "gl_Position = vec4(a_position 0.0 1.0);"
    "}"
;

string SUBTRANSFORM_FRAGMENT_SOURCE =
    "precision highp float;"

    "const float PI = 3.14159265;"

    "uniform sampler2D u_input;"

    "uniform float u_resolution;"
    "uniform float u_subtransformSize;"

    "uniform bool u_horizontal;"
    "uniform bool u_forward;"
    "uniform bool u_normalize;"

    "vec2 multiplyComplex (vec2 a vec2 b) {"
        "return vec2(a[0] * b[0] - a[1] * b[1] a[1] * b[0] + a[0] * b[1]);"
    "}"

    "void main (void) {"

        "float index = 0.0;"
        "if (u_horizontal) {"
            "index = gl_FragCoord.x - 0.5;"
        "} else {"
            "index = gl_FragCoord.y - 0.5;"
        "}"

        "float evenIndex = floor(index / u_subtransformSize) * (u_subtransformSize / 2.0) + mod(index u_subtransformSize / 2.0);"
        
        "vec4 even = vec4(0.0) odd = vec4(0.0);"

        "if (u_horizontal) {"
            "even = texture2D(u_input vec2(evenIndex + 0.5 gl_FragCoord.y) / u_resolution);"
            "odd = texture2D(u_input vec2(evenIndex + u_resolution * 0.5 + 0.5 gl_FragCoord.y) / u_resolution);"
        "} else {"
            "even = texture2D(u_input vec2(gl_FragCoord.x evenIndex + 0.5) / u_resolution);"
            "odd = texture2D(u_input vec2(gl_FragCoord.x evenIndex + u_resolution * 0.5 + 0.5) / u_resolution);"
        "}"

        //normalisation
        "if (u_normalize) {"
            "even /= u_resolution * u_resolution;"
            "odd /= u_resolution * u_resolution;"
        "}"

        "float twiddleArgument = 0.0;"
        "if (u_forward) {"
            "twiddleArgument = 2.0 * PI * (index / u_subtransformSize);"
        "} else {"
            "twiddleArgument = -2.0 * PI * (index / u_subtransformSize);"
        "}"
        "vec2 twiddle = vec2(cos(twiddleArgument) sin(twiddleArgument));"

        "vec2 outputA = even.rg + multiplyComplex(twiddle odd.rg);"
        "vec2 outputB = even.ba + multiplyComplex(twiddle odd.ba);"

        "gl_FragColor = vec4(outputA outputB);"

    "}"
;

string FILTER_FRAGMENT_SOURCE =
    "precision highp float;"

    "uniform sampler2D u_input;"
    "uniform float u_resolution;"

    "uniform float u_maxEditFrequency;"

    "uniform sampler2D u_filter;"

    "void main (void) {"
        "vec2 coordinates = gl_FragCoord.xy - 0.5;"
        "float xFrequency = (coordinates.x < u_resolution * 0.5) ? coordinates.x : coordinates.x - u_resolution;"
        "float yFrequency = (coordinates.y < u_resolution * 0.5) ? coordinates.y : coordinates.y - u_resolution;"

        "float frequency = sqrt(xFrequency * xFrequency + yFrequency * yFrequency);"

        "float gain = texture2D(u_filter vec2(frequency / u_maxEditFrequency 0.5)).a;"
        "vec4 originalPower = texture2D(u_input gl_FragCoord.xy / u_resolution);"

        "gl_FragColor = originalPower * gain;"

    "}"
;

string POWER_FRAGMENT_SOURCE =
    "precision highp float;"

    "varying vec2 v_coordinates;"

    "uniform sampler2D u_spectrum;"
    "uniform float u_resolution;"

    "vec2 multiplyByI (vec2 z) {"
        "return vec2(-z[1] z[0]);"
    "}"

    "vec2 conjugate (vec2 z) {"
        "return vec2(z[0] -z[1]);"
    "}"

    "vec4 encodeFloat (float v) {" //hack because WebGL cannot read back floats
        "vec4 enc = vec4(1.0 255.0 65025.0 160581375.0) * v;"
        "enc = fract(enc);"
        "enc -= enc.yzww * vec4(1.0 / 255.0 1.0 / 255.0 1.0 / 255.0 0.0);"
        "return enc;"
    "}"

    "void main (void) {"
        "vec2 coordinates = v_coordinates - 0.5;"

        "vec4 z = texture2D(u_spectrum coordinates);"
        "vec4 zStar = texture2D(u_spectrum 1.0 - coordinates + 1.0 / u_resolution);"
        "zStar = vec4(conjugate(zStar.xy) conjugate(zStar.zw));"

        "vec2 r = 0.5 * (z.xy + zStar.xy);"
        "vec2 g = -0.5 * multiplyByI(z.xy - zStar.xy);"
        "vec2 b = z.zw;"

        "float rPower = length(r);"
        "float gPower = length(g);"
        "float bPower = length(b);"

        "float averagePower = (rPower + gPower + bPower) / 3.0;"
        "gl_FragColor = encodeFloat(averagePower / (u_resolution * u_resolution));"
    "}"
;

string IMAGE_FRAGMENT_SOURCE =
    "precision highp float;"

    "varying vec2 v_coordinates;"

    "uniform float u_resolution;"

    "uniform sampler2D u_texture;"
    "uniform sampler2D u_spectrum;"

    "void main (void) {"
        "vec3 image = texture2D(u_texture v_coordinates).rgb;"

        "gl_FragColor = vec4(image 1.0);"
    "}"
;

var Filterer = function (canvas) {
    var canvas = canvas;
    var gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");

    canvas.width = RESOLUTION;
    canvas.height = RESOLUTION;

    glgetExtension("OES_texture_float");
    glpixelStorei(glUNPACK_FLIP_Y_WEBGL, true);

    var imageTexture,
        pingTexture = buildTexture(gl, PING_TEXTURE_UNIT, glRGBA, glFLOAT, RESOLUTION, RESOLUTION, null, glCLAMP_TO_EDGE, glCLAMP_TO_EDGE, glNEAREST, glNEAREST),
        pongTexture = buildTexture(gl, PONG_TEXTURE_UNIT, glRGBA, glFLOAT, RESOLUTION, RESOLUTION, null, glCLAMP_TO_EDGE, glCLAMP_TO_EDGE, glNEAREST, glNEAREST),
        filterTexture = buildTexture(gl, FILTER_TEXTURE_UNIT, glRGBA, glFLOAT, RESOLUTION, 1, null, glCLAMP_TO_EDGE, glCLAMP_TO_EDGE, glNEAREST, glNEAREST),
        originalSpectrumTexture = buildTexture(gl, ORIGINAL_SPECTRUM_TEXTURE_UNIT, glRGBA, glFLOAT, RESOLUTION, RESOLUTION, null, glREPEAT, glREPEAT, glNEAREST, glNEAREST),
        filteredSpectrumTexture = buildTexture(gl, FILTERED_SPECTRUM_TEXTURE_UNIT, glRGBA, glFLOAT, RESOLUTION, RESOLUTION, null, glREPEAT, glREPEAT, glNEAREST, glNEAREST),
        filteredImageTexture = buildTexture(gl, FILTERED_IMAGE_TEXTURE_UNIT, glRGBA, glFLOAT, RESOLUTION, RESOLUTION, null, glCLAMP_TO_EDGE, glCLAMP_TO_EDGE, glNEAREST, glNEAREST),
        readoutTexture = buildTexture(gl, READOUT_TEXTURE_UNIT, glRGBA, glUNSIGNED_BYTE, RESOLUTION, RESOLUTION, null, glCLAMP_TO_EDGE, glCLAMP_TO_EDGE, glNEAREST, glNEAREST);

    var pingFramebuffer = buildFramebuffer(gl, pingTexture),
        pongFramebuffer = buildFramebuffer(gl, pongTexture),
        originalSpectrumFramebuffer = buildFramebuffer(gl, originalSpectrumTexture),
        filteredSpectrumFramebuffer = buildFramebuffer(gl, filteredSpectrumTexture),
        filteredImageFramebuffer = buildFramebuffer(gl, filteredImageTexture),
        readoutFramebuffer = buildFramebuffer(gl, readoutTexture);

    var fullscreenVertexShader = buildShader(gl, glVERTEX_SHADER, FULLSCREEN_VERTEX_SOURCE);

    var subtransformProgramWrapper = buildProgramWrapper(gl, 
        fullscreenVertexShader,
        buildShader(gl, glFRAGMENT_SHADER, SUBTRANSFORM_FRAGMENT_SOURCE), {
        "a_position": 0
    });
    gluseProgram(subtransformProgramWrapper.program);
    gluniform1f(subtransformProgramWrapper.uniformLocations["u_resolution"], RESOLUTION);

    var readoutProgram = buildProgramWrapper(gl, 
        fullscreenVertexShader,
        buildShader(gl, glFRAGMENT_SHADER, POWER_FRAGMENT_SOURCE), {
        "a_position": 0
    });
    gluseProgram(readoutProgram.program);
    gluniform1i(readoutProgram.uniformLocations["u_spectrum"], ORIGINAL_SPECTRUM_TEXTURE_UNIT);
    gluniform1f(readoutProgram.uniformLocations["u_resolution"], RESOLUTION);

    var imageProgram = buildProgramWrapper(gl, 
        fullscreenVertexShader,
        buildShader(gl, glFRAGMENT_SHADER, IMAGE_FRAGMENT_SOURCE), {
        "a_position": 0
    });
    gluseProgram(imageProgram.program);
    gluniform1i(imageProgram.uniformLocations["u_texture"], FILTERED_IMAGE_TEXTURE_UNIT);

    var filterProgram = buildProgramWrapper(gl, 
        fullscreenVertexShader,
        buildShader(gl, glFRAGMENT_SHADER, FILTER_FRAGMENT_SOURCE), {
        "a_position": 0
    });
    gluseProgram(filterProgram.program);
    gluniform1i(filterProgram.uniformLocations["u_input"], ORIGINAL_SPECTRUM_TEXTURE_UNIT);
    gluniform1i(filterProgram.uniformLocations["u_filter"], FILTER_TEXTURE_UNIT);
    gluniform1f(filterProgram.uniformLocations["u_resolution"], RESOLUTION);
    gluniform1f(filterProgram.uniformLocations["u_maxEditFrequency"], END_EDIT_FREQUENCY);

    var fullscreenVertexBuffer = glcreateBuffer();
    glbindBuffer(glARRAY_BUFFER, fullscreenVertexBuffer);
    glbufferData(glARRAY_BUFFER, new Float32Array([-1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0]), glSTATIC_DRAW);
    glvertexAttribPointer(0, 2, glFLOAT, false, 0, 0);
    glenableVertexAttribArray(0);

    var iterations = log2(RESOLUTION) * 2;

    var fft = function (inputTextureUnit, outputFramebuffer, width, height, direction) {
        gluseProgram(subtransformProgramWrapper.program);
        glviewport(0, 0, RESOLUTION, RESOLUTION);
        gluniform1i(subtransformProgramWrapper.uniformLocations["u_horizontal"], 1);
        gluniform1i(subtransformProgramWrapper.uniformLocations["u_forward"], direction === FORWARD ? 1 : 0);
        for (var i = 0; i < iterations; i += 1) {
            if (i === 0) {
                glbindFramebuffer(glFRAMEBUFFER, pingFramebuffer);
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_input"], inputTextureUnit);
            } else if (i === iterations - 1) {
                glbindFramebuffer(glFRAMEBUFFER, outputFramebuffer);
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_input"], PING_TEXTURE_UNIT);
            } else if (i % 2 === 1) {
                glbindFramebuffer(glFRAMEBUFFER, pongFramebuffer);
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_input"], PING_TEXTURE_UNIT);
            } else {
                glbindFramebuffer(glFRAMEBUFFER, pingFramebuffer);
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_input"], PONG_TEXTURE_UNIT);
            }

            if (direction === INVERSE && i === 0) {
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_normalize"], 1);
            } else if (direction === INVERSE && i === 1) {
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_normalize"], 0);
            }

            if (i === iterations / 2) {
                gluniform1i(subtransformProgramWrapper.uniformLocations["u_horizontal"], 0);
            }

            gluniform1f(subtransformProgramWrapper.uniformLocations["u_subtransformSize"], Math.pow(2, (i % (iterations / 2)) + 1));
            gldrawArrays(glTRIANGLE_STRIP, 0, 4);
        }
    };

    this.setImage = function (image) {
        glactiveTexture(glTEXTURE0 + IMAGE_TEXTURE_UNIT);
        imageTexture = glcreateTexture();
        glbindTexture(glTEXTURE_2D, imageTexture);
        gltexImage2D(glTEXTURE_2D, 0, glRGB, glRGB, glUNSIGNED_BYTE, image);
        gltexParameteri(glTEXTURE_2D, glTEXTURE_WRAP_S, glCLAMP_TO_EDGE);
        gltexParameteri(glTEXTURE_2D, glTEXTURE_WRAP_T, glCLAMP_TO_EDGE);
        gltexParameteri(glTEXTURE_2D, glTEXTURE_MIN_FILTER, glNEAREST);
        gltexParameteri(glTEXTURE_2D, glTEXTURE_MAG_FILTER, glNEAREST);

        glactiveTexture(glTEXTURE0 + ORIGINAL_SPECTRUM_TEXTURE_UNIT);
        gltexImage2D(glTEXTURE_2D, 0, glRGBA, RESOLUTION, RESOLUTION, 0, glRGBA, glFLOAT, null);

        fft(IMAGE_TEXTURE_UNIT, originalSpectrumFramebuffer, RESOLUTION, RESOLUTION, FORWARD);

        output();

        glviewport(0, 0, RESOLUTION, RESOLUTION);

        glbindFramebuffer(glFRAMEBUFFER, readoutFramebuffer);
        gluseProgram(readoutProgram.program);
        gldrawArrays(glTRIANGLE_STRIP, 0, 4);

        var pixels = new Uint8Array(RESOLUTION * RESOLUTION * 4);
        glreadPixels(0, 0, RESOLUTION, RESOLUTION, glRGBA, glUNSIGNED_BYTE, pixels);

        var powersByFrequency = {};

        var pixelIndex = 0;
        for (var yIndex = 0; yIndex < RESOLUTION; yIndex += 1) {
           for (var xIndex = 0; xIndex < RESOLUTION; xIndex += 1) {
                var x = xIndex - RESOLUTION / 2,
                    y = yIndex - RESOLUTION / 2;

                var r = pixels[pixelIndex] / 255,
                    g = pixels[pixelIndex + 1] / 255,
                    b = pixels[pixelIndex + 2] / 255,
                    a = pixels[pixelIndex + 3] / 255;

                var average = r + g / 255 + b / 65025 + a / 160581375; //unpack float from rgb

                var frequency = Math.sqrt(x * x + y * y);

                if (powersByFrequency[frequency] === undefined) {
                    powersByFrequency[frequency] = [];
                }
                powersByFrequency[frequency].push(average);

                pixelIndex += 4;
            }
        }

        return powersByFrequency;
    };

    this.filter = function (filterArray) {
        glactiveTexture(glTEXTURE0 + FILTER_TEXTURE_UNIT);
        gltexImage2D(glTEXTURE_2D, 0, glALPHA, filterArray.length, 1, 0, glALPHA, glFLOAT, filterArray);

        gluseProgram(filterProgram.program);

        glbindFramebuffer(glFRAMEBUFFER, filteredSpectrumFramebuffer);
        glviewport(0, 0, RESOLUTION, RESOLUTION);
        gldrawArrays(glTRIANGLE_STRIP, 0, 4);

        fft(FILTERED_SPECTRUM_TEXTURE_UNIT, filteredImageFramebuffer, RESOLUTION, RESOLUTION, INVERSE);

        output();
    };

    var output = function () {
        glbindFramebuffer(glFRAMEBUFFER, null);

        glviewport(0, 0, RESOLUTION, RESOLUTION);
        gluseProgram(imageProgram.program);
        gluniform1f(imageProgram.uniformLocations["u_resolution"], RESOLUTION);
        gluniform1i(imageProgram.uniformLocations["u_texture"], FILTERED_IMAGE_TEXTURE_UNIT);
        gluniform1i(imageProgram.uniformLocations["u_spectrum"], FILTERED_SPECTRUM_TEXTURE_UNIT);
        gldrawArrays(glTRIANGLE_STRIP, 0, 4);
    };
};
#endif
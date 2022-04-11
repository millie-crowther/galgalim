let cubeRotation = 0.0;
let instanceID = null;
let playerID = null;
let model = null;

function sendJSONRequest(method, path, payload) {
    var request = new XMLHttpRequest();
    var json_string = JSON.stringify(payload);
    request.open(method, `${document.baseURI}${path}`);
    request.send(json_string);
}

function sendKeyEvent(event_type, key) {
    sendJSONRequest("POST", "event", {
        instance_id: instanceID,
        player_id: playerID,
        name: "keyboard",
        type: event_type,
        key: key,
    });
}

class Buffer {
    constructor(arrayBuffer) {
        this.arrayBuffer = arrayBuffer;
        this.buffers = {};
    }

    static async create(buffer) {
        let uri;
        if (buffer.uri.startsWith("data:application/octet-stream;base64,")) {
            uri = buffer.uri;
        } else {
            // TODO
        }
        let response = await fetch(uri);
        let arrayBuffer = await response.arrayBuffer();
        return new Buffer(arrayBuffer);
    }

    bind(gl, target) {
        if (!this.buffers[target]) {
            this.buffers[target] = gl.createBuffer();
            gl.bindBuffer(target, this.buffers[target]);
            gl.bufferData(target, this.arrayBuffer, gl.STATIC_DRAW);
        }
        gl.bindBuffer(target, this.buffers[target]);
        return this.buffers[target];
    }
}

class BufferView {
    constructor(bufferView, buffers) {
        this.buffer = buffers[bufferView.buffer];
        this.byteLength = bufferView.byteLength;
        this.byteOffset = bufferView.byteOffset || 0;
    }
}

class Accessor {
    constructor(accessor, bufferViews) {
        this.bufferView = bufferViews[accessor.bufferView];
        this.componentType = accessor.componentType;
        this.type = accessor.type;
        this.count = accessor.count;
        this.min = accessor.min || null;
        this.max = accessor.max || null;

        this.elementSize = {
            SCALAR: 1,
            VEC2: 2,
            VEC3: 3,
            VEC4: 4,
            MAT2: 4,
            MAT3: 9,
            MAT4: 16,
        }[this.type];

        // this.componentSize = {
        //     5120: 1, // signed byte
        //     5121: 1, // unsigned byte
        //     5122: 2, // signed short
        //     5123: 2, // unsigned short
        //     5125: 4, // unsigned int
        //     5126: 4, // float
        // }[this.componentType];
    }

    bindAttribute(gl, position) {
        gl.enableVertexAttribArray(position);
        this.bufferView.buffer.bind(gl, gl.ARRAY_BUFFER);
        gl.vertexAttribPointer(
            position,
            this.elementSize,
            this.componentType,
            false,
            0,
            this.bufferView.byteOffset
        );
    }
}

class Primitive {
    constructor(gl, primitive, accessors, materials) {
        this.indicesAccessor = accessors[primitive.indices] || null;
        this.material = materials[primitive.material];
        this.mode = primitive.mode || gl.TRIANGLES;

        this.attributeAccessors = {};
        for (let key in primitive.attributes) {
            this.attributeAccessors[key] = accessors[primitive.attributes[key]];
        }
    }

    render(gl, uniforms) {
        if (this.material) {
            //         applyTexture(gl, material.baseColorTexture, 0, uniforms.baseColorTexture, uniforms.hasBaseColorTexture);
            //         applyTexture(gl, material.metallicRoughnessTexture, 1, uniforms.metallicRoughnessTexture, uniforms.hasMetallicRoughnessTexture);
            //         applyTexture(gl, material.emissiveTexture, 2, uniforms.emissiveTexture, uniforms.hasEmissiveTexture);
            //         applyTexture(gl, material.normalTexture, 3, uniforms.normalTexture, uniforms.hasNormalTexture);
            //         applyTexture(gl, material.occlusionTexture, 4, uniforms.occlusionTexture, uniforms.hasOcclusionTexture);
            //         gl.uniform4f(uniforms.baseColorFactor, material.baseColorFactor[0], material.baseColorFactor[1], material.baseColorFactor[2], material.baseColorFactor[3]);
            //         gl.uniform1f(uniforms.metallicFactor, material.metallicFactor);
            //         gl.uniform1f(uniforms.roughnessFactor, material.roughnessFactor);
            //         gl.uniform3f(uniforms.emissiveFactor, material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]);
        }
        this.attributeAccessors.POSITION.bindAttribute(gl, uniforms.position);
        // bindBuffer(gl, uniforms.position, mesh.positions);
        // bindBuffer(gl, uniforms.normal, mesh.normals);
        // bindBuffer(gl, uniforms.tangent, mesh.tangents);
        // bindBuffer(gl, uniforms.texCoord, mesh.texCoord);
        // bindBuffer(gl, uniforms.joints, mesh.joints);
        // bindBuffer(gl, uniforms.weights, mesh.weights);

        // gl.uniformMatrix4fv(uniforms.mMatrix, false, transform);

        if (this.indicesAccessor) {
            let bufferView = this.indicesAccessor.bufferView;
            bufferView.buffer.bind(gl, gl.ELEMENT_ARRAY_BUFFER);
            gl.drawElements(
                this.mode,
                this.indicesAccessor.count,
                gl.UNSIGNED_SHORT,
                bufferView.byteOffset
            );
        } else {
            let positionAccessor = this.attributeAccessors.POSITION;
            // TODO: first?
            gl.drawArrays(this.mode, 0, positionAccessor.count);
        }
    }
}

class Mesh {
    constructor(gl, mesh, accessors, materials) {
        this.primitives = mesh.primitives.map((x) => new Primitive(gl, x, accessors, materials));
    }

    render(gl, uniforms) {
        this.primitives.forEach((primitive) => {
            primitive.render(gl, uniforms);
        });
    }
}

class Material {
    constructor(data) {
        // TODO
    }
}

class Node {
    constructor(node, meshes) {
        this.mesh = meshes[node.mesh] || null;
        this.children = (node.chidren || []).map((x) => new Node(x, meshes));
    }

    render(gl, uniforms) {
        if (this.mesh) {
            this.mesh.render(gl, uniforms);
        }

        this.children.forEach((child) => {
            child.render(gl, uniforms);
        });
    }
}

class Model {
    constructor(nodes) {
        this.nodes = nodes;
    }

    render(gl, uniforms) {
        this.nodes.forEach((node) => {
            node.render(gl, uniforms);
        });
    }

    static async create(gl, gltf) {
        let buffers = await Promise.all(gltf.buffers.map(Buffer.create));
        let bufferViews = gltf.bufferViews.map((x) => new BufferView(x, buffers));
        let accessors = gltf.accessors.map((x) => new Accessor(x, bufferViews));
        let materials = gltf.materials.map((x) => new Material(x));
        let meshes = gltf.meshes.map((x) => new Mesh(gl, x, accessors, materials));
        let nodes = gltf.nodes.map((x) => new Node(x, meshes));
        let sceneIndex = gltf.scene || 0;
        let sceneNodes = gltf.scenes[sceneIndex].nodes.map((x) => nodes[x]);
        return new Model(sceneNodes);
    }
}

class Shader {
    // TODO: load source from backend in async
    constructor(gl, source, type) {
        this.shader = gl.createShader(type);
        if (!this.shader) {
            throw new Error("gl.createShader returned null");
        }
        gl.shaderSource(this.shader, source);
        gl.compileShader(this.shader);
        if (!gl.getShaderParameter(this.shader, gl.COMPILE_STATUS)) {
            console.error(`Failed to load shader`);
            console.error(gl.getShaderInfoLog(this.shader));
        }
    }

    static async create(gl, uri, type){
        let response = await fetch(uri);
        let source = await response.text();
        return new Shader(gl, source, type);
    }
}

class Program {
    constructor(gl, shaders) {
        this.program = gl.createProgram();
        if (!this.program) {
            throw new Error("gl.createProgram returned null");
        }

        shaders.forEach((shader) => gl.attachShader(this.program, shader.shader));
        gl.linkProgram(this.program);

        if (!gl.getProgramParameter(this.program, gl.LINK_STATUS)) {
            console.error(gl.getProgramInfoLog(this.program));
        }
        gl.useProgram(this.program);

        this.uniforms = {
            position: gl.getAttribLocation(this.program, "vertexPosition"),
        };
    }
}

async function main() {
    const canvas = document.getElementById("glCanvas");
    const gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");

    // If we don't have a GL context, give up now
    if (!gl) {
        alert("Unable to initialize WebGL. Your browser or machine may not support it.");
        return;
    }

    // Vertex shader program
    // const vertexShaderSource = `
    //     attribute vec4 vertexPosition;
    //     uniform mat4 uModelViewMatrix;
    //     uniform mat4 uProjectionMatrix;
    //     varying lowp vec4 vColor;
    //     void main(void) {
    //         gl_Position = uProjectionMatrix * uModelViewMatrix * vertexPosition;
    //         vColor = vec4(1.0, 1.0, 1.0, 1.0);
    //     }
    // `;

    // // Fragment shader program
    // const fragmentShaderSource = `
    //     varying lowp vec4 vColor;
    //     void main(void) {
    //         gl_FragColor = vColor;
    //     }
    // `;
    let shaders = await Promise.all([
        Shader.create(gl, '/vertex.glsl', gl.VERTEX_SHADER),
        Shader.create(gl, '/fragment.glsl', gl.FRAGMENT_SHADER),
    ]);
    let program = new Program(gl, shaders);

    // Initialize a shader program; this is where all the lighting
    // for the vertices and so forth is established.
    // const shaderProgram = initShaderProgram(gl, vertexShaderSource, fragmentShaderSource);

    // Collect all the info needed to use the shader program.
    // Look up which attributes our shader program is using
    // for aVertexPosition, aVertexColor and also
    // look up uniform locations.
    // const programInfo = {
    // program: shaderProgram,
    // attribLocations: {
    //     vertexPosition: gl.getAttribLocation(shaderProgram, "aVertexPosition"),
    //     vertexColor: gl.getAttribLocation(shaderProgram, "aVertexColor"),
    // },
    // uniformLocations: {
    //     projectionMatrix: gl.getUniformLocation(shaderProgram, "uProjectionMatrix"),
    //     modelViewMatrix: gl.getUniformLocation(shaderProgram, "uModelViewMatrix"),
    // },
    // };

    // Here's where we call the routine that builds all the
    // objects we'll be drawing.
    // const buffers = initBuffers(gl);

    model = await Model.create(gl, JSON.parse(gltfSource));

    var then = 0;

    // Draw the scene repeatedly
    function render(now) {
        now *= 0.001; // convert to seconds
        const deltaTime = now - then;
        then = now;

        drawScene(gl, program, deltaTime);

        requestAnimationFrame(render);
    }
    requestAnimationFrame(render);

    document.addEventListener("keydown", (event) => sendKeyEvent("press", event.key));
    document.addEventListener("keyup", (event) => sendKeyEvent("release", event.key));
}

window.onload = main;

//
// initBuffers
//
// Initialize the buffers we'll need. For this demo, we just
// have one object -- a simple three-dimensional cube.
//
// function initBuffers(gl) {
//     // Create a buffer for the cube's vertex positions.
//     const positionBuffer = gl.createBuffer();

//     // Select the positionBuffer as the one to apply buffer
//     // operations to from here out.
//     gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);

//     // Now create an array of positions for the cube.
//     const positions = [
//         // Front face
//         -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 1.0,

//         // Back face
//         -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, -1.0,

//         // Top face
//         -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0,

//         // Bottom face
//         -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, -1.0, 1.0,

//         // Right face
//         1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0,

//         // Left face
//         -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0,
//     ];

//     // Now pass the list of positions into WebGL to build the
//     // shape. We do this by creating a Float32Array from the
//     // JavaScript array, then use it to fill the current buffer.
//     gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

//     // Now set up the colors for the faces. We'll use solid colors
//     // for each face.
//     const faceColors = [
//         [1.0, 1.0, 1.0, 1.0], // Front face: white
//         [1.0, 0.0, 0.0, 1.0], // Back face: red
//         [0.0, 1.0, 0.0, 1.0], // Top face: green
//         [0.0, 0.0, 1.0, 1.0], // Bottom face: blue
//         [1.0, 1.0, 0.0, 1.0], // Right face: yellow
//         [1.0, 0.0, 1.0, 1.0], // Left face: purple
//     ];

//     // Convert the array of colors into a table for all the vertices.
//     var colors = [];
//     for (var j = 0; j < faceColors.length; ++j) {
//         const c = faceColors[j];

//         // Repeat each color four times for the four vertices of the face
//         colors = colors.concat(c, c, c, c);
//     }

//     const colorBuffer = gl.createBuffer();
//     gl.bindBuffer(gl.ARRAY_BUFFER, colorBuffer);
//     gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colors), gl.STATIC_DRAW);

//     // Build the element array buffer; this specifies the indices
//     // into the vertex arrays for each face's vertices.
//     const indexBuffer = gl.createBuffer();
//     gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);

//     // This array defines each face as two triangles, using the
//     // indices into the vertex array to specify each triangle's
//     // position.
//     const indices = [
//         0,
//         1,
//         2,
//         0,
//         2,
//         3, // front
//         4,
//         5,
//         6,
//         4,
//         6,
//         7, // back
//         8,
//         9,
//         10,
//         8,
//         10,
//         11, // top
//         12,
//         13,
//         14,
//         12,
//         14,
//         15, // bottom
//         16,
//         17,
//         18,
//         16,
//         18,
//         19, // right
//         20,
//         21,
//         22,
//         20,
//         22,
//         23, // left
//     ];

//     // Now send the element array to GL
//     gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);

//     return {
//         position: positionBuffer,
//         color: colorBuffer,
//         indices: indexBuffer,
//     };
// }

// //
// // Draw the scene.
// //
function drawScene(gl, program, deltaTime) {
    gl.clearColor(0.5, 0.5, 0.5, 1.0);
    gl.clearDepth(1.0); // Clear everything
    gl.enable(gl.DEPTH_TEST); // Enable depth testing
    gl.depthFunc(gl.LEQUAL); // Near things obscure far things

    // Clear the canvas before we start drawing on it.

    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    // Create a perspective matrix, a special matrix that is
    // used to simulate the distortion of perspective in a camera.
    // Our field of view is 45 degrees, with a width/height
    // ratio that matches the display size of the canvas
    // and we only want to see objects between 0.1 units
    // and 100 units away from the camera.

    const fieldOfView = (45 * Math.PI) / 180; // in radians
    const aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
    const zNear = 0.1;
    const zFar = 100.0;
    const projectionMatrix = mat4.create();

    // note: glmatrix.js always has the first argument
    // as the destination to receive the result.
    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);

    // Set the drawing position to the "identity" point, which is
    // the center of the scene.
    const modelViewMatrix = mat4.create();

    // Now move the drawing position a bit to where we want to
    // start drawing the square.
    mat4.translate(
        modelViewMatrix, // destination matrix
        modelViewMatrix, // matrix to translate
        [-0.0, 0.0, -6.0]
    ); // amount to translate
    mat4.rotate(
        modelViewMatrix, // destination matrix
        modelViewMatrix, // matrix to rotate
        cubeRotation, // amount to rotate in radians
        [0, 0, 1]
    ); // axis to rotate around (Z)
    mat4.rotate(
        modelViewMatrix, // destination matrix
        modelViewMatrix, // matrix to rotate
        cubeRotation * 0.7, // amount to rotate in radians
        [0, 1, 0]
    ); // axis to rotate around (X)

    // Tell WebGL how to pull out the positions from the position
    // buffer into the vertexPosition attribute
    // {
    //     const numComponents = 3;
    //     const type = gl.FLOAT;
    //     const normalize = false;
    //     const stride = 0;
    //     const offset = 0;
    //     gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
    //     gl.vertexAttribPointer(
    //         programInfo.attribLocations.vertexPosition,
    //         numComponents,
    //         type,
    //         normalize,
    //         stride,
    //         offset
    //     );
    //     gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);
    // }

    // Tell WebGL how to pull out the colors from the color buffer
    // into the vertexColor attribute.
    // {
    //     const numComponents = 4;
    //     const type = gl.FLOAT;
    //     const normalize = false;
    //     const stride = 0;
    //     const offset = 0;
    //     gl.bindBuffer(gl.ARRAY_BUFFER, buffers.color);
    //     gl.vertexAttribPointer(
    //         programInfo.attribLocations.vertexColor,
    //         numComponents,
    //         type,
    //         normalize,
    //         stride,
    //         offset
    //     );
    //     gl.enableVertexAttribArray(programInfo.attribLocations.vertexColor);
    // }

    // Tell WebGL which indices to use to index the vertices
    // gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers.indices);

    // Tell WebGL to use our program when drawing
    gl.useProgram(program.program);

    let projectionMatrixLocation = gl.getUniformLocation(program.program, "uProjectionMatrix");
    let modelViewMatrixLocation = gl.getUniformLocation(program.program, "uModelViewMatrix");

    // Set the shader uniforms
    gl.uniformMatrix4fv(projectionMatrixLocation, false, projectionMatrix);
    gl.uniformMatrix4fv(modelViewMatrixLocation, false, modelViewMatrix);

    // {
    //     const vertexCount = 36;
    //     const type = gl.UNSIGNED_SHORT;
    //     const offset = 0;
    //     gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
    // }
    model.render(gl, program.uniforms);

    // Update the rotation for the next draw
    cubeRotation += deltaTime;
}

//
// Initialize a shader program, so WebGL knows how to draw our data
//
// function initShaderProgram(gl, vsSource, fsSource) {
//     const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
//     const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

//     // Create the shader program
//     const shaderProgram = gl.createProgram();
//     gl.attachShader(shaderProgram, vertexShader);
//     gl.attachShader(shaderProgram, fragmentShader);
//     gl.linkProgram(shaderProgram);

//     // If creating the shader program failed, alert
//     if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
//         alert("Unable to initialize the shader program: " + gl.getProgramInfoLog(shaderProgram));
//         return null;
//     }

//     return shaderProgram;
// }

// //
// // creates a shader of the given type, uploads the source and
// // compiles it.
// //
// function loadShader(gl, type, source) {
//     const shader = gl.createShader(type);

//     // Send the source to the shader object
//     gl.shaderSource(shader, source);

//     // Compile the shader program
//     gl.compileShader(shader);

//     // See if it compiled successfully
//     if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
//         alert("An error occurred compiling the shaders: " + gl.getShaderInfoLog(shader));
//         gl.deleteShader(shader);
//         return null;
//     }

//     return shader;
// }

gltfSource = `
{
    "asset": {
        "generator": "Khronos glTF Blender I/O v1.8.19",
        "version": "2.0"
    },
    "scene": 0,
    "scenes": [
        {
            "name": "Scene",
            "nodes": [0]
        }
    ],
    "nodes": [
        {
            "mesh": 0,
            "name": "Cube"
        }
    ],
    "materials": [
        {
            "doubleSided": true,
            "name": "Material",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.800000011920929, 0.800000011920929, 0.800000011920929, 1],
                "metallicFactor": 0,
                "roughnessFactor": 0.4000000059604645
            }
        }
    ],
    "meshes": [
        {
            "name": "Cube",
            "primitives": [
                {
                    "attributes": {
                        "POSITION": 0,
                        "NORMAL": 1,
                        "TEXCOORD_0": 2
                    },
                    "indices": 3,
                    "material": 0
                }
            ]
        }
    ],
    "accessors": [
        {
            "bufferView": 0,
            "componentType": 5126,
            "count": 24,
            "max": [1, 1, 1],
            "min": [-1, -1, -1],
            "type": "VEC3"
        },
        {
            "bufferView": 1,
            "componentType": 5126,
            "count": 24,
            "type": "VEC3"
        },
        {
            "bufferView": 2,
            "componentType": 5126,
            "count": 24,
            "type": "VEC2"
        },
        {
            "bufferView": 3,
            "componentType": 5123,
            "count": 36,
            "type": "SCALAR"
        }
    ],
    "bufferViews": [
        {
            "buffer": 0,
            "byteLength": 288,
            "byteOffset": 0
        },
        {
            "buffer": 0,
            "byteLength": 288,
            "byteOffset": 288
        },
        {
            "buffer": 0,
            "byteLength": 192,
            "byteOffset": 576
        },
        {
            "buffer": 0,
            "byteLength": 72,
            "byteOffset": 768
        }
    ],
    "buffers": [
        {
            "byteLength": 840,
            "uri": "data:application/octet-stream;base64,AACAPwAAgD8AAIC/AACAPwAAgD8AAIC/AACAPwAAgD8AAIC/AACAPwAAgL8AAIC/AACAPwAAgL8AAIC/AACAPwAAgL8AAIC/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgD8AAIA/AACAPwAAgL8AAIA/AACAPwAAgL8AAIA/AACAPwAAgL8AAIA/AACAvwAAgD8AAIC/AACAvwAAgD8AAIC/AACAvwAAgD8AAIC/AACAvwAAgL8AAIC/AACAvwAAgL8AAIC/AACAvwAAgL8AAIC/AACAvwAAgD8AAIA/AACAvwAAgD8AAIA/AACAvwAAgD8AAIA/AACAvwAAgL8AAIA/AACAvwAAgL8AAIA/AACAvwAAgL8AAIA/AAAAAAAAAAAAAIC/AAAAAAAAgD8AAACAAACAPwAAAAAAAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIC/AACAPwAAAAAAAACAAAAAAAAAAAAAAIA/AAAAAAAAgD8AAACAAACAPwAAAAAAAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIA/AACAPwAAAAAAAACAAACAvwAAAAAAAACAAAAAAAAAAAAAAIC/AAAAAAAAgD8AAACAAACAvwAAAAAAAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIC/AACAvwAAAAAAAACAAAAAAAAAAAAAAIA/AAAAAAAAgD8AAACAAACAvwAAAAAAAACAAAAAAAAAgL8AAACAAAAAAAAAAAAAAIA/AAAgPwAAAD8AACA/AAAAPwAAID8AAAA/AADAPgAAAD8AAMA+AAAAPwAAwD4AAAA/AAAgPwAAgD4AACA/AACAPgAAID8AAIA+AADAPgAAgD4AAMA+AACAPgAAwD4AAIA+AAAgPwAAQD8AACA/AABAPwAAYD8AAAA/AADAPgAAQD8AAAA+AAAAPwAAwD4AAEA/AAAgPwAAgD8AACA/AAAAAAAAYD8AAIA+AADAPgAAgD8AAAA+AACAPgAAwD4AAAAAAQAOABQAAQAUAAcACgAGABMACgATABcAFQASAAwAFQAMAA8AEAADAAkAEAAJABYABQACAAgABQAIAAsAEQANAAAAEQAAAAQA"
        }
    ]
}
`;

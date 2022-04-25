let cubeRotation = 0.0;
let instanceID = null;
let playerID = null;
let model = null;

let camera = {rY:1.5707963267948966,position:[0.9,-0.1,0]};

function sendJSONRequest(method, uri, payload) {
    fetch(uri, {
        method: method,
        headers: {
            Accept: "application/json",
            "Content-Type": "application/json",
        },
        body: JSON.stringify(payload),
    });
}

function sendKeyEvent(event_type, key) {
    let d = 0.1;
    if (key == "a") {
        camera.position[0] += d * Math.cos(camera.rY);
        camera.position[2] += d * Math.sin(camera.rY);
    }

    if (key == "d") {
        camera.position[0] -= d * Math.cos(camera.rY);
        camera.position[2] -= d * Math.sin(camera.rY);
    }

    if (key == "s") {
        camera.position[0] += d * Math.cos(camera.rY - Math.PI / 2);
        camera.position[2] += d * Math.sin(camera.rY - Math.PI / 2);
    }

    if (key == "w") {
        camera.position[0] -= d * Math.cos(camera.rY - Math.PI / 2);
        camera.position[2] -= d * Math.sin(camera.rY - Math.PI / 2);
    }

    if (key == "q") {
        camera.position[1] -= d;
    }

    if (key == "e") {
        camera.position[1] += d;
    }

    sendJSONRequest("POST", "/event", {
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

    static async create(buffer, modelURI) {
        let uri = buffer.uri;
        if (!buffer.uri.startsWith("data:application/octet-stream;base64,")) {
            uri = modelURI.slice(0, modelURI.lastIndexOf('/') + 1) + buffer.uri;
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
        this.byteStride = bufferView.byteStride || 0;
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
    }

    bindAttribute(gl, position) {
        gl.enableVertexAttribArray(position);
        this.bufferView.buffer.bind(gl, gl.ARRAY_BUFFER);
        gl.vertexAttribPointer(
            position,
            this.elementSize,
            this.componentType,
            false,
            this.bufferView.byteStride,
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

    render(gl, program) {
        if (this.material) {
            if (this.material.colourTexture) {
                this.material.colourTexture.bind(gl, 0, program.uniforms.colourTexture);
            }
        }
        this.attributeAccessors.POSITION.bindAttribute(gl, program.attributes.position);
        this.attributeAccessors.TEXCOORD_0.bindAttribute(gl, program.attributes.textureCoordinate);

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

    render(gl, program) {
        this.primitives.forEach((primitive) => {
            primitive.render(gl, program);
        });
    }
}

class Material {
    constructor(material, textures) {
        this.colourTexture = null;
        if (material.pbrMetallicRoughness && material.pbrMetallicRoughness.baseColorTexture) {
            this.colourTexture = textures[material.pbrMetallicRoughness.baseColorTexture.index];
        }
        // TODO
    }
}

class Node {
    constructor(node, meshes) {
        this.mesh = meshes[node.mesh] || null;
        this.childrenIndices = node.children || [];
    }

    render(gl, program) {
        if (this.mesh) {
            this.mesh.render(gl, program);
        }

        this.children.forEach((child) => child.render(gl, program));
    }

    linkChildren(nodes) {
        this.children = this.childrenIndices.map((i) => nodes[i]);
    }
}

class Sampler {
    constructor(gl, sampler) {
        this.magFilter = sampler.magFilter || gl.LINEAR;
        this.minFilter = sampler.minFilter || gl.NEAREST_MIPMAP_LINEAR;
        this.wrapS = sampler.wrapS || gl.REPEAT;
        this.wrapT = sampler.wrapT || gl.REPEAT;
    }
}

class Texture {
    constructor(gl, texture, samplers, images) {
        this.sampler = samplers[texture.sampler] || new Sampler(gl, {});
        this.texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, images[texture.source]);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, this.sampler.minFilter);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, this.sampler.magFilter);

        let ext = gl.getExtension("EXT_texture_filter_anisotropic");
        if (ext) {
            let max = gl.getParameter(ext.MAX_TEXTURE_MAX_ANISOTROPY_EXT);
            gl.texParameterf(gl.TEXTURE_2D, ext.TEXTURE_MAX_ANISOTROPY_EXT, max);
        }

        gl.generateMipmap(gl.TEXTURE_2D);
    }

    bind(gl, target, position) {
        gl.activeTexture(gl.TEXTURE0 + target);
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.uniform1i(position, target);
    }
}

class Model {
    constructor(nodes) {
        this.nodes = nodes;
    }

    render(gl, program) {
        this.nodes.forEach((node) => node.render(gl, program));
    }

    static async create(gl, uri) {
        let response = await fetch(uri);
        let gltf = await response.json();
        let buffers = await Promise.all(gltf.buffers.map((x) => Buffer.create(x, uri)));
        let images = await Promise.all((gltf.images || []).map((x) => createImage(x, uri)));
        let samplers = (gltf.samplers || []).map((x) => new Sampler(gl, x));
        let textures = (gltf.textures || []).map((x) => new Texture(gl, x, samplers, images));

        let bufferViews = gltf.bufferViews.map((x) => new BufferView(x, buffers));
        let accessors = gltf.accessors.map((x) => new Accessor(x, bufferViews));
        let materials = gltf.materials.map((x) => new Material(x, textures));
        let meshes = gltf.meshes.map((x) => new Mesh(gl, x, accessors, materials));
        let nodes = gltf.nodes.map((x) => new Node(x, meshes));
        nodes.forEach((x) => x.linkChildren(nodes));
        let sceneIndex = gltf.scene || 0;
        let sceneNodes = gltf.scenes[sceneIndex].nodes.map((x) => nodes[x]);
        return new Model(sceneNodes);
    }
}

function createImage(imageData, modelURI) {
    let image = new Image();
    let uri = imageData.uri;
    if (!imageData.uri.startsWith("data:")) {
        uri = modelURI.slice(0, modelURI.lastIndexOf('/') + 1) + imageData.uri;
    }
    return new Promise((resolve, reject) => {
        image.onload = () => resolve(image);
        image.onerror = reject;
        image.src = uri;
    });
}

class Shader {
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

    static async create(gl, uri, type) {
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

        this.attributes = {
            position: gl.getAttribLocation(this.program, "vertexPosition"),
            textureCoordinate: gl.getAttribLocation(this.program, "vertexTextureCoordinate"),
        };

        this.uniforms = {
            colourTexture: gl.getUniformLocation(this.program, "colourTexture"),
        };
    }
}

function mouseMove(event) {
    if (event.buttons) {
        camera.rY += event.movementX / 100;
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

    instanceID = document.URL.slice(document.URL.lastIndexOf("/") + 1);
    fetch('/player', {
        method: "POST",
        body: JSON.stringify({ instanceID: instanceID }),
    })
        .then((response) => response.json())
        .then((newPlayer) => {
            playerID = newPlayer.ID;
        });

    let shaders = await Promise.all([
        Shader.create(gl, "/vertex.glsl", gl.VERTEX_SHADER),
        Shader.create(gl, "/fragment.glsl", gl.FRAGMENT_SHADER),
    ]);
    let program = new Program(gl, shaders);

    model = await Model.create(gl, "/asset/glTF/BarramundiFish.gltf");

    let then = 0;
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
    document.addEventListener("mousemove", mouseMove);
}

window.onload = main;

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
    const zFar = 10000.0;
    const projectionMatrix = mat4.create();

    // note: glmatrix.js always has the first argument
    // as the destination to receive the result.
    mat4.perspective(projectionMatrix, fieldOfView, aspect, zNear, zFar);

    // Set the drawing position to the "identity" point, which is
    // the center of the scene.
    const modelViewMatrix = mat4.create();

    // Now move the drawing position a bit to where we want to
    // start drawing the square.
    mat4.rotate(
        modelViewMatrix, // destination matrix
        modelViewMatrix, // matrix to rotate
        camera.rY, // amount to rotate in radians
        [0, 1, 0]
    );
    mat4.translate(
        modelViewMatrix, // destination matrix
        modelViewMatrix, // matrix to translate
        camera.position
    ); // amount to translate
    gl.useProgram(program.program);

    let projectionMatrixLocation = gl.getUniformLocation(program.program, "uProjectionMatrix");
    let modelViewMatrixLocation = gl.getUniformLocation(program.program, "uModelViewMatrix");

    gl.uniformMatrix4fv(projectionMatrixLocation, false, projectionMatrix);
    gl.uniformMatrix4fv(modelViewMatrixLocation, false, modelViewMatrix);

    model.render(gl, program);
}

let cubeRotation = 0.0;
let instanceID = null;
let playerID = null;
let model = null;
const DRACO_EXTENSION_NAME = "KHR_draco_mesh_compression";

let camera = {
    rY: 0,
    position:[ -3.5, 12.600000000000001, 33.29999999999976 ]
};

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
    if (key == "w") {
        camera.position[0] -= d * Math.sin(camera.rY );
        camera.position[2] -= d * Math.cos(camera.rY );
    }

    if (key == "s") {
        camera.position[0] += d * Math.sin(camera.rY );
        camera.position[2] += d * Math.cos(camera.rY );
    }

    if (key == "a") {
        camera.position[0] += d * Math.sin(camera.rY - Math.PI/2);
        camera.position[2] += d * Math.cos(camera.rY- Math.PI/2);
    }

    if (key == "d") {
        camera.position[0] -= d * Math.sin(camera.rY- Math.PI/2);
        camera.position[2] -= d * Math.cos(camera.rY- Math.PI/2);
    }

    if (key == "q") {
        camera.position[1] += d;
    }

    if (key == "e") {
        camera.position[1] -= d;
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
            uri = modelURI.slice(0, modelURI.lastIndexOf("/") + 1) + buffer.uri;
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
        this.byteOffset = bufferView.byteOffset || 0;
        this.byteLength = bufferView.byteLength || this.buffer.arrayBuffer.byteLength - this.byteOffset;
        this.byteStride = bufferView.byteStride || 0;
    }
}

class Accessor {
    constructor(accessor, bufferViews) {
        this.bufferView = bufferViews[accessor.bufferView] || null;
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
    constructor(indicesAccessor, material, mode, attributeAccessors) {
        this.indicesAccessor = indicesAccessor;
        this.material = material;
        this.mode = mode;
        this.attributeAccessors = attributeAccessors;
    }

    static async create(gl, primitive, accessors, materials, bufferViews) {
        let indicesAccessor = accessors[primitive.indices] || null;
        let material = materials[primitive.material];
        let mode = primitive.mode || gl.TRIANGLES;

        let attributeAccessors = {};
        for (let key in primitive.attributes) {
            attributeAccessors[key] = accessors[primitive.attributes[key]];
        }

        let dracoExtension = primitive?.extensions[DRACO_EXTENSION_NAME];
        let decoderConfig = {
            type: "js",
        };
        let decoderModule = await DracoDecoderModule(decoderConfig);
        let decoder = new decoderModule.Decoder();
        if (dracoExtension) {
            const bufferView = bufferViews[dracoExtension.bufferView];
            const decoderBuffer = new decoderModule.DecoderBuffer();
            decoderBuffer.Init(
                new Int8Array(
                    bufferView.buffer.arrayBuffer,
                    bufferView.byteOffset,
                    bufferView.byteLength
                ),
                bufferView.byteLength
            );
            let dracoGeometry;
            let decodingStatus;

            const geometryType = decoder.GetEncodedGeometryType(decoderBuffer);

            if (geometryType === decoderModule.TRIANGULAR_MESH) {
                dracoGeometry = new decoderModule.Mesh();
                decodingStatus = decoder.DecodeBufferToMesh(decoderBuffer, dracoGeometry);
            } else {
                throw new Error("Draco unexpected geometry type.");
            }

            if (!decodingStatus.ok() || dracoGeometry.ptr === 0) {
                throw new Error("Draco decoding failed: " + decodingStatus.error_msg());
            }

            indicesAccessor.bufferView = this.decodeIndexBufferView(decoderModule, decoder, dracoGeometry, indicesAccessor);
            for (let key in dracoExtension.attributes) {
                let attributeName = key == "TEXCOORD_0" ? "TEX_COORD" : key;
                let attributeID = decoder.GetAttributeId(
                    dracoGeometry,
                    decoderModule[attributeName]
                );
                if (attributeID != -1) {
                    let attribute = decoder.GetAttribute(dracoGeometry, attributeID);
                    let bufferView = this.decodeAttributeBufferView(
                        decoderModule,
                        decoder,
                        dracoGeometry,
                        key,
                        attribute,
                        attributeAccessors[key]
                    );
                    attributeAccessors[key].bufferView = bufferView;
                }
            }
        }

        return new Primitive(indicesAccessor, material, mode, attributeAccessors);
    }

    static decodeIndexBufferView(decoderModule, decoder, dracoGeometry, indicesAccessor) {
        const byteLength = indicesAccessor.count * 2;
        const ptr = decoderModule._malloc(byteLength);
        decoder.GetTrianglesUInt16Array(dracoGeometry, byteLength, ptr);
        const indexBuffer = new Int8Array(decoderModule.HEAP8.buffer, ptr, byteLength).slice();
        decoderModule._free(ptr);
        let buffer = new Buffer(indexBuffer.buffer);
        return new BufferView({ buffer: 0 }, [buffer]);
    }

    static decodeAttributeBufferView(
        decoderModule,
        decoder,
        dracoGeometry,
        attributeName,
        attribute,
        attributeAccessor
    ) {
        const attributeTypes = {
            POSITION: Float32Array,
            NORMAL: Float32Array,
            COLOR: Float32Array,
            TEXCOORD_0: Float32Array,
            TANGENT: Float32Array,
        };
        const AttributeType = attributeTypes[attributeName];

        const byteLength = attributeAccessor.count * attributeAccessor.elementSize * 4;
        const dataType = this.getDracoDataType(decoderModule, AttributeType);
        const ptr = decoderModule._malloc(byteLength);
        decoder.GetAttributeDataArrayForAllPoints(
            dracoGeometry,
            attribute,
            dataType,
            byteLength,
            ptr
        );

        const attributeBuffer = new Int8Array(decoderModule.HEAP8.buffer, ptr, byteLength).slice();
        decoderModule._free(ptr);
        let buffer = new Buffer(attributeBuffer.buffer);
        return new BufferView({ buffer: 0 }, [buffer]);
    }

    static getDracoDataType(decoderModule, attributeType) {
        switch (attributeType) {
            case Float32Array:
                return decoderModule.DT_FLOAT32;
            case Int8Array:
                return decoderModule.DT_INT8;
            case Int16Array:
                return decoderModule.DT_INT16;
            case Int32Array:
                return decoderModule.DT_INT32;
            case Uint8Array:
                return decoderModule.DT_UINT8;
            case Uint16Array:
                return decoderModule.DT_UINT16;
            case Uint32Array:
                return decoderModule.DT_UINT32;
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
    constructor(primitives) {
        this.primitives = primitives;
    }

    static async create(gl, mesh, accessors, materials, bufferViews) {
        let primitives = await Promise.all(
            mesh.primitives.map((x) => Primitive.create(gl, x, accessors, materials, bufferViews))
        );
        return new Mesh(primitives);
    }

    render(gl, program) {
        this.primitives.forEach((primitive) => primitive.render(gl, program));
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
        this.matrix = mat4.create();
        if (node.translation){
            mat4.translate(this.matrix, this.matrix, node.translation); 
        }
    }

    render(gl, program) {
        let modelMatrixLocation = gl.getUniformLocation(program.program, "modelMatrix");
        gl.uniformMatrix4fv(modelMatrixLocation, false, this.matrix);

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
        let meshes = await Promise.all(
            gltf.meshes.map((x) => Mesh.create(gl, x, accessors, materials, bufferViews))
        );
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
        uri = modelURI.slice(0, modelURI.lastIndexOf("/") + 1) + imageData.uri;
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
    fetch("/player", {
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

    model = await Model.create(gl, "/asset/glTF/glTF-Draco/Lantern.gltf");

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
    gl.clearColor(0.7, 0.4, 0.7, 1.0);
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
    const viewMatrix = mat4.create();
    mat4.translate(
        viewMatrix, // destination matrix
        viewMatrix, // matrix to translate
        camera.position
    ); // amount to translate
    mat4.rotate(
        viewMatrix, // destination matrix
        viewMatrix, // matrix to rotate
        camera.rY, // amount to rotate in radians
        [0, 1, 0]
    );
    mat4.invert(viewMatrix, viewMatrix);
    gl.useProgram(program.program);

    let projectionMatrixLocation = gl.getUniformLocation(program.program, "uProjectionMatrix");
    let viewMatrixLocation = gl.getUniformLocation(program.program, "viewMatrix");

    gl.uniformMatrix4fv(projectionMatrixLocation, false, projectionMatrix);
    gl.uniformMatrix4fv(viewMatrixLocation, false, viewMatrix);

    model.render(gl, program);
}

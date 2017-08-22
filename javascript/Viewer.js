class Viewer {
    constructor() {
        this.planes = [];
        this.renderer;
    }
    init() {
        // Set the scene size.
        const WIDTH = window.innerWidth;
        const HEIGHT = window.innerHeight;

        // Set some camera attributes.
        const VIEW_ANGLE = 45;
        const ASPECT = WIDTH / HEIGHT;
        const NEAR = 0.1;
        const FAR = 10000;

        // Get the DOM element to attach to
        this.container = document.querySelector("#container");
        // Create a WebGL renderer, camera
        // and a scene
        this.renderer = new THREE.WebGLRenderer();
        this.camera =
            new THREE.PerspectiveCamera(
                VIEW_ANGLE,
                ASPECT,
                NEAR,
                FAR
            );
        this.camera.up.set( 0, 0, 1 );
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x393939);
        // Add the camera to the scene.
        this.scene.add(this.camera);

        // Start the renderer.
        this.renderer.setSize(WIDTH, HEIGHT);
        this.resized = true;
        window.addEventListener("resize",()=>{
            this.resized = true;
        })

        // Attach the webgl dom element.
        this.container.appendChild(this.renderer.domElement);

        // TODO: create a generator to cycle through colours.
        this.baseMaterial = new THREE.MeshLambertMaterial({
                color: 0xCC0000, side: THREE.DoubleSide
            });
        // TODO: scale and reposition to scene.
        // Add ambient lighting.
        this.ambilight = new THREE.AmbientLight(0xFFFFFF, 0.4);
        this.scene.add(this.ambilight);
        this.hemilight = new THREE.HemisphereLight(0xffffbb, 0x080820, 1);
        // hemilight.position = new THREE.Vector3( 0, 1, 0 );
        this.hemihelper = new THREE.HemisphereLightHelper(this.hemilight, 5 );

        // TODO: place in good position depending on geometry.
        this.hemilight.translateZ(10)
        this.hemilight.rotateX(-Math.PI/2)
        
        this.scene.add( this.hemihelper );
        this.scene.add(this.hemilight);
        this.controls = new THREE.OrbitControls( this.camera, this.renderer.domElement );
        // controls.addEventListener( 'change', update ); // remove when using animation loop
        // enable animation loop when using damping or autorotation
        this.controls.enableDamping = true;
        this.controls.dampingFactor = 0.25;
        this.controls.enableZoom = true;
        this.renderer.render(this.scene, this.camera);
        this.camera.position.z = 5;

        this.grid = new THREE.GridHelper(100, 100);
        this.grid.geometry.rotateX( Math.PI / 2 );
        this.scene.add(this.grid);
        this.axisHelper = new THREE.AxisHelper( 5 );
        this.scene.add( this.axisHelper );

        this.camera.position.x = 5;
        this.camera.position.y = -5;
        this.camera.position.z = 5;
        this.camera.lookAt( this.scene.position );

        // Kick off the rendering.
        requestAnimationFrame(this.update.bind(this));
    }
    update () {
        this.renderer.render(this.scene, this.camera);
        var timer = Date.now() * 0.001;
        // camera.position.x = Math.cos( timer ) * 5;
        // camera.position.y = Math.sin( timer ) * 5;
        // camera.position.z = Math.sin( timer ) * 5;
        // camera.lookAt( scene.position );

        this.controls.update(); // required if controls.enableDamping = true, or if controls.autoRotate = true

        if(this.resized){
            this.camera.aspect = window.innerWidth / window.innerHeight;
            this.camera.updateProjectionMatrix();
        
            this.renderer.setSize( window.innerWidth, window.innerHeight );
            this.resized = false;
        }
        // Run this function on the next frame
        requestAnimationFrame(this.update.bind(this));
    }
    createPlane(v1,v2,v3,v4) {
        const geom = new THREE.Geometry();
        if (v4) {
            // Rectangular surface.
            geom.vertices.push(
                new THREE.Vector3(v1.x, v1.y, v1.z),
                new THREE.Vector3(v2.x, v2.y, v2.z),
                new THREE.Vector3(v3.x, v3.y, v3.z),
                new THREE.Vector3(v4.x, v4.y, v4.z)
            );
            geom.faces.push( new THREE.Face3( 0, 1, 2 ),
                             new THREE.Face3( 2, 3, 0 )
            );
        } else {
            // Trianglular surface.
            geom.vertices.push(
                new THREE.Vector3(v1.x, v1.y, v1.z),
                new THREE.Vector3(v2.x, v2.y, v2.z),
                new THREE.Vector3(v3.x, v3.y, v3.z)
            );
            geom.faces.push(new THREE.Face3( 0, 1, 2 ));
        }
        geom.computeBoundingSphere();
        geom.computeFaceNormals();
        const mesh = new THREE.Mesh(geom, this.baseMaterial);
        this.planes.push(mesh);
        return mesh;
    }
    addSurface(inputData,surface) {
        const v1 = inputData.vertices.get(surface.v1);
        const v2 = inputData.vertices.get(surface.v2);
        const v3 = inputData.vertices.get(surface.v3);
        const v4 = inputData.vertices.get(surface.v4);
        const mesh = this.createPlane(v1,v2,v3,v4);
        this.scene.add(mesh);
        // Add wireframe.
        var eGeometry = new THREE.EdgesGeometry( mesh.geometry );
        var eMaterial = new THREE.LineBasicMaterial( { color: 0x0000000, linewidth: 8 } );
        var edges = new THREE.LineSegments( eGeometry , eMaterial );
        this.planes.push(edges);
        this.scene.add(edges);
    }
    addInputData(input) {
        for (const surf of input.surfaces.values()) {
            this.addSurface(input, surf);
        }
        return input;
    }
    clear() {
        for (const i of this.planes) {
            this.scene.remove(i);
        }
        this.planes = [];
    }
}


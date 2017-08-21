function parseInputFile(inputString) {
  // Initialise input object.
  const input = new View3DInput();
  // Split into lines.
  const lines = inputString.split(/\r?\n/);
  for (let line of lines) {
    switch (line[0]) {
      case '/':
      case '!':
        break;
      case 't':
      case 'T':
        // Encountered a title line.
        // Remove code and trim whitespace.
        const title = line.slice(1).trim();
        if(input.title) throw new Error('Title aleady determined');
        input.title = title;
        break;
      case 'c':
      case 'C':
        {
          // Encounted a control line.
          // Remove code and trim whitespace.
          const cl = line.slice(1).trim();
          const values = cl.split(/\s+|=/);
          // There must be an even number of values.
          const control = {};
          for (let i = 0; i < values.length; i = i + 2) {
            control[values[i]] = values[i+1];
          }
          if(input.control) throw new Error('control aleady determined');
          input.control = control;
        }
        break;
      case 'f':
      case 'F':
        // Encounted a geometry line, which determines the geometry format.
        // Remove code and trim whitespace.
        const fl = line.slice(1).trim();
        if(input.geom) throw new Error('geom aleady determined');
        input.geom = fl;
        break;
      case 'v':
      case 'V':
        {
          // Encounted a vertex line.
          // Remove code and trim whitespace.
          const vl = line.slice(1).trim();
          // Format is 1 integer and 3 floats. The integer is the vertex index
          // and the 3 floats are x, y, and z respectively.
          const values = vl.split(/\s+/);
          const index = parseInt(values[0]);
          const x = parseFloat(values[1]);
          const y = parseFloat(values[2]);
          const z = parseFloat(values[3]);
          input.addVertex(index, x, y, z);
        }
        break;
      case 's':
      case 'S':
        {
          // Encounted a surface line.
          // Remove code and trim whitespace.
          const sl = line.slice(1).trim();
          // Format is 7 integers, 1 float, and 1 string. The first integer is the
          // surface index. The next 4 integers are vertex indices of the surface.
          // THe last two integers are the base surface index and combination
          // surface index respectively. The string is the name of the surface.
          const values = sl.split(/\s+/);
          const index = parseInt(values[0]);
          const v1 = parseInt(values[1]);
          const v2 = parseInt(values[2]);
          const v3 = parseInt(values[3]);
          const v4 = parseInt(values[4]);
          const base = parseInt(values[5]);
          const cmb = parseInt(values[6]);
          const emit = parseFloat(values[7]);
          const name = values[8];
          input.addSurface(index, v1, v2, v3, v4, base, cmb, emit, name);
        }
        break;
      case '*':
      case 'e':
      case 'E':
        // Reached end of data.
        return input;
        break;
      default:
        console.log(line);
        break;
    }
  }
}

class View3DInput {
  constructor() {
      this.vertices = new Map();
      this.surfaces = new Map();
  }
  addVertex(index, x, y, z) {
    if(this.vertices.has(index))
      throw new Error(`Vertex index ${index} listed twice.`);
    this.vertices.set(index, new Vertex(x, y, z));
  }
  addSurface(index, v1, v2, v3, v4, base, cmb, emit, name) {
    if(this.surfaces.has(index))
      throw new Error(`Surface index ${index} listed twice.`);
    this.surfaces.set(index, new Surface(v1, v2, v3, v4, base, cmb, emit, name));
  }
}

class Vertex {
  constructor(x,y,z) {
    this.x = x;
    this.y = y;
    this.z = z;
  }
}

class Surface {
  constructor(v1, v2, v3, v4, base, cmb, emit, name) {
    this.v1 = v1;
    this.v2 = v2;
    this.v3 = v3;
    this.v4 = v4;
  }
}

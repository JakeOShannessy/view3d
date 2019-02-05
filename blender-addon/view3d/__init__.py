bl_info = {
    "name": "Export View3D",
    "category": "Import-Export",
    "blender" : (2, 80, 0),
    "version": (0, 1, 0),
}

import bpy
import os
from bpy_extras.io_utils import ExportHelper

class V3DData:
    title = "unnamed"
    vertices = []
    surfaces = []
    list = 1
    row = 0
    col = 0
    def add_mesh(self, mesh):
        prev_n_vertices = len(self.vertices)
        for vertex in mesh.vertices:
            self.vertices.append(vertex)
        for i,polygon in enumerate(mesh.polygons):
            polyverts = []
            for v in polygon.vertices:
                polyverts.append(prev_n_vertices+v)
            surface = V3DSurface(polyverts)
            if len(mesh.polygons) <= 1:
                surface.name = mesh.name
            else:
                surface.name = "{}-{}".format(mesh.name, i)
            self.surfaces.append(surface)

    def __str__(self):
        string = ""
        string += "T {}\n".format(self.title)
        string += "C list={} row={} col={}\n".format(self.list, self.row, self.col)
        string += "F 3\n"
        # TODO: space these properly
        string += "!  #  x  y  z\n"
        for i,v in enumerate(self.vertices):
            string += "V {} {} {} {}\n".format(i+1, v.co.x, v.co.y, v.co.z)
        string += "!   #   v1  v2  v3  v4 base cmb emit  name\n"
        for i,s in enumerate(self.surfaces):
            string += "S {} {} 0   0  0.90  {}\n".format(i+1, s, s.name)
        return string

class V3DSurface:
    name = "unnamed"
    def __init__(self, vertices):
        self.vertices = vertices
    def __str__(self):
        s = ""
        for v in self.vertices:
            s += "{}".format(v+1)
            s += " "
        return s

def export_view3d_menu(self, context):
    """Export current scene as a View3D input file"""

    filepath = "{0}.vs3".format(os.path.splitext(bpy.data.filepath)[0])
    directory = os.path.dirname(filepath)
    basename = os.path.basename(filepath)
    filepath = "{0}/{1}".format(directory, basename)
    #self.layout.operator(ExportView3D.bl_idname, text="Scene to View3D Input (.vs3)")
    self.layout.operator(ExportView3D.bl_idname, text="Scene to View3D Input (.vs3)").filepath = filepath

class ExportView3D(bpy.types.Operator, ExportHelper):
    """Operator which exports the current scene as a View3D input file"""

    bl_label = "Export View3D"
    bl_idname = "export_scene.vs3_input"
    bl_description = "Export current scene as a View3D input file"
    filename_ext = ".vs3"
    #filter_glob = bpy.props.StringProperty(default="*.vs3", options={'HIDDEN'})
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        window = context.window_manager.windows[0]
        window.cursor_modal_set("WAIT")
        filepath = self.filepath
        if not filepath.lower().endswith('.vs3'): filepath += '.vs3'
        filepath = bpy.path.abspath(filepath)
        # Cycle through each object and export each face
        # only operate on meshes
        v3data = V3DData()
        for obj in bpy.context.scene.objects:
            if obj.type == 'MESH':
                v3data.add_mesh(obj.data)
        v3data.title = bpy.context.scene.name
        string_data = str(v3data)
        try:
            with open(filepath,"w",encoding="utf8",errors="ignore") as out_file:
                out_file.write(string_data)
        except IOError:
            window.cursor_modal_restore()
            self.report({"ERROR"}, "Could not write to file, export failed")
            return {'CANCELLED'}

        window.cursor_modal_restore()
        self.report({"INFO"}, "View3D data exported")
        return {'FINISHED'}

def register():
    bpy.utils.register_class(ExportView3D)
    bpy.types.TOPBAR_MT_file_export.prepend(export_view3d_menu)


def unregister():
    bpy.utils.unregister_class(ExportView3D)
    bpy.types.TOPBAR_MT_file_export.remove(export_view3d_menu)


# This allows you to run the script directly from blenders text editor
# to test the addon without having to install it.
if __name__ == "__main__":
    register()

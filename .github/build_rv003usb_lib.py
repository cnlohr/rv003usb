import os.path
Import("env")

# find the path to our library. This is a bit hacky since in SCons we cannot just get the path to this file.
# Use a side effect of "-Ilib" in the library.json, which evaluates to  the full path to this library.
rv003usb_folder = None
for path in env['CPPPATH']:
    path = str(path)
    if os.path.basename(os.path.normpath(path)) == "lib":
        # try to go one level up, then into rv003usb and check if rv003usb.S is there
        lib_path = os.path.abspath(os.path.join(path, "..", "rv003usb"))
        if os.path.isfile(os.path.join(lib_path, "rv003usb.S")):
            rv003usb_folder = lib_path
            break
if rv003usb_folder is None:
    print("Fatal: Could not find rv003usb library folder")
    env.Exit(-1)

# build rv003usb.S in a separate folder to avoid name clash with rv003usb.c
env.BuildSources(
    os.path.join("$BUILD_DIR", "rv003usbASM"),
    rv003usb_folder,
    src_filter=[
        "-<*>",
        "+<rv003usb.S>",
    ]
)

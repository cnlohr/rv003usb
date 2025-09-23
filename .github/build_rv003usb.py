import os.path
Import("env")
# build rv003usb.S in a separate folder to avoid name clash with rv003usb.c
env.BuildSources(
    os.path.join("$BUILD_DIR", "rv003usbASM"),
    os.path.join(env.subst("$PROJECT_DIR"), "rv003usb"),
    src_filter=[
        "-<*>",
        "+<rv003usb.S>",
    ]
)
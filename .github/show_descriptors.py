Import("env")

def is_pio_build():
	from SCons.Script import COMMAND_LINE_TARGETS
	return all([x not in COMMAND_LINE_TARGETS for x in ["idedata", "_idedata", "__idedata"]])

# only do this when generating intellisense definitions, not during actual build.
if not is_pio_build():
    env.Append(CPPDEFINES=["INSTANCE_DESCRIPTORS"])

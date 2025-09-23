import os.path
Import("env", "projenv")
# Rely on the fact that all demos to build_src_filter = +<DEMO_THEY_WANT>
# And figure out correct include path from that
src_filter = str(env.subst("$SRC_FILTER"))
# A string with e.g. "+<ch32fun> +<examples/blink>" 
# rudimentary parsing is fine for CI
src_filters = src_filter.split("+")
example_folder = ""
for filter in src_filters:
   # cleanup unwanted characters
   f = filter.replace("<", "", 1).replace(">", "", 1)
   # Some examples set a source filter to only build one file, in which case we need the base directory
   if f.endswith(".c"):
      f = os.path.dirname(f)
   # starts with "demo" or "bootloader?
   if (f.startswith("demo") or f.startswith("bootloader") or f.startswith("testing")) and os.path.isdir(f):
      example_folder = f
      break
   
# propagate to all construction environments
for e in env, projenv, DefaultEnvironment():
   e.Append(CCFLAGS=[("-I", example_folder)])
   e.Append(ASFLAGS=[("-I", example_folder)])
   e.Append(CPPPATH=[example_folder])

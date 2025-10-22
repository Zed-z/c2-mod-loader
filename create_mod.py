import shutil
import os, fnmatch
import sys

template = 'ModTemplate'
mod_name = sys.argv[1]

# Copy template folder
shutil.copytree(template, mod_name)
os.chdir(mod_name)

# Rename file names
for count, f in enumerate(os.listdir()):
    f_name, f_ext = os.path.splitext(f)
    f_name = f_name.replace(template, mod_name)
    new_name = f'{f_name}{f_ext}'
    os.rename(f, new_name)

# Replace mod name in template files
for dname, dirs, files in os.walk("."):
    for fname in files:
        fpath = os.path.join(dname, fname)
        with open(fpath) as f:
            s = f.read()
            
        s = s.replace(template, mod_name)
        s = s.replace(template.upper(), mod_name.upper())

        with open(fpath, "w") as f:
            f.write(s)

# Distutils build script
# Builds the extensions with separate SWIG run-time support
# (see http://www.swig.org/Doc1.3/Modules.html) so as to
# share datatypes among different extensions.

import os
import sys
import platform
from distutils.core import setup, Extension

if '--help' in str(sys.argv):
    setup(help = True)

    print ('IMPACT specific options (always as first argument):')
    print ('  return_acquire_version')
    print ('  increment_version_build_counter')
    print ('  update_config_sed')
    print
    print ('Example:')
    print ('  python setup.py return_acquire_version')
    print ('will print something like:')
    print ('  1.10.99.177')

    sys.exit()

def readEnvVar(var, errorMsg):
    try:
        s = os.environ[var]
    except KeyError:
        print ('No', var, 'in environment. Set this to the', errorMsg )
        sys.exit(-1)
    globals()[var.lower()] = s # lower case: env. var. FOO breeds global var. foo

if "MVIMPACT_ACQUIRE_SOURCE_DIR" in os.environ:
    readEnvVar('MVIMPACT_ACQUIRE_SOURCE_DIR', 'mvIMPACT Acquire source code root directory')
    globals()["mvimpact_acquire_dir"] = mvimpact_acquire_source_dir
elif "MVIMPACT_ACQUIRE_DIR" in os.environ:
    readEnvVar('MVIMPACT_ACQUIRE_DIR', 'mvIMPACT Acquire source code root directory')
    globals()["mvimpact_acquire_source_dir"] = mvimpact_acquire_dir
else:
    print ("Either 'MVIMPACT_ACQUIRE_DIR' or 'MVIMPACT_ACQUIRE_SOURCE_DIR' must be set")
    sys.exit(-1)

systemPlatform = platform.system()
if systemPlatform != "Linux" and systemPlatform != "Windows":
    print ("Could not determine system platform")
    sys.exit(-1)

if "BUILDDIR_DN" in os.environ:
    readEnvVar('BUILDDIR_DN', 'directory where to put all the build files.')
else:
    if systemPlatform == "Linux":
        os.system("export BUILDDIR_DN=" + mvimpact_acquire_dir + "/mvIMPACT_Python/Output")
    elif systemPlatform == "Windows":
        os.system("set BUILDDIR_DN=" + mvimpact_acquire_dir + "/mvIMPACT_Python/Output")
    builddir_dn=mvimpact_acquire_dir + "/mvIMPACT_Python/Output"
version_file = mvimpact_acquire_source_dir + '/DriverBase/Include/mvVersionInfo.h'

class AcquireVersion(object):
    def __init__(self):
        class BuildNr(str):
            def returnNr(self, key): # key is a str like 'MVIMPACT_ACQUIRE_MAJOR_VERSION   '
                offset = self.find(key)
                if offset == -1:
                    return '?'
                offset += len(key)
                while not self[offset].isdigit():
                    offset += 1
                nr = self[offset]
                while True:
                    offset += 1
                    if not self[offset].isdigit():
                        return nr
                    nr += self[offset]

        version_all_h = open(version_file)
        version = BuildNr(version_all_h.read())
        version_all_h.close()
        self.v_major = version.returnNr('MVIMPACT_ACQUIRE_MAJOR_VERSION   ')
        self.v_minor = version.returnNr('MVIMPACT_ACQUIRE_MINOR_VERSION   ')
        self.v_point = version.returnNr('MVIMPACT_ACQUIRE_RELEASE_VERSION ')

    def full(self):
        return self.v_major + '.' + self.v_minor + '.' + self.v_point

acquire_version = AcquireVersion()

if len(sys.argv) > 1:
  if sys.argv[1] == 'return_acquire_version':
    print ('%s.%s.%s' % (acquire_version.v_major,
                        acquire_version.v_minor,
                        acquire_version.v_point))
    sys.exit()

  if sys.argv[1] == 'increment_version_build_counter':
    class OrigVer(str):
        def __init__(self, s):
            str.__init__(s)
            self.offset = 0
            self.slices = []

        def getSlice(self, key): # key is a str like '___v_build' or '___sv_build'
            end = self.find(key, self.offset) + len(key)
            while not self[end].isdigit():
                end += 1
            begin = end
            while self[end].isdigit() or self[end] == '.':
                if self[end] == '.':
                    begin = end + 1
                end += 1
            buildNr = int(self[begin:end])
            self.slices.append(origVer[self.offset:begin] + str(buildNr + 1))
            self.offset = end

        def writeNew(self):
            return ''.join(self.slices) + origVer[self.offset:]

    vbc = open(version_file, 'r')
    origVer = OrigVer(vbc.read())
    vbc.close()
    origVer.getSlice('___v_build')
    origVer.getSlice('___sv_build')
    origVer.getSlice('___sv_version')
    vbc = open(version_file, 'w')
    vbc.write(origVer.writeNew())
    vbc.close()
    sys.exit()

  if sys.argv[1] == 'update_config_sed':
    cs = open('config.sed', 'w')
    cs.write('#!/bin/sed -f\n'
             's,@impact_major@,%s,g\n'
             's,@impact_minor@,%s,g\n'
             's,@impact_point@,%s,g\n' %
             (acquire_version.v_major, acquire_version.v_minor, acquire_version.v_point))
    cs.close()
    sys.exit()



if systemPlatform == "Linux":
    bits = os.system('getconf LONG_BIT')
elif systemPlatform == "Windows":
    bits = 64 if os.environ["PROCESSOR_ARCHITECTURE"] == "AMD64" else 32

macros = []
if os.getenv('DEFINES') != None:
    for dfn in os.getenv('DEFINES').split():
        macros += [(dfn[2:],'1')]

class dirlist(list):
    """You can define multiple directories in one environment variable"""
    def addDir(self, d):
        if type(d) is list:
            for a in d:
                self.addDir(a) # recurse
        elif type(d) is str:
            if os.pathsep in d: # turn 'bar;foo' into ['bar', 'foo']
                self.addDir(d.split(os.pathsep)) # recurse
            else:
                d = d.replace('\ ',' ')
                if os.path.isdir(d):
                    self.append(d)
    def getList(self):
        return list(self)

inc_dirs = dirlist()
lib_dirs = dirlist()

# inc_dirs     : additional include directories
# lib_dirs     : additional library directories
# macros       : defined while building the extension
# datafiles    : additional files to install in the Python directory

datafiles = []

installation_path = os.getenv('BUILD_FROM_INSTALLATION') # None if not set
if installation_path == '0': # '0' is explicit for 'not from installation'
    installation_path = None #  (like not specifying BUILD_FROM_INSTALLATION at all)
python_ver = sys.version[0:3]
if systemPlatform == "Linux":
    if bits == 32:
        target_lib_sub_dir = '/lib/x86'
    else:
        target_lib_sub_dir = '/lib/x86_64'

inc_dirs.addDir([mvimpact_acquire_source_dir])
inc_dirs.addDir([mvimpact_acquire_source_dir + '/mvIMPACT_CPP'])
if systemPlatform == "Windows":
    inc_dirs.addDir([mvimpact_acquire_source_dir + '/mvDisplay/Include'])
    lib_dirs.addDir([mvimpact_acquire_source_dir + '/lib'])
elif systemPlatform == "Linux":
    lib_dirs.addDir([mvimpact_acquire_source_dir + target_lib_sub_dir])

build_options = {'build_base' : builddir_dn}
bdist_options = {'bdist_base' : builddir_dn, 'dist_dir' : builddir_dn}

link_args = None
compile_args = ['-Wno-unknown-pragmas'] if systemPlatform == "Linux" else ['']
if bits == 64: # 64 bit build takes forever (hours and hours) when optimizing
    compile_args += ['-O0']
    link_args = ['-Wl,-O0'] # /usr/lib/python2.[56]/config/Makefile specifies '-Wl,-O1'

# Create an 'Extension' object that describes how to build the Python extension.
def createExtension():
    libs = ['mvDeviceManager']

    incl_dirs = inc_dirs.getList()
    libr_dirs = lib_dirs.getList()

    if installation_path:
        incl_dirs.insert(0, installation_path)
        incl_dirs.insert(0, installation_path + '\\mvDisplay\\Include')
        incl_dirs.insert(0, installation_path + '\\mvIMPACT_CPP')
        libr_dirs.insert(0, installation_path + '\\lib')
    else:
        incl_dirs.insert(0, mvimpact_acquire_source_dir)
    sources_fpns = [os.path.join(builddir_dn,'acquire_wrap.cpp')]
    return Extension('lib_mvIMPACT_acquire',
        sources       = sources_fpns,
        include_dirs  = incl_dirs,
        library_dirs  = libr_dirs,
        define_macros = macros,
        libraries     = libs,
        extra_compile_args = compile_args,
        extra_link_args = link_args)

from distutils.command.build_ext import build_ext

class build__ext(build_ext):
    # on Linux, once the lib has been built, dump a map, then strip the lib
    def build_extensions(self):
        self.check_extensions_list(self.extensions)

        for ext in self.extensions:
            self.build_extension(ext)
            libName = os.path.join(self.build_lib,'mvIMPACT',ext.name)

def getPlatform():
    if systemPlatform == "Linux":
        p = os.system('ldconfig -p | grep mvDeviceManager | cut -d \> -f 2 | grep -o "/lib/.*/" | cut -d/ -f 3')
        return p 
    elif systemPlatform == "Windows":
        p = os.environ["PROCESSOR_ARCHITECTURE"]
        return p 

setup(name = 'mvIMPACT',
    version     = acquire_version.full(),
    platforms   = getPlatform(),
    author      = 'MATRIX VISION GmbH',
    author_email= 'support@matrix-vision.de',
    url         = 'http://www.matrix-vision.com/products/software/hardwareinfo.php',
    license     = 'All rights reserved',
    description = 'mvIMPACT Acquire Python %s interface' % python_ver,
    long_description = 'Python %s interface for mvIMPACT Acquire, a generic image acquisition interface' % python_ver,
    ext_package = 'mvIMPACT', # all modules are in 'mvIMPACT'
    ext_modules = [createExtension()],
    data_files  = datafiles,
    py_modules  = ['mvIMPACT.acquire'],
    package_dir = {'mvIMPACT' : os.path.join(builddir_dn,'')},
    cmdclass    = {'build_ext' : (build_ext, build__ext)[os.name == 'posix']},
    options     = {'build' : build_options, 'bdist' : bdist_options})

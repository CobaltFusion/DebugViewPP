"""Apply clang-format to a given set of files
"""

from __future__ import print_function
import traceback
import os, sys, glob

def clangFormat(filemask, recursive):
    files = GetFiles(filemask, recursive)
    path = os.path.dirname(os.path.abspath(sys.argv[0]))
    os.chdir(path)
    for fullname in files:
        print("clang-format: " + fullname)
        os.system("clang-format -i " + fullname)

def listFiles(filemask, recursive):
    print("List: " + filemask + ", recursive:", recursive)
    for fullname in GetFiles(filemask, recursive):
        print(fullname)

def IsCppFile(fullname):
    filename, ext = os.path.splitext(fullname)
    if ext.lower().endswith(".cpp"):
        return True
    if ext.lower().endswith(".cc"):
        return True
    if ext.lower().endswith(".h"):
        return True
    if ext.lower().endswith(".hpp"):
        return True
    if ext.lower().endswith(".hh"):
        return True
    return False    

def IsIgnoredDirectory(dirname):
    if dirname.lower().endswith("\gen"):
        return True
    if dirname.lower().endswith("\gen64"):
        return True
    return False

def GetFiles(filemask, recurse):
    if (recurse):
        result = []
        path = os.path.dirname(os.path.abspath(filemask))
        for dirName, subdirList, fileList in os.walk(path):
            if IsIgnoredDirectory(dirName): 
                continue
            for fname in fileList:
                filename = dirName + "\\" +  fname
                if not IsCppFile(filename): 
                    continue
                result += [filename]
        return result
    else:
        result = []
        for filename in glob.glob(filemask):
            if not IsCppFile(filename): 
                continue
            result += [os.path.abspath(filename)]
        return result
    
def filter(keyword):
    for line in sys.stdin:
        if not keyword in line:
            sys.stdout.write(line)
        else:
            sys.stdout.write("// " + line)

def HasArgument(option):
    for arg in sys.argv:
        if (arg.strip() == ("/" + option)):
            return True
    return False

def HasInvalidArgs():
    args = len(sys.argv) - 1
    if args < 1 or args > 3:
        return True
    if sys.argv[1].startswith("/"):
        return True
    return False
    
def main():
    if HasInvalidArgs():
        print ("Usage: " + os.path.basename(sys.argv[0]) + " <filemask> [/s] [/f]")
        print ("  note: gen\ and gen64\ directories are ignored")
        print ("  /s = recursively process files in subdirectories");
        print ("  /f = actually format the selected files instead of just listing them");
        print ("")
        print ("  example: ")
        print ("    "+ os.path.basename(sys.argv[0]) + " src\*.* /s /f")
        return

    if HasArgument("f"):
        clangFormat(os.path.abspath(sys.argv[1]), HasArgument("s"))
    else:
        listFiles(os.path.abspath(sys.argv[1]), HasArgument("s"))

if __name__ == "__main__":
    try:
        main()
    except SystemExit :
        pass
    except KeyboardInterrupt:
        pass
    except Exception:
        traceback.print_exc(file=sys.stdout) 
    sys.exit(0)
    


#!/usr/bin/env python3
"""Creates a release of DebugView++: a clean build with a given version, a git tag, a zip file and a GitHub release.

    python utils/release.py -r 2.0.0.0            everything: -v 2.0.0.0 -c -t -z -p
    python utils/release.py -v 2.0.0.0 -c         compile clean, with version 2.0.0.0
    python utils/release.py -v 2.0.0.0 -t         create the git tag 2.0.0.0
    python utils/release.py -v 2.0.0.0 -z         create out/release/debugviewpp-v2.0.0.0.zip
    python utils/release.py -v 2.0.0.0 -p         publish the release on GitHub, with the zip attached

When more than one step is given they run in the order compile, tag, zip, publish, and the first one that fails stops the rest.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "out" / "release"
BIN_DIR = BUILD_DIR / "bin"
VERSION_H = ROOT / "application" / "DebugViewpp" / "version.h"
VERSION_WXI = ROOT / "application" / "DebugViewpp" / "version.wxi"
VERSION_FILES = [VERSION_H, VERSION_WXI]

# the binaries that go in the zip: the original and the packed (UPX) form, see the post-build steps in the CMakeLists.txt
BINARIES = ["Debugviewpp.exe", "Debugviewpp-small.exe", "DebugViewConsole.exe", "DebugViewConsole-small.exe"]
# the ones that have the version in their version resource, this is checked so that an old build is not released
VERSIONED_BINARIES = ["Debugviewpp.exe", "Debugviewpp-small.exe"]

VERSION_RE = re.compile(r"^\d+\.\d+\.\d+\.\d+$")
# warnings and errors of the compiler, linker, resource compiler and ninja/msbuild, the build has /WX but the linker and others can still warn.
# (warnings of cmake itself are not included, those come from third party projects like GoogleTest)
PROBLEM_RE = re.compile(r"\b(warning|error|fatal error)\s+(C|LNK|D|RC|MSB)\d+\b", re.IGNORECASE)


class ReleaseError(Exception):
    pass


def tag_name(version):
    return version  # the tags have no 'v': 1.22.0.0


def release_name(version):
    return "v" + version  # the zip, the title of the release on GitHub and the commit have one: v1.22.0.0


def zip_path(version):
    return BUILD_DIR / ("debugviewpp-%s.zip" % release_name(version))


def log(message):
    print("== " + message, flush=True)


def run(command, cwd=ROOT, env=None, capture=False):
    """Runs a command, raises ReleaseError when it fails. Returns the output when capture is set."""
    shown = " ".join(str(c) for c in command)
    if not capture:
        log(shown)
    try:
        result = subprocess.run(command, cwd=cwd, env=env, check=False, text=True, stdout=subprocess.PIPE if capture else None, stderr=subprocess.STDOUT if capture else None)
    except FileNotFoundError:
        raise ReleaseError("%s not found" % command[0])
    if result.returncode != 0:
        raise ReleaseError("command failed (exit code %d): %s%s" % (result.returncode, shown, ("\n" + result.stdout) if capture and result.stdout else ""))
    return result.stdout if capture else ""


def git(*args):
    return run(["git", *args], capture=True).strip()


# ---------------------------------------------------------------- compile

def visual_studio_environment():
    """The environment of a 'x64 Native Tools' prompt, unless the compiler is already available."""
    env = _visual_studio_environment()
    # CMakeLists.buildnumber.cmake runs 'cmd /C UpdateBuildNr.cmd' from the current directory, that fails when this is set
    for name in [name for name in env if name.upper() == "NODEFAULTCURRENTDIRECTORYINEXEPATH"]:
        del env[name]
    env["DEBUGVIEW_KEEP_BUILD_NUMBER"] = "1"  # do not raise the build number, a release has the version that was asked for
    return env


def _visual_studio_environment():
    if shutil.which("cl"):
        return dict(os.environ)

    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        raise ReleaseError("Visual Studio was not found (vswhere.exe is missing), run this from an 'x64 Native Tools Command Prompt'")
    install = subprocess.run([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
                             capture_output=True, text=True).stdout.strip()
    vcvars = Path(install) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not install or not vcvars.exists():
        raise ReleaseError("Visual Studio with the C++ tools was not found, run this from an 'x64 Native Tools Command Prompt'")

    output = subprocess.run('cmd /c ""%s" >nul && set"' % vcvars, capture_output=True, text=True, shell=True).stdout
    env = {}
    for line in output.splitlines():
        name, separator, value = line.partition("=")
        if separator:
            env[name] = value
    return env


def read_version_str():
    match = re.search(r'#define VERSION_STR "([^"]+)"', VERSION_H.read_text())
    if not match:
        raise ReleaseError("VERSION_STR not found in " + str(VERSION_H))
    return match.group(1)


def prepare_version(version):
    """Writes the version to version.h. CMake runs UpdateBuildNr.cmd when it configures, which raises the build number by one,
    unless DEBUGVIEW_KEEP_BUILD_NUMBER is set (see visual_studio_environment), so the version stays what it is here."""
    VERSION_H.write_bytes(('#define VERSION %s \r\n#define VERSION_STR "%s" \r\n' % (version.replace(".", ","), version)).encode())


def run_build(command, env, problems):
    log(" ".join(str(c) for c in command))
    process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
    for line in process.stdout:
        print(line, end="", flush=True)
        if PROBLEM_RE.search(line):
            problems.append(line.strip())
    if process.wait() != 0:
        raise ReleaseError("the build failed (exit code %d)" % process.returncode)


def compile_clean(version):
    log("compile clean, version " + version)
    env = visual_studio_environment()

    if BUILD_DIR.exists():
        log("removing " + str(BUILD_DIR))
        shutil.rmtree(BUILD_DIR)

    prepare_version(version)
    problems = []
    run_build(["cmake", "-S", str(ROOT), "-B", str(BUILD_DIR), "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=" + str(BIN_DIR)], env, problems)

    actual = read_version_str()
    if actual != version:
        raise ReleaseError("the build has version %s instead of %s, is application/DebugViewpp/UpdateBuildNr.cmd changed?" % (actual, version))

    run_build(["cmake", "--build", str(BUILD_DIR)], env, problems)
    if problems:
        raise ReleaseError("the build is not clean, %d warning(s)/error(s):\n  %s" % (len(problems), "\n  ".join(problems[:20])))
    for name in BINARIES:
        if not (BIN_DIR / name).exists():
            raise ReleaseError("%s was not built" % (BIN_DIR / name))
    log("compiled cleanly: version %s, binaries in %s" % (version, BIN_DIR))


# ---------------------------------------------------------------- tag

def uncommitted_changes():
    """The tracked files that are changed in the working directory (untracked files do not count)."""
    return [line.split(None, 1)[1].strip('"') for line in git("status", "--porcelain", "--untracked-files=no").splitlines() if line.strip()]


def check_uncommitted(force, action, ignore=(), quiet=False):
    """A release is of what is committed, so uncommitted changes are an error, unless -f is given."""
    changes = [c for c in uncommitted_changes() if c not in ignore]
    if changes and force:
        if not quiet:
            log("warning: -f, %s without the uncommitted changes of:\n  %s" % (action, "\n  ".join(changes)))
    elif changes:
        raise ReleaseError("there are uncommitted changes, commit or stash them first (or use -f):\n  " + "\n  ".join(changes))


def create_tag(version, dry_run, force):
    tag = tag_name(version)
    log("git tag " + tag)

    if read_version_str() != version:
        raise ReleaseError("version.h has version %s, not %s: compile first (-c)" % (read_version_str(), version))
    if git("tag", "--list", tag):
        if not force:
            raise ReleaseError("the tag %s exists already (use -f to continue with it)" % tag)
        on = git("log", "-1", "--format=%h %s", tag + "^{commit}")
        where = "on HEAD" if git("rev-parse", tag + "^{commit}") == git("rev-parse", "HEAD") else "on %s, not on HEAD" % on
        log("warning: -f, the tag %s exists already, %s: it is kept as it is" % (tag, where))
        return

    # the tag must be a commit that has this version in it; the version files are the only change a release build makes
    version_paths = [str(p.relative_to(ROOT)).replace("\\", "/") for p in VERSION_FILES]
    check_uncommitted(force, "the tag is made", ignore=version_paths)

    if any(c in version_paths for c in uncommitted_changes()):
        if dry_run:
            log("dry run: would commit %s as 'Release %s'" % (", ".join(version_paths), release_name(version)))
        else:
            run(["git", "add", *version_paths])
            run(["git", "commit", "-m", "Release " + release_name(version), "--", *version_paths])
    if dry_run:
        log("dry run: would create the tag %s on %s" % (tag, git("rev-parse", "--short", "HEAD")))
        return
    run(["git", "tag", "-a", tag, "-m", "DebugView++ " + release_name(version)])
    log("created tag %s on %s (not pushed, use -p to publish, or: git push origin %s)" % (tag, git("rev-parse", "--short", "HEAD"), tag))


# ---------------------------------------------------------------- zip

def file_version(path):
    command = ["powershell", "-NoProfile", "-Command", "(Get-Item -LiteralPath '%s').VersionInfo.FileVersion" % path]
    return run(command, capture=True).strip()


def create_zip(version):
    target = zip_path(version)
    log("zip " + str(target))
    missing = [name for name in BINARIES if not (BIN_DIR / name).exists()]
    if missing:
        raise ReleaseError("not found in %s: %s (compile first, -c)" % (BIN_DIR, ", ".join(missing)))
    for name in VERSIONED_BINARIES:
        found = file_version(BIN_DIR / name)
        if found != version:
            raise ReleaseError("%s has version %s, not %s: it is not from this release (compile first, -c)" % (name, found or "(none)", version))

    if target.exists():
        target.unlink()
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in BINARIES:
            archive.write(BIN_DIR / name, name)
    with zipfile.ZipFile(target) as archive:
        if archive.testzip() is not None:
            raise ReleaseError("the zip file is corrupt")
        for info in archive.infolist():
            log("  %-28s %9.1f KB" % (info.filename, info.file_size / 1024))
    log("created %s (%.1f MB)" % (target, target.stat().st_size / 1024 / 1024))


# ---------------------------------------------------------------- publish

def github_repo():
    """owner/name of the GitHub repository that origin points to. gh is told this explicitly, it does not need a default repository."""
    url = git("remote", "get-url", "origin")
    match = re.search(r"github\.com[:/]+([^/]+)/([^/]+?)(?:\.git)?/?$", url)
    if not match:
        raise ReleaseError("origin (%s) is not a GitHub repository" % url)
    return "%s/%s" % match.groups()


def check_github():
    """Checks that a release can be made on GitHub, returns the repository."""
    if not shutil.which("gh"):
        raise ReleaseError("the GitHub CLI (gh) was not found")
    run(["gh", "auth", "status"], capture=True)
    return github_repo()


def publish(version, dry_run, force):
    tag = tag_name(version)
    archive = zip_path(version)
    log("publish " + tag)

    check_uncommitted(force, "the release is published")
    if not archive.exists():
        raise ReleaseError("%s does not exist (create it with -z)" % archive)
    missing = [name for name in BINARIES if not (BIN_DIR / name).exists()]
    if missing:
        raise ReleaseError("not found in %s: %s (compile first, -c)" % (BIN_DIR, ", ".join(missing)))
    if not git("tag", "--list", tag):
        raise ReleaseError("the tag %s does not exist (create it with -t)" % tag)
    repo = check_github()

    # the release is made from the tag, so the tag (and the commit it points to) must be on GitHub
    push = ["git", "push", "origin", "refs/tags/" + tag]
    # the zip and next to it the executables themselves, as assets
    assets = [str(archive)] + [str(BIN_DIR / name) for name in BINARIES]
    create = ["gh", "release", "create", tag, *assets, "--repo", repo, "--title", release_name(version), "--generate-notes", "--verify-tag"]
    if dry_run:
        log("dry run: would run: " + " ".join(push))
        log("dry run: would run: " + " ".join(create))
        return
    run(push)
    run(create)
    log("published; the commit of the tag is not on a branch on GitHub until you push the branch (git push)")


# ---------------------------------------------------------------- main

def parse_arguments(argv):
    parser = argparse.ArgumentParser(description="Create a release of DebugView++.", epilog=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-v", dest="version", metavar="<version>", help="the version, for example 2.0.0.0")
    parser.add_argument("-c", dest="compile", action="store_true", help="compile clean, with this version (requires -v)")
    parser.add_argument("-t", dest="tag", action="store_true", help="create the git tag <version>, for example 2.0.0.0 (requires -v)")
    parser.add_argument("-z", dest="zip", action="store_true", help="create the zip file (requires -v, the binaries must exist)")
    parser.add_argument("-p", dest="publish", action="store_true", help="publish the release on GitHub with the zip attached (requires -v, the zip must exist)")
    parser.add_argument("-f", dest="force", action="store_true", help="go on when there are uncommitted changes, for -t and -p (they are not part of the release)")
    parser.add_argument("-r", dest="release", metavar="<version>", help="a complete release: -v <version> -c -t -z -p")
    parser.add_argument("--dry-run", action="store_true", help="do not commit, tag, push or publish, only say what would be done")
    args = parser.parse_args(argv)

    if args.release:
        if args.version and args.version != args.release:
            parser.error("-r and -v have different versions")
        args.version = args.release
        args.compile = args.tag = args.zip = args.publish = True
    if not (args.compile or args.tag or args.zip or args.publish):
        parser.error("nothing to do, specify -c, -t, -z, -p or -r")
    if not args.version:
        parser.error("-c, -t, -z and -p require a version, specify -v <version>")
    if not VERSION_RE.match(args.version):
        parser.error("the version must have four numbers, like 2.0.0.0, not '%s'" % args.version)
    return args


def preflight(args):
    """What can be checked before the first step is checked first, so a combination like -c -t does not build and then refuse to tag."""
    if args.tag and not args.force and git("tag", "--list", tag_name(args.version)):
        raise ReleaseError("the tag %s exists already (use -f to continue with it)" % tag_name(args.version))
    if args.tag or args.publish:
        # with -t the version files are committed by the tag step, and the compile step is what changes them
        version_paths = [str(p.relative_to(ROOT)).replace("\\", "/") for p in VERSION_FILES] if args.tag else []
        # -f only says that it is ok, the step that is going to make the tag or publish says what is not included
        check_uncommitted(args.force, "", ignore=version_paths, quiet=True)
    if args.publish:
        check_github()


def main(argv):
    args = parse_arguments(argv)
    try:
        preflight(args)
        if args.compile:
            compile_clean(args.version)
        if args.tag:
            create_tag(args.version, args.dry_run, args.force)
        if args.zip:
            create_zip(args.version)
        if args.publish:
            publish(args.version, args.dry_run, args.force)
    except ReleaseError as error:
        print("\nERROR: %s" % error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

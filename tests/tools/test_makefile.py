"""Check phony entry points and configure/build failure launch protection."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


def check_entrypoints():
    make = shutil.which("make")
    if make is None:
        raise RuntimeError("make is required for the entry-point check")
    repository = Path(__file__).resolve().parents[2]
    storage = repository / ".local" / "test-makefile"
    storage.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="moorhuhn2-make-", dir=storage) as temporary:
        root = Path(temporary)
        shutil.copy2(repository / "Makefile", root / "Makefile")
        for target in ("build", "run", "configure"):
            (root / target).touch()
        cmake = root / "cmake-check"
        cmake.write_text(
            "#!/usr/bin/env python3\n"
            "import json, os, sys\n"
            "with open(os.environ['MOORHUHN2_MAKE_LOG'], 'a') as stream:\n"
            "    stream.write(json.dumps(sys.argv[1:]) + '\\n')\n"
            "if '-S' in sys.argv and os.environ['MOORHUHN2_FAIL_CONFIGURE'] == '1':\n"
            "    sys.exit(10)\n"
            "if '--build' in sys.argv and os.environ['MOORHUHN2_FAIL_BUILD'] == '1':\n"
            "    sys.exit(11)\n",
            encoding="utf-8",
        )
        cmake.chmod(0o755)
        log = root / "calls.jsonl"
        environment = dict(os.environ, MOORHUHN2_MAKE_LOG=str(log), MOORHUHN2_FAIL_BUILD="0", MOORHUHN2_FAIL_CONFIGURE="0")
        for name in ("MAKEFLAGS", "MFLAGS", "MAKEOVERRIDES"):
            environment.pop(name, None)

        def calls():
            return [json.loads(line) for line in log.read_text(encoding="utf-8").splitlines()] if log.exists() else []

        def invoke(target, success, expected):
            before = len(calls())
            result = subprocess.run(
                [make, target, f"CMAKE={cmake}", "BUILD_DIR=output"],
                cwd=root, env=environment, capture_output=True, text=True, check=False,
            )
            if (result.returncode == 0) != success:
                raise RuntimeError(result.stdout + result.stderr)
            actual = calls()[before:]
            kinds = ["configure" if "-S" in call else "run" if "--target" in call and call[call.index("--target") + 1] == "run" else "build" for call in actual]
            if kinds != expected:
                raise RuntimeError(f"Expected {expected} calls for make {target}, received {kinds}")

        invoke("build", True, ["configure", "build"])
        invoke("build", True, ["configure", "build"])
        invoke("run", True, ["configure", "build", "run"])
        environment["MOORHUHN2_FAIL_CONFIGURE"] = "1"
        invoke("run", False, ["configure"])
        environment["MOORHUHN2_FAIL_CONFIGURE"] = "0"
        environment["MOORHUHN2_FAIL_BUILD"] = "1"
        invoke("run", False, ["configure", "build"])


class MakefileTest(unittest.TestCase):
    def test_build_failure_prevents_launch_and_targets_are_phony(self):
        check_entrypoints()


if __name__ == "__main__":
    unittest.main()

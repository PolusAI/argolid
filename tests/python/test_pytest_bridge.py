import os
import subprocess
import sys
import unittest
from pathlib import Path


if "PYTEST_CURRENT_TEST" in os.environ:
    raise unittest.SkipTest("pytest bridge should not run under pytest")

class TestPytestSuite(unittest.TestCase):
    """Run pytest-based tests under unittest-driven CI."""
    def test_pytest(self) -> None:
        "The entry point of pytest execution"
        tests_python = Path(__file__).resolve().parent  # tests/python

        env = os.environ.copy()
        env["PYTEST_DISABLE_PLUGIN_AUTOLOAD"] = "1"

        subprocess.run(
            [
                sys.executable,
                "-m",
                "pytest",
                "-vv",
                "-s",
                str(tests_python),
            ],
            check=True,
            env=env,
            timeout=900,
        )
